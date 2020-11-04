#  Python script to run TauDEM

import os
import sys
import shutil
import subprocess
import inspect
import json

from os.path import join as _join
from os.path import split as _split
from os.path import exists as _exists
import math

from collections import Counter
# from pprint import pprint

from osgeo import gdal, ogr, osr
import utm

import numpy as np
from scipy.ndimage import label

# import cv2

from wepppy.all_your_base import (
    read_tif,
    write_arc,
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

_thisdir = os.path.dirname(__file__)
_taudem_bin = _join(_thisdir, '../../bin')

assert _exists(_join(_taudem_bin, 'pitremove')), 'Cannot find pitremove'
assert _exists(_join(_taudem_bin, 'd8flowdir')), 'Cannot find d8flowdir'
assert _exists(_join(_taudem_bin, 'aread8')), 'Cannot find aread8'
assert _exists(_join(_taudem_bin, 'threshold')), 'Cannot find threshold'
assert _exists(_join(_taudem_bin, 'moveoutletstostrm')), 'Cannot find moveoutletstostrm'
assert _exists(_join(_taudem_bin, 'peukerdouglas')), 'Cannot find peukerdouglas'
assert _exists(_join(_taudem_bin, 'dropanalysis')), 'Cannot find dropanalysis'
assert _exists(_join(_taudem_bin, 'streamnet')), 'Cannot find streamnet'
assert _exists(_join(_taudem_bin, 'dropanalysis')), 'Cannot find dropanalysis'

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
    def __init__(self, wd, dem, vector_ext='geojson'):
        """
        provide a path to a directory to store the topaz files a
        path to a dem
        """

        # verify the dem exists
        if not _exists(wd):
            raise Exception('working directory "%s" does not exist' % wd)

        self.wd = wd

        # verify the dem exists
        if not _exists(dem):
            raise Exception('file "%s" does not exist' % dem)

        self._dem_ext = _split(dem)[-1].split('.')[-1]
        shutil.copyfile(dem, self._z)

        self._vector_ext = vector_ext

        self.user_outlet = None
        self.outlet = None
        self._parse_dem()

    def _parse_dem(self):
        """
        Uses gdal to extract elevation values from dem and puts them in a
        single column ascii file named DEDNM.INP for topaz
        """
        dem = self._z

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
        print(datum, utm_zone, hemisphere)
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
        srs.MorphToESRI()
        self.srs_wkt = srs.ExportToWkt()
        self.minimum_elevation = minimum_elevation
        self.maximum_elevation = maximum_elevation

        self.scratch = {}

        del ds

    def data_fetcher(self, band, dtype=None):
        if dtype is None:
            dtype = np.int16

        if band not in self.scratch:
            _band = getattr(self, '_' + band)
            self.scratch[band], _, _ = read_tif(_band, dtype=dtype)

        return self.scratch[band]

    def get_elevation(self, easting, northing):
        z_data = self.data_fetcher('z', dtype=np.float64)
        x, y = self.utm_to_px(easting, northing)

        return z_data[x, y]

    def utm_to_px(self, easting, northing):
        """
        return the utm coords from pixel coords
        """

        # unpack variables for instance
        cellsize, num_cols, num_rows = self.cellsize, self.num_cols, self.num_rows
        ul_x, ul_y, lr_x, lr_y = self.ul_x, self.ul_y, self.lr_x, self.lr_y

        x = int(round((easting - ul_x) / cellsize))
        y = int(round((northing - ul_y) / -cellsize))

        assert x >= 0 and x < num_rows, x
        assert y >= 0 and x < num_cols, y

        return x, y

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

    # dem
    @property
    def _z(self):
        return _join(self.wd, 'dem.%s' % self._dem_ext)

    @property
    def _z_args(self):
        return ['-z', self._z]

    # fel
    @property
    def _fel(self):
        return _join(self.wd, 'fel.tif')

    @property
    def _fel_args(self):
        return ['-fel', self._fel]

    # point
    @property
    def _p(self):
        return _join(self.wd, 'point.tif')

    @property
    def _p_args(self):
        return ['-p', self._p]

    # slope d8
    @property
    def _sd8(self):
        return _join(self.wd, 'slope_d8.tif')

    @property
    def _sd8_args(self):
        return ['-sd8', self._sd8]

    # area d8
    @property
    def _ad8(self):
        return _join(self.wd, 'area_d8.tif')

    @property
    def _ad8_args(self):
        return ['-ad8', self._ad8]

    # stream raster
    @property
    def _src(self):
        return _join(self.wd, 'src.tif')

    @property
    def _src_args(self):
        return ['-src', self._src]

    # pk stream reaster
    @property
    def _pksrc(self):
        return _join(self.wd, 'pksrc.tif')

    @property
    def _pksrc_args(self):
        return ['-src', self._pksrc]

    # net
    @property
    def _net(self):
        return _join(self.wd, 'net.%s' % self._vector_ext)

    @property
    def _net_args(self):
        return ['-net', self._net]

    # user outlet
    @property
    def _uo(self):
        return _join(self.wd, 'user_outlet.%s' % self._vector_ext)

    @property
    def _uo_args(self):
        return ['-o', self._uo]

    # outlet
    @property
    def _o(self):
        return _join(self.wd, 'outlet.%s' % self._vector_ext)

    @property
    def _o_args(self):
        return ['-o', self._o]

    # stream source
    @property
    def _ss(self):
        return _join(self.wd, 'ss.tif')

    @property
    def _ss_args(self):
        return ['-ss', self._ss]

    # ssa
    @property
    def _ssa(self):
        return _join(self.wd, 'ssa.tif')

    @property
    def _ssa_args(self):
        return ['-ssa', self._ssa]

    # drop
    @property
    def _drp(self):
        return _join(self.wd, 'drp.tif')

    @property
    def _drp_args(self):
        return ['-drp', self._drp]

    # tree
    @property
    def _tree(self):
        return _join(self.wd, 'tree.tsv')

    @property
    def _tree_args(self):
        return ['-tree', self._tree]

    # coord
    @property
    def _coord(self):
        return _join(self.wd, 'coord.tsv')

    @property
    def _coord_args(self):
        return ['-coord', self._coord]

    # order
    @property
    def _ord(self):
        return _join(self.wd, 'order.tif')

    @property
    def _ord_args(self):
        return ['-ord', self._ord]

    # watershed
    @property
    def _w(self):
        return _join(self.wd, 'watershed.tif')

    @property
    def _w_args(self):
        return ['-w', self._w]

    # gord
    @property
    def _gord(self):
        return _join(self.wd, 'gord.tif')

    @property
    def _gord_args(self):
        return ['-gord', self._gord]

    # plen
    @property
    def _plen(self):
        return _join(self.wd, 'plen.tif')

    @property
    def _plen_args(self):
        return ['-plen', self._plen]

    # tlen
    @property
    def _tlen(self):
        return _join(self.wd, 'tlen.tif')

    @property
    def _tlen_args(self):
        return ['-tlen', self._tlen]

    # subprocess methods

    @property
    def _mpi_args(self):
        global _USE_MPI

        if _USE_MPI:
            return ['mpiexec', '-n', NCPU]
        else:
            return []

    def _sys_call(self, cmd, verbose=True, intent_in=None, intent_out=None):
        # verify inputs exist
        if intent_in is not None:
            for product in intent_in:
                assert _exists(product)

        # delete outputs if they exist
        if intent_out is not None:
            for product in intent_out:
                if _exists(product):
                    os.remove(product)

        cmd = [str(v) for v in cmd]
        caller = inspect.stack()[1].function
        log = _join(self.wd, caller + '.log')
        _log = open(log, 'w')

        if verbose:
            print(caller, cmd)

        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=_log, stderr=_log)
        p.wait()
        _log.close()

        if intent_out is None:
            return

        for product in intent_out:
            if not _exists(product):
                raise Exception('{} Failed: {} does not exist. See {}'.format(caller, product, log))

        os.remove(log)

    @property
    def __pitremove(self):
        return self._mpi_args + [_join(_taudem_bin, 'pitremove')]

    @property
    def __d8flowdir(self):
        return self._mpi_args + [_join(_taudem_bin, 'd8flowdir')]

    @property
    def __aread8(self):
        return self._mpi_args + [_join(_taudem_bin, 'aread8')]

    @property
    def __gridnet(self):
        return self._mpi_args + [_join(_taudem_bin, 'gridnet')]

    @property
    def __threshold(self):
        return self._mpi_args + [_join(_taudem_bin, 'threshold')]

    @property
    def __moveoutletstostrm(self):
        return self._mpi_args + [_join(_taudem_bin, 'moveoutletstostrm')]

    @property
    def __peukerdouglas(self):
        return self._mpi_args + [_join(_taudem_bin, 'peukerdouglas')]

    @property
    def __dropanalysis(self):
        return self._mpi_args + [_join(_taudem_bin, 'dropanalysis')]

    @property
    def __streamnet(self):
        return self._mpi_args + [_join(_taudem_bin, 'streamnet')]

    @property
    def __gagewatershed(self):
        return self._mpi_args + [_join(_taudem_bin, 'gagewatershed')]

    # TauDEM wrapper methods

    def run_pitremove(self):
        """
        This function takes as input an elevation data grid and outputs a hydrologically correct elevation grid file
        with pits filled, using the flooding algorithm.

        in: dem
        out: fel
        """
        self._sys_call(self.__pitremove + self._z_args + self._fel_args,
                       intent_in=(self._z,),
                       intent_out=(self._fel,))

    def run_d8flowdir(self):
        """
        This function takes as input the hydrologically correct elevation grid and outputs D8 flow direction and slope
        for each grid cell. In flat areas flow directions are assigned away from higher ground and towards lower ground
        using the method of Garbrecht and Martz (Garbrecht and Martz, 1997).

        in: fel
        out: point, slope_d8
        """
        self._sys_call(self.__d8flowdir + self._fel_args + self._p_args + self._sd8_args,
                       intent_in=(self._fel,),
                       intent_out=(self._sd8, self._p))

    def run_aread8(self, no_edge_contamination_checking=False):
        """
        This function takes as input a D8 flow directions file and outputs the contributing area. The result is the
        number of grid cells draining through each grid cell. The optional command line argument for the outlet
        shapefile results in only the area contributing to outlet points in the shapefile being calculated. The optional
        weight grid input results in the output being the accumulation (sum) of the weights from upstream grid cells
        draining through each grid cell. By default the program checks for edge contamination. The edge contamination
        checking may be overridden with the optional command line.

        in: point
        out: area_d8
        """
        self._sys_call(self.__aread8 + self._p_args + self._ad8_args + ([], ['-nc'])[no_edge_contamination_checking],
                       intent_in=(self._p,),
                       intent_out=(self._ad8,))

    def run_gridnet(self):
        """
        in: p
        out: gord, plen, tlen
        """
        self._sys_call(self.__gridnet + self._p_args + self._gord_args + self._plen_args + self._tlen_args,
                       intent_in=(self._p,),
                       intent_out=(self._gord, self._plen, self._tlen))

    def _run_threshold(self, ssa, src, threshold=1000):
        """
        This function operates on any grid and outputs an indicator (1,0) grid of grid cells that have values >= the
        input threshold. The standard use is to threshold an accumulated source area grid to determine a stream raster.
        There is an option to include a mask input to replicate the functionality for using the sca file as an edge
        contamination mask. The threshold logic should be src = ((ssa >= thresh) & (mask >=0)) ? 1:0

        in: ssa
        out: src
        """
        self._sys_call(self.__threshold + ['-ssa', ssa, '-src', src, '-thresh', threshold],
                       intent_in=(ssa,),
                       intent_out=(src,))

    def run_src_threshold(self, threshold=1000):
        self._run_threshold(ssa=self._ad8, src=self._src, threshold=threshold)

    def _make_outlet(self, long=None, lat=None, dst=None, easting=None, northing=None):
        assert dst is not None

        if long is not None and lat is not None:
            easting, northing = self.longlat_to_utm(long=long, lat=lat)

        assert isfloat(easting), easting
        assert isfloat(northing), northing

        with open(dst, 'w') as fp:
            fp.write(_outlet_template_geojson.format(epsg=self.epsg, easting=easting, northing=northing))

        assert _exists(dst), dst
        return dst

    def run_moveoutletstostrm(self, long, lat):
        """
        This function finds the closest channel location to the requested location

        :param long: requested longitude
        :param lat: requested latitude
        """
        self.user_outlet = long, lat
        self._make_outlet(long=long, lat=lat, dst=self._uo)
        self._sys_call(self.__moveoutletstostrm + self._p_args + self._src_args + ['-o', self._uo] + ['-om', self._o],
                       intent_in=(self._p, self._src, self._uo),
                       intent_out=(self._o,))

    def run_peukerdouglas(self, center_weight=0.4, side_weight=0.1, diagonal_weight=0.05):
        """
        This function operates on an elevation grid and outputs an indicator (1,0) grid of upward curved grid cells
        according to the Peuker and Douglas algorithm. This is to be based on code in tardemlib.cpp/source.

        in: fel
        out: ss
        """
        self._sys_call(self.__peukerdouglas + self._fel_args + self._ss_args +
                       ['-par', center_weight, side_weight, diagonal_weight],
                       intent_in=(self._fel,),
                       intent_out=(self._ss,))

    @property
    def drop_analysis_threshold(self):
        """
        Reads the drop table and extracts the optimal value

        :return: optimimum threshold value from drop table
        """
        with open(self._drp) as fp:
            lines = fp.readlines()

        last = lines[-1]

        assert 'Optimum Threshold Value:' in last, '\n'.join(lines)
        return float(last.replace('Optimum Threshold Value:', '').strip())

    def run_peukerdouglas_stream_delineation(self, threshmin=5, threshmax=500, nthresh=10, steptype=0, threshold=None):
        """

        :param threshmin:
        :param threshmax:
        :param nthresh:
        :param steptype:
        :param threshold:

        in: p, o, ss
        out:
        """
        self._sys_call(self.__aread8 + self._p_args + self._o_args + ['-ad8', self._ssa] + ['-wg', self._ss],
                       intent_in=(self._p, self._o, self._ss),
                       intent_out=(self._ssa,))

        self._sys_call(self.__dropanalysis + self._p_args + self._fel_args +
                       self._ad8_args + self._ssa_args + self._drp_args +
                       self._o_args + ['-par', threshmin, threshmax, nthresh, steptype],
                       intent_in=(self._p, self._fel, self._ad8, self._o, self._ssa),
                       intent_out=(self._drp,))

        if threshold is None:
            threshold = self.drop_analysis_threshold

        self._run_threshold(self._ssa, self._pksrc, threshold=threshold)

    def run_streamnet(self, single_watershed=False):
        """
        in: fel, p, ad8, pksrc, o
        out: w, ord, tree, net, coors
        """
        self._sys_call(self.__streamnet + self._fel_args + self._p_args + self._ad8_args +
                       self._pksrc_args + self._o_args + self._ord_args + self._tree_args + self._net_args +
                       self._coord_args + self._w_args + ([], ['-sw'])[single_watershed],
                       intent_in=(self._fel, self._p, self._ad8, self._pksrc, self._o),
                       intent_out=(self._w, self._ord, self._tree, self._net, self._coord))

    def _run_gagewatershed(self, **kwargs):
        """
        in: p
        out: gw
        """
        long = kwargs.get('long', None)
        lat = kwargs.get('lat', None)
        easting = kwargs.get('easting', None)
        northing = kwargs.get('northing', None)
        dst = kwargs.get('dst', None)

        point = self._make_outlet(long=long, lat=lat, easting=easting, northing=northing, dst=dst[:-4] + '.geojson')
        self._sys_call(self.__gagewatershed + self._p_args + ['-o', point] + ['-gw', dst],
                       intent_in=(point, self._p),
                       intent_out=(dst,))

    def _create_prj(self, fname):
        """
        Create a PRJ for a topaz resource based on dem's projection
        """
        fname = fname.replace('.arc', '.prj')

        if _exists(fname):
            os.remove(fname)

        fid = open(fname, 'w')
        fid.write(self.srs_wkt)
        fid.close()

        return True

    def run_subcatchment_delineation(self, min_sub_area=None):
        cellsize = self.cellsize

        w_data = self.data_fetcher('w', dtype=np.int16)
        src_data = self.data_fetcher('pksrc', dtype=np.int16)

        subwta = np.zeros(w_data.shape, dtype=np.int16)

        s = [[0, 1, 0],
             [1, 1, 1],
             [0, 1, 0]]

        with open(self._net) as fp:
            js = json.load(fp)

        for feature in js['features']:
            chn_enum = feature['properties']['WSNO']
            coords = feature['geometry']['coordinates']
            uslinkn01 = feature['properties']['USLINKNO1']
            uslinkn02 = feature['properties']['USLINKNO2']
            end_node = uslinkn01 == -1 and uslinkn02 == -1

            first = coords[0]
            last = coords[-1]

            z_first = self.get_elevation(easting=first[0], northing=first[1])
            z_last = self.get_elevation(easting=last[0], northing=last[1])

            print(chn_enum, first, z_first, last, z_last, uslinkn01, uslinkn02, end_node)

            if z_first > z_last:
                top = first
            else:
                top = last

            # Identify the catchment by walking along the channel and sampling the watershed layer.
            # The sampling is needed because the channel could border on the diagonal of a catchment
            # and a single value could return the wrong catchment.
            catchment_id = Counter()
            for i in range(0, len(coords), 5):
                _e, _n, _ = coords[i]
                _x, _y = self.utm_to_px(easting=_e, northing=_n)
                catchment_id[w_data[_x, _y]] += 1
            catchment_id = catchment_id.most_common()[0][0]

            # need a mask for the side subcatchments
            catchment_data = np.zeros(w_data.shape, dtype=np.int16)
            catchment_data[np.where(w_data == catchment_id)] = 1

            if end_node:
                gw = _join(self.wd, 'wsno_%05i.tif' % chn_enum)
                self._run_gagewatershed(easting=top[0], northing=top[1], dst=gw)

                gw_data, _, _ = read_tif(gw)

                gw_indx = np.where(gw_data == 0)
                subwta[gw_indx] = int(str(catchment_id) + '1')

                # remove end subcatchment from the catchment mask
                catchment_data[gw_indx] = 0

                os.remove(gw)
                os.remove(gw[:-4] + '.geojson')

            # remove channels from catchment mask
            catchment_data -= src_data
            indx, indy = np.where(catchment_data == 1)

            # the whole catchment drains through the top of the channel
            if len(indx) == 0:
                continue

            x0, xend = np.min(indx), np.max(indx)

            if x0 > 0:
                x0 -= 1
            if xend < self.num_cols - 1:
                xend += 1

            y0, yend = np.min(indy), np.max(indy)

            if y0 > 0:
                y0 -= 1
            if yend < self.num_rows - 1:
                yend += 1

            _catchment_data = catchment_data[x0:xend, y0:yend]
            _catchment_data = np.clip(_catchment_data, a_min=0, a_max=1)
            # cv2.imwrite('%s.png' % str(catchment_id), np.array(255 * _catchment_data.T, dtype=np.uint8))

            subcatchment_data, n_labels = label(_catchment_data)

            for i in range(n_labels):
                indxx, indyy = np.where(subcatchment_data == i + 1)
                sub_area = len(indxx) * cellsize * cellsize
                if sub_area < min_sub_area:
                    continue

                subwta[x0:xend, y0:yend][indxx, indyy] = int(str(catchment_id) + str(i+2))

        subwta[np.where(w_data < 0)] = 0
        subwta = np.clip(subwta, 0, 2**15)
        subwta_fn = _join(self.wd, 'subwta.arc')
        write_arc(subwta, subwta_fn, self.ll_x, self.ll_y, self.cellsize)
        self._create_prj(subwta_fn)


if __name__ == "__main__":
    wd = _join(_thisdir, 'test')
    dem = _join(_thisdir, 'logan.tif')

    taudem = TauDEMRunner(wd=wd, dem=dem)
    taudem.run_pitremove()
    taudem.run_d8flowdir()
    taudem.run_aread8()
    taudem.run_gridnet()
    taudem.run_src_threshold()
    taudem.run_moveoutletstostrm(long=-111.784228758779406, lat=41.743629188805421)
    taudem.run_peukerdouglas()
    taudem.run_peukerdouglas_stream_delineation()
    taudem.run_streamnet()
    taudem._run_gagewatershed(long=-111.63609, lat=42.020262, dst=_join(wd, 'gw_test.tif'))
    taudem.run_subcatchment_delineation(min_sub_area=30*30*10)


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
system("mpiexec -n 8 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc1.tif -o outlet.shp -ord loganord.tif -tree logantree.txt -coord logancoord.txt -net logannet.shp -w loganw.tif")
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