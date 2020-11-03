#  Python script to run TauDEM

import os
import sys
import shutil
import subprocess
import inspect

from os.path import join as _join
from os.path import split as _split
from os.path import exists as _exists
import math

from pprint import pprint

from osgeo import gdal, ogr, osr
import utm

import numpy as np

from wepppy.all_your_base import (
    read_arc,
    utm_srid,
    isfloat,
    GeoTransformer,
    wgs84_proj4,
    IS_WINDOWS,
    NCPU
)


def get_utm_zone(srs):
    """
    extracts the utm_zone from an osr.SpatialReference object (srs)

    returns the utm_zone as an int, returns None if utm_zone not found
    """

    if not isinstance(srs, osr.SpatialReference):
        raise TypeError('srs is not a osr.SpatialReference instance')

    if srs.IsProjected() != 1:
        return None

    projcs = srs.GetAttrValue('projcs')
    assert 'UTM' in projcs

    datum = None
    if 'NAD83' in projcs:
        datum = 'NAD83'
    elif 'WGS84' in projcs:
        datum = 'WGS84'
    elif 'NAD27' in projcs:
        datum = 'NAD27'

    # should be something like NAD83 / UTM zone 11N...

    if '/' in projcs:
        utm_token = projcs.split('/')[1]
    else:
        utm_token = projcs
    if 'UTM' not in utm_token:
        return None

    # noinspection PyBroadException
    try:
        utm_zone = int(''.join([k for k in utm_token if k in '0123456789']))
    except Exception:
        return None

    if utm_zone < 0 or utm_zone > 60:
        return None

    hemisphere = projcs[-1]
    return datum, utm_zone, hemisphere


# This also assumes that MPICH2 is properly installed on your machine and that TauDEM command line executables exist
# MPICH2.  Obtain from http://www.mcs.anl.gov/research/projects/mpich2/
# Install following instructions at http://hydrology.usu.edu/taudem/taudem5.0/downloads.html.
# It is important that you install this from THE ADMINISTRATOR ACCOUNT.

# TauDEM command line executables.

taudem_bin = '../../bin'

assert _exists(_join(taudem_bin, 'pitremove')), 'Cannot find pitremove'
assert _exists(_join(taudem_bin, 'd8flowdir')), 'Cannot find d8flowdir'
assert _exists(_join(taudem_bin, 'aread8')), 'Cannot find aread8'
assert _exists(_join(taudem_bin, 'threshold')), 'Cannot find threshold'
assert _exists(_join(taudem_bin, 'moveoutletstostrm')), 'Cannot find moveoutletstostrm'
assert _exists(_join(taudem_bin, 'peukerdouglas')), 'Cannot find peukerdouglas'
assert _exists(_join(taudem_bin, 'dropanalysis')), 'Cannot find dropanalysis'
assert _exists(_join(taudem_bin, 'streamnet')), 'Cannot find streamnet'
assert _exists(_join(taudem_bin, 'dropanalysis')), 'Cannot find dropanalysis'

_TIMEOUT = 60

_USE_MPI = True


_outlet_template_geojson = """{{
"type": "FeatureCollection",
"name": "Outlet",
"crs": {{ "type": "name", "properties": {{ "name": "urn:ogc:def:crs:EPSG::{epsg}" }} }},
"features": [
{{ "type": "Feature", "properties": {{ "Id": 0 }}, 
   "geometry": {{ "type": "Point", "coordinates": [ {easting}, {northing} ] }} }}
]
}}"""

class TauDEMRunner:
    """
    Object oriented abstraction for running TauDEM

    For more infomation on topaz see the manual available here:
        https://hydrology.usu.edu/taudem/taudem5/documentation.html
    """
    def __init__(self, wd, dem):
        """
        provide a path to a directory to store the topaz files a
        path to a dem
        """
        global taudem_bin

        # verify the dem exists
        if not _exists(dem):
            raise Exception('file "%s" does not exist' % dem)

        # verify the dem exists
        if not _exists(wd):
            raise Exception('working directory "%s" does not exist' % wd)

        _dem = _join(wd, _split(dem)[-1])
        shutil.copyfile(dem, _dem)

        self.wd = wd
        self.dem = _dem

        self.outlet = None
        self.actual_outlet = None
        self._parse_dem()

    def _parse_dem(self):
        """
        Uses gdal to extract elevation values from dem and puts them in a
        single column ascii file named DEDNM.INP for topaz
        """
        dem = self.dem

        # open the dataset
        ds = gdal.Open(dem)

        # read and verify the num_cols and num_rows
        num_cols = ds.RasterXSize
        num_rows = ds.RasterYSize

        if num_cols <= 0 or num_rows <= 0:
            raise Exception('input is empty')

        # read and verify the _transform
        _transform = ds.GetGeoTransform()

        if abs(_transform[1]) != abs(_transform[5]):
            raise Exception('input cells are not square')

        cellsize = abs(_transform[1])
        ul_x = int(round(_transform[0]))
        ul_y = int(round(_transform[3]))

        lr_x = ul_x + cellsize * num_cols
        lr_y = ul_y - cellsize * num_rows

        ll_x = int(ul_x)
        ll_y = int(lr_y)

        # read the projection and verify dataset is in utm
        srs = osr.SpatialReference()
        srs.ImportFromWkt(ds.GetProjectionRef())

        datum, utm_zone, hemisphere = get_utm_zone(srs)
        if utm_zone is None:
            raise Exception('input is not in utm')

        # get band
        band = ds.GetRasterBand(1)

        # get band dtype
        dtype = gdal.GetDataTypeName(band.DataType)

        if 'float' not in dtype.lower():
            raise Exception('dem dtype does not contain float data')

        # extract min and max elevation
        stats = band.GetStatistics(True, True)
        minimum_elevation = stats[0]
        maximum_elevation = stats[1]

        # store the relevant variables to the class
        self.transform = _transform
        self.num_cols = num_cols
        self.num_rows = num_rows
        self.cellsize = cellsize
        self.ul_x = ul_x
        self.ul_y = ul_y
        self.lr_x = lr_x
        self.lr_y = lr_y
        self.ll_x = ll_x
        self.ll_y = ll_y
        self.datum = datum
        self.hemisphere = hemisphere
        self.epsg = utm_srid(utm_zone, datum, hemisphere)
        self.utm_zone = utm_zone
        self.srs_proj4 = srs.ExportToProj4()
        self.epsg = utm_srid(utm_zone, hemisphere='')
        srs.MorphToESRI()
        self.srs_wkt = srs.ExportToWkt()
        self.minimum_elevation = minimum_elevation
        self.maximum_elevation = maximum_elevation

        del ds

    def longlat_to_pixel(self, long, lat):
        """
        return the x,y pixel coords of long, lat
        """

        # unpack variables for instance
        cellsize, num_cols, num_rows = self.cellsize, self.num_cols, self.num_rows
        ul_x, ul_y, lr_x, lr_y = self.ul_x, self.ul_y, self.lr_x, self.lr_y

        # find easting and northing
        x, y, _, _ = utm.from_latlon(lat, long, self.utm_zone)

        # assert this makes sense with the stored extent
        assert round(x) >= round(ul_x), (x, ul_x)
        assert round(x) <= round(lr_x), (x, lr_x)
        assert round(y) >= round(lr_y), (y, lr_y)
        assert round(y) <= round(y), (y, ul_y)

        # determine pixel coords
        _x = int(round((x - ul_x) / cellsize))
        _y = int(round((ul_y - y) / cellsize))

        # sanity check on the coords
        assert 0 <= _x < num_cols, str(x)
        assert 0 <= _y < num_rows, str(y)

        return _x, _y

    def pixel_to_utm(self, x, y):
        """
        return the utm coords from pixel coords
        """

        # unpack variables for instance
        cellsize, num_cols, num_rows = self.cellsize, self.num_cols, self.num_rows
        ul_x, ul_y, lr_x, lr_y = self.ul_x, self.ul_y, self.lr_x, self.lr_y

        assert 0 <= x < num_cols
        assert 0 <= y < num_rows

        easting = ul_x + cellsize * x
        northing = ul_y - cellsize * y

        return easting, northing

    def longlat_to_utm(self, long, lat):
        """
        return the utm coords from longlat coords
        """
        wgs2proj_transformer = GeoTransformer(src_proj4=wgs84_proj4, dst_proj4=self.srs_proj4)
        return wgs2proj_transformer.transform(long, lat)

    def pixel_to_longlat(self, x, y):
        """
        return the long/lat (WGS84) coords from pixel coords
        """
        easting, northing = self.pixel_to_utm(x, y)
        proj2wgs_transformer = GeoTransformer(src_proj4=self.srs_proj4, dst_proj4=wgs84_proj4)
        return proj2wgs_transformer.transform(easting, northing)

    @property
    def dem_args(self):
        return ['-z', self.dem]

    @property
    def fel_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'fel')
        return '.'.join(_dem)

    _fel = fel_path

    @property
    def fel_args(self):
        return ['-fel', self._fel]

    @property
    def point_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'p')
        return '.'.join(_dem)

    _p = point_path

    @property
    def point_args(self):
        return ['-p', self._p]

    @property
    def slope_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'sd8')
        return '.'.join(_dem)

    _sd8 = slope_path

    @property
    def slope_args(self):
        return ['-sd8', self._sd8]

    @property
    def contrib_area_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'ad8')
        return '.'.join(_dem)

    _ad8 = contrib_area_path

    @property
    def contrib_area_args(self):
        return ['-ad8', self._ad8]

    @property
    def stream_raster_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'src')
        return '.'.join(_dem)

    _src = stream_raster_path

    @property
    def stream_raster_args(self):
        return ['-src', self._src]

    @property
    def pk_stream_raster_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'pksrc')
        return '.'.join(_dem)

    _pksrc = pk_stream_raster_path

    @property
    def channel_shp_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'net')
        _dem[-1] = 'geojson'
        return '.'.join(_dem)

    _net = channel_shp_path

    @property
    def channel_shp_args(self):
        return ['-net', self._net]

    @property
    def outlet_shp_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'o')
        _dem[-1] = 'geojson'
        return '.'.join(_dem)

    _o = outlet_shp_path

    @property
    def outlet_shp_args(self):
        return ['-o', self._o]

    @property
    def actual_outlet_shp_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'om')
        _dem[-1] = 'geojson'
        return '.'.join(_dem)

    _om = actual_outlet_shp_path

    @property
    def actual_outlet_shp_args(self):
        return ['-om', self._om]

    @property
    def short_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'ss')
        return '.'.join(_dem)

    _ss = short_path

    @property
    def short_args(self):
        return ['-ss', self._ss]

    @property
    def short_area_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'ssa')
        return '.'.join(_dem)

    _ssa = short_area_path

    @property
    def short_area_args(self):
        return ['-ssa', self._ssa]

    @property
    def drop_file_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'drp')
        _dem[-1] = 'txt'
        return '.'.join(_dem)

    _drp = drop_file_path

    @property
    def drop_file_args(self):
        return ['-drp', self._drp]

    @property
    def tree_file_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'tree')
        _dem[-1] = 'txt'
        return '.'.join(_dem)

    _tree = tree_file_path

    @property
    def tree_file_args(self):
        return ['-tree', self._tree]

    @property
    def coord_file_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'coord')
        _dem[-1] = 'txt'
        return '.'.join(_dem)

    _coord = coord_file_path

    @property
    def coord_file_args(self):
        return ['-coord', self._coord]

    @property
    def order_file_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'ord')
        return '.'.join(_dem)

    _ord = order_file_path

    @property
    def order_file_args(self):
        return ['-ord', self._ord]

    @property
    def watershed_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'w')
        return '.'.join(_dem)

    _w = watershed_path

    @property
    def watershed_args(self):
        return ['-w', self._w]

    @property
    def gagewatershed_path(self):
        _dem = self.dem.split('.')
        _dem.insert(-1, 'gw')
        return '.'.join(_dem)

    _gw = gagewatershed_path

    @property
    def gagewatershed_args(self):
        return ['-gw', self._gw]

    @property
    def _mpi_args(self):
        global _USE_MPI

        if _USE_MPI:
            return ['mpiexec', '-n', NCPU]
        else:
            return []

    def _nix_runner(self, cmd, verbose=True):
        cmd = [str(v) for v in cmd]
        caller = inspect.stack()[1].function
        _log = open(_join(self.wd, caller + '.log'), 'w')

        if verbose:
            print(caller, cmd)

        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=_log, stderr=_log)
        p.wait()
        _log.close()

    @property
    def pitremove(self):
        return self._mpi_args + [_join(taudem_bin, 'pitremove')]

    @property
    def d8flowdir(self):
        return self._mpi_args + [_join(taudem_bin, 'd8flowdir')]

    @property
    def aread8(self):
        return self._mpi_args + [_join(taudem_bin, 'aread8')]

    @property
    def threshold(self):
        return self._mpi_args + [_join(taudem_bin, 'threshold')]

    @property
    def moveoutletstostrm(self):
        return self._mpi_args + [_join(taudem_bin, 'moveoutletstostrm')]

    @property
    def peukerdouglas(self):
        return self._mpi_args + [_join(taudem_bin, 'peukerdouglas')]

    @property
    def dropanalysis(self):
        return self._mpi_args + [_join(taudem_bin, 'dropanalysis')]

    @property
    def streamnet(self):
        return self._mpi_args + [_join(taudem_bin, 'streamnet')]

    @property
    def gagewatershed(self):
        return self._mpi_args + [_join(taudem_bin, 'gagewatershed')]

    def run_pitremove(self):
        """
        This function takes as input an elevation data grid and outputs a hydrologically correct elevation grid file
        with pits filled, using the flooding algorithm.

        in: dem
        out: fel
        """
        self._nix_runner(self.pitremove + self.dem_args + self.fel_args)
        assert _exists(self.fel_path), self.fel_path

    def run_d8flowdir(self):
        """
        This function takes as input the hydrologically correct elevation grid and outputs D8 flow direction and slope
        for each grid cell. In flat areas flow directions are assigned away from higher ground and towards lower ground
        using the method of Garbrecht and Martz (Garbrecht and Martz, 1997).

        in: fel
        out: point, slope
        """
        self._nix_runner(self.d8flowdir + self.fel_args + self.point_args + self.slope_args)
        assert _exists(self.point_path), self.point_path
        assert _exists(self.slope_path), self.slope_path

    def run_aread8(self, no_edge_contamination_checking=False):
        """
        This function takes as input a D8 flow directions file and outputs the contributing area. The result is the
        number of grid cells draining through each grid cell. The optional command line argument for the outlet
        shapefile results in only the area contributing to outlet points in the shapefile being calculated. The optional
        weight grid input results in the output being the accumulation (sum) of the weights from upstream grid cells
        draining through each grid cell. By default the program checks for edge contamination. The edge contamination
        checking may be overridden with the optional command line.

        in: point
        out: contrib_area
        """
        self._nix_runner(self.aread8 + self.point_args + self.contrib_area_args +
                         ([], ['-nc'])[no_edge_contamination_checking])

    def run_threshold(self, ssa, src, threshold=1000):
        """
        This function operates on any grid and outputs an indicator (1,0) grid of grid cells that have values >= the
        input threshold. The standard use is to threshold an accumulated source area grid to determine a stream raster.
        There is an option to include a mask input to replicate the functionality for using the sca file as an edge
        contamination mask. The threshold logic should be src = ((ssa >= thresh) & (mask >=0)) ? 1:0

        in: ssa
        out: src
        """
        self._nix_runner(self.threshold + ['-ssa', ssa, '-src', src, '-thresh', threshold])

    def run_src_threshold(self, threshold=1000):
        self.run_threshold(ssa=self._ad8, src=self._src, threshold=threshold)

        assert _exists(self._src), self._src

    def run_moveoutletstostrm(self, long, lat):
        """
        This function finds the closest channel location to the requested location

        :param long: requested longitude
        :param lat: requested latitude
        """
        self.outlet = long, lat
        self._make_outlet()

        self._nix_runner(self.moveoutletstostrm + self.point_args + self.stream_raster_args +
                         self.outlet_shp_args + self.actual_outlet_shp_args)

        assert _exists(self._om), self._om

    def _make_outlet(self):
        outlet = self.outlet
        assert outlet is not None
        long, lat = outlet
        easting, northing = self.longlat_to_utm(long=long, lat=lat)

        with open(self.outlet_shp_path, 'w') as fp:
            fp.write(_outlet_template_geojson.format(epsg=self.epsg, easting=easting, northing=northing))

        assert _exists(self._o), self._o

    def run_peukerdouglas(self, center_weight=0.4, side_weight=0.1, diagonal_weight=0.05):
        """
        This function operates on an elevation grid and outputs an indicator (1,0) grid of upward curved grid cells
        according to the Peuker and Douglas algorithm. This is to be based on code in tardemlib.cpp/source.

        in: fel
        out: short
        """
        self._nix_runner(self.peukerdouglas + self.fel_args + self.short_args +
                         ['-par', center_weight, side_weight, diagonal_weight])

        assert _exists(self.short_path), self.short_path

    @property
    def drop_analysis_threshold(self):
        with open(self._drp) as fp:
            lines = fp.readlines()

        last = lines[-1]

        assert 'Optimum Threshold Value:' in last, '\n'.join(lines)
        return float(last.replace('Optimum Threshold Value:', '').strip())

    def run_peukerdouglas_stream_delineation(self, threshmin=5, threshmax=500, nthresh=10, steptype=0, threshold=None):
        self._nix_runner(self.aread8 + self.point_args + ['-o', self._om] + ['-ad8', self._ssa] + ['-wg', self._ss])

        assert _exists(self._ssa), self._ssa

        self._nix_runner(self.dropanalysis + self.point_args + self.fel_args +
                         self.contrib_area_args + self.short_area_args + self.drop_file_args +
                         ['-o', self._om] + ['-par', threshmin, threshmax, nthresh, steptype])

        assert _exists(self._drp), self._drp

        if threshold is None:
            threshold = self.drop_analysis_threshold

        self.run_threshold(self._ssa, self._pksrc, threshold=threshold)

    def run_streamnet(self, single_watershed=False):
        """
        """
        self._nix_runner(self.streamnet + self.fel_args + self.point_args + self.contrib_area_args +
                         ['-src', self._pksrc] + self.order_file_args + self.tree_file_args + self.channel_shp_args +
                         self.coord_file_args + self.watershed_args + ['-o', self._om] +
                         ([], ['-sw'])[single_watershed])

        assert _exists(self._w), self._w

    def run_gagewatershed(self):
        """
        """
        self._nix_runner(self.gagewatershed + self.point_args + ['-o', self._om] + self.gagewatershed_args)

        assert _exists(self._gw), self._gw

 #Streamnet -fel loganfel.tif -p loganp.tif -ad8
 #    loganad8.tif -src logansrc.tif -ord loganord3.tif -tree
 #    logantree.dat -coord logancoord.dat -net logannet.shp -w
 #    loganw.tif -o loganoutlet.shp

if __name__ == "__main__":
    wd = 'test'
    dem = 'logan.tif'

    taudem = TauDEMRunner(wd=wd, dem=dem)
    taudem.run_pitremove()
    taudem.run_d8flowdir()
    taudem.run_aread8()
    taudem.run_src_threshold()
    taudem.run_moveoutletstostrm(long=-111.784228758779406, lat=41.743629188805421)
    taudem.run_peukerdouglas()
    taudem.run_peukerdouglas_stream_delineation()
    taudem.run_streamnet()
    taudem.run_gagewatershed()


"""
plot(z)

# Pitremove
system("mpiexec -n 8 pitremove -z logan.tif -fel loganfel.tif")
fel = raster("loganfel.tif")
plot(fel)

# D8 flow directions
system("mpiexec -n 8 D8Flowdir -p loganp.tif -sd8 logansd8.tif -fel loganfel.tif",
       show.output.on.console = F, invisible = F)
p = raster("loganp.tif")
plot(p)
sd8 = raster("logansd8.tif")
plot(sd8)

# Contributing area
system("mpiexec -n 8 AreaD8 -p loganp.tif -ad8 loganad8.tif")
ad8 = raster("loganad8.tif")
plot(log(ad8))
zoom(log(ad8))

# Grid Network 
system("mpiexec -n 8 Gridnet -p loganp.tif -gord logangord.tif -plen loganplen.tif -tlen logantlen.tif")
gord = raster("logangord.tif")
plot(gord)
zoom(gord)

# DInf flow directions
system("mpiexec -n 8 DinfFlowdir -ang loganang.tif -slp loganslp.tif -fel loganfel.tif",
       show.output.on.console = F, invisible = F)
ang = raster("loganang.tif")
plot(ang)
slp = raster("loganslp.tif")
plot(slp)

# Dinf contributing area
system("mpiexec -n 8 AreaDinf -ang loganang.tif -sca logansca.tif")
sca = raster("logansca.tif")
plot(log(sca))
zoom(log(sca))

# Threshold
system("mpiexec -n 8 Threshold -ssa loganad8.tif -src logansrc.tif -thresh 100")
src = raster("logansrc.tif")
plot(src)
zoom(src)

# a quick R function to write a shapefile
makeshape.r = function(sname="shape", n=1)
{
    xy = locator(n=n)
points(xy)

# Point
dd < - data.frame(Id=1: n, X = xy$x, Y = xy$y)
ddTable < - data.frame(Id=c(1), Name=paste("Outlet", 1: n, sep = ""))
ddShapefile < - convert.to.shapefile(dd, ddTable, "Id", 1)
write.shapefile(ddShapefile, sname, arcgis=T)
}

makeshape.r("ApproxOutlets")

# Move Outlets
system("mpiexec -n 8 moveoutletstostreams -p loganp.tif -src logansrc.tif -o approxoutlets.shp -om Outlet.shp")
outpt = read.shp("outlet.shp")
approxpt = read.shp("ApproxOutlets.shp")

plot(src)
points(outpt$shp[2], outpt$shp[3], pch = 19, col = 2)
points(approxpt$shp[2], approxpt$shp[3], pch = 19, col = 4)

zoom(src)

# Contributing area upstream of outlet
system("mpiexec -n 8 Aread8 -p loganp.tif -o Outlet.shp -ad8 loganssa.tif")
ssa = raster("loganssa.tif")
plot(ssa)

# Threshold
system("mpiexec -n 8 threshold -ssa loganssa.tif -src logansrc1.tif -thresh 2000")
src1 = raster("logansrc1.tif")
plot(src1)
zoom(src1)

# Stream Reach and Watershed
system(
    "mpiexec -n 8 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc1.tif -o outlet.shp -ord loganord.tif -tree logantree.txt -coord logancoord.txt -net logannet.shp -w loganw.tif")
plot(raster("loganord.tif"))
zoom(raster("loganord.tif"))
plot(raster("loganw.tif"))

# Plot streams using stream order as width
snet = read.shapefile("logannet")
ns = length(snet$shp$shp)
for (i in 1:ns)
    {
        lines(snet$shp$shp[[i]]$points, lwd = snet$dbf$dbf$Order[i])
    }

    # Peuker Douglas stream definition
    system("mpiexec -n 8 PeukerDouglas -fel loganfel.tif -ss loganss.tif")
    ss = raster("loganss.tif")
    plot(ss)
    zoom(ss)

    #  Accumulating candidate stream source cells
    system("mpiexec -n 8 Aread8 -p loganp.tif -o outlet.shp -ad8 loganssa.tif -wg loganss.tif")
    ssa = raster("loganssa.tif")
    plot(ssa)

    #  Drop Analysis
    system(
        "mpiexec -n 8 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp.txt -o outlet.shp -par 5 500 10 0")

    # Deduce that the optimal threshold is 300 
    # Stream raster by threshold
    system("mpiexec -n 8 Threshold -ssa loganssa.tif -src logansrc2.tif -thresh 300")
    plot(raster("logansrc2.tif"))

    # Stream network
    system(
        "mpiexec -n 8 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc2.tif -ord loganord2.tif -tree logantree2.dat -coord logancoord2.dat -net logannet2.shp -w loganw2.tif -o Outlet.shp",
        show.output.on.console = F, invisible = F)

    plot(raster("loganw2.tif"))
    snet = read.shapefile("logannet2")
    ns = length(snet$shp$shp)
    for (i in 1:ns)
        {
            lines(snet$shp$shp[[i]]$points, lwd = snet$dbf$dbf$Order[i])
        }

        # Wetness Index
        system("mpiexec -n 8 SlopeAreaRatio -slp loganslp.tif -sca logansca.tif -sar logansar.tif",
               show.output.on.console = F, invisible = F)
        sar = raster("logansar.tif")
        wi = sar
        wi[,]=-log(sar[,])
        plot(wi)

        # Distance Down
        system(
            "mpiexec -n 8 DinfDistDown -ang loganang.tif -fel loganfel.tif -src logansrc2.tif -m ave v -dd logandd.tif",
            show.output.on.console = F, invisible = F)
        plot(raster("logandd.tif"))



"""