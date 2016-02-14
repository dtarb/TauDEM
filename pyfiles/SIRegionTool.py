__author__ = 'Pabitra'

import os
import sys
import argparse

from osgeo import ogr, gdal, osr
from gdalconst import *

import arcpy

import Utils

gdal.UseExceptions()

# Check out any necessary licenses
arcpy.CheckOutExtension("spatial")

"""
Inputs for testing:
--dem = r"E:\Graip\GRAIPPythonTools\demo\demo_Calibration_Region_Tool\demfel"
--parreg = r"E:\Graip\GRAIPPythonTools\demo\demo_Calibration_Region_Tool\parreg_out_from_esri_demfel_1.tif"
--att = r"E:\Graip\GRAIPPythonTools\demo\demo_Calibration_Region_Tool\demcalp_from_esri_demfel_1.txt"

One of the following options can be used:
Option-1: using an input raster region grid
--parreg-in = r"E:\Graip\GRAIPPythonTools\demo\demo_Calibration_Region_Tool\parreg_out_shp_3.tif"

Option-2 : using input feature class
--shp = r"E:\Graip\GRAIPPythonTools\demo\demo_Calibration_Region_Tool\RegionShape.shp"
--shp-att-name = 'ID'

To see the help on command line parameters for this script, type the following in a terminal:
 $ python CalibrationTool.py -h
"""


def main():
    parser = argparse.ArgumentParser(description='Creates SI region grid')
    parser.add_argument('--dem', help='Input dem raster')
    parser.add_argument('--parreg', help='Output parameter region grid raster')
    parser.add_argument('--att', help='Output calibration table text file')
    parser.add_argument('--parreg-in', default=None, help='Input region dataset')
    parser.add_argument('--shp', default=None, help='Input region feature class')
    parser.add_argument('--shp-att-name', default='ID', help='Input feature class attribute')
    parser.add_argument('--att-tmin', default=2.708, help='Transmissivity lower bound (m^2/hr)')
    parser.add_argument('--att-tmax', default=2.708, help='Transmissivity upper bound (m^2/hr)')
    parser.add_argument('--att-cmin', default=0.0, help='Dimensionless cohesion lower bound')
    parser.add_argument('--att-cmax', default=0.25, help='Dimensionless cohesion upper bound')
    parser.add_argument('--att-phimin', default=30.0, help='Soil friction angle lower bound')
    parser.add_argument('--att-phimax', default=45.0, help='Soil friction angle upper bound')
    parser.add_argument('--att-soildens', default=2000.0, help='Soil density (kg/m^3)')
    args = parser.parse_args()
    if args.shp_att_name:
        args.shp_att_name = args.shp_att_name.encode('ascii', 'ignore')

    _validate_args(args.dem, args.parreg_in, args.shp, args.shp_att_name, args.parreg, args.att)
    _create_parameter_region_grid(args.dem, args.shp, args.shp_att_name, args.parreg_in, args.parreg)
    _create_parameter_attribute_table_text_file(args.parreg, args.parreg_in, args.shp, args.att, args.att_tmin,
                                                args.att_tmax, args.att_cmin, args.att_cmax,
                                                args.att_phimin, args.att_phimax, args.att_soildens)


def _validate_args(dem, parreg_in, shp, shp_att_name, parreg, att):
    dataSource = gdal.Open(dem, GA_ReadOnly)
    if not dataSource:
        raise Utils.ValidationException("File open error. Not a valid file (%s) provided for the '--dem' parameter."
                                        % dem)
    else:
        dataSource = None

    if parreg_in:
        dataSource = gdal.Open(parreg_in, 1)
        if not dataSource:
            raise Utils.ValidationException("File open error. Not a valid file (%s) provided for the '--parreg-in' "
                                            "parameter." % parreg_in)
        else:
            # check the provided region grid file has only integer data
            parreg_in_band = dataSource.GetRasterBand(1)
            if parreg_in_band.DataType != gdal.GDT_UInt32 and parreg_in_band.DataType != gdal.GDT_UInt16 and \
                            parreg_in_band.DataType != gdal.GDT_Byte:
                dataSource = None
                raise Utils.ValidationException("Not a valid file (%s) provided for the '--parreg-in' "
                                                "parameter. Data type must be integer." % parreg_in)

            else:
                dataSource = None

    if parreg_in and shp:
        raise Utils.ValidationException("Either a value for the parameter '--parreg-in' or '--shp' should be "
                                        "provided. But not both.")

    if shp:
        driver_shp = ogr.GetDriverByName(Utils.GDALFileDriver.ShapeFile)
        dataSource = driver_shp.Open(shp, 1)
        if not dataSource:
            raise Utils.ValidationException("Not a valid shape file (%s) provided for the '--shp' parameter." % shp)
        else:
            if shp_att_name is None:
                raise Utils.ValidationException("Name for the attribute from the shapefile to be used for creating "
                                                "region grid is missing. ('--shp-att-name' parameter).")

            else:
                # Attribute name can't be 'FID" since one of the FID values is always zero
                # and zero is a no-data value in the grid that we generate from the shape file
                if shp_att_name == 'FID':
                    raise Utils.ValidationException("'FID' is an invalid shape file attribute for calibration "
                                                    "region calculation.")

                # check the provided attribute name is valid for the shapefile
                layer = dataSource.GetLayer()
                layer_defn = layer.GetLayerDefn()
                try:
                    layer_defn.GetFieldIndex(shp_att_name)
                except:
                    dataSource.Destroy()
                    raise Utils.ValidationException("Invalid shapefile. Attribute '%s' is missing." % shp_att_name)

            dataSource.Destroy()

    parreg_dir = os.path.dirname(os.path.abspath(parreg))
    if not os.path.exists(parreg_dir):
        raise Utils.ValidationException("File path '(%s)' for tif output file (parameter '--parreg') does not exist."
                                        % parreg_dir)

    att_dir = os.path.dirname(os.path.abspath(att))
    if not os.path.exists(att_dir):
        raise Utils.ValidationException("File path '(%s)' for output parameter attribute table (parameter '--att') "
                                        "does not exist." % att_dir)

    #TODO: check the extension of the parreg grid file is 'tif'


def _create_parameter_region_grid(dem, shp, shp_att_name, parreg_in, parreg):
    if os.path.exists(parreg):
            os.remove(parreg)

    if shp is None and parreg_in is None:
        Utils.initialize_output_raster_file(dem, parreg, initial_data=1, data_type=gdal.GDT_UInt32)
    else:
        # determine cell size from the dem
        base_raster_file_obj = gdal.Open(dem, GA_ReadOnly)
        geotransform = base_raster_file_obj.GetGeoTransform()
        pixelWidth = geotransform[1]

        if shp:

            # This environment settings needed  for the arcpy.PlogonToRaster_conversion function
            #env.extent = dem
            #env.snapRaster = dem
            #print (">>> setting the environment for polygon to raster conversion")
            # TODO: try if we can use the gdal api to convert shape file to raster instead of arcpy
            # Ref: https://pcjericks.github.io/py-gdalogr-cookbook/raster_layers.html
            #utils.initialize_output_raster_file(dem, parreg, initial_data=1, data_type=gdal.GDT_Int32)
            #target_ds = gdal.Open(parreg)

            # For some reason the gdal RasterizeLayer function works with a in memory output raster dataset only
            target_ds = _create_in_memory_raster(dem, data_type=gdal.GDT_UInt32)
            source_ds = ogr.Open(shp)
            source_layer = source_ds.GetLayer()
            # Rasterize
            err = gdal.RasterizeLayer(target_ds, [1], source_layer, options=["ATTRIBUTE=%s" % shp_att_name])
            if err != 0:
                raise Exception(err)
            else:
                # save the in memory output raster to the disk
                gdal.GetDriverByName(Utils.GDALFileDriver.TifFile).CreateCopy(parreg, target_ds)

            target_ds = None
            source_ds = None

            # arcpy.PolygonToRaster_conversion(shp, shp_att_name, temp_shp_raster, "CELL_CENTER", "NONE", str(pixelWidth))
            #arcpy.ResetEnvironments()

        elif parreg_in:
            # TODO: This one only gets the grid cell size to the size in dem file
            # but it doesn't get the output grid size (rows and cols) same as the dem
            temp_parreg = os.path.join(os.path.dirname(dem), 'temp_parreg.tif')
            if os.path.exists(temp_parreg):
                arcpy.Delete_management(temp_parreg)
                #os.remove(temp_parreg)
            target_ds = _create_in_memory_raster(dem, data_type=gdal.GDT_UInt32)
            #utils.initialize_output_raster_file(dem, parreg, initial_data=utils.NO_DATA_VALUE, data_type=gdal.GDT_UInt32)
            #arcpy.Resample_management(parreg_in, parreg, str(pixelWidth), "NEAREST")
            arcpy.Resample_management(parreg_in, temp_parreg, str(pixelWidth), "NEAREST")
            # save the in memory output raster to the disk
            source_ds = gdal.Open(temp_parreg, GA_ReadOnly)
            gdal.ReprojectImage(source_ds, target_ds, None, None, gdal.GRA_NearestNeighbour)
            source_ds = None
            gdal.GetDriverByName(Utils.GDALFileDriver.TifFile).CreateCopy(parreg, target_ds)
            arcpy.Delete_management(temp_parreg)


def _create_in_memory_raster(base_raster, data_type):
    # TODO: document this method

    base_raster = gdal.Open(base_raster, GA_ReadOnly)
    geotransform = base_raster.GetGeoTransform()
    originX = geotransform[0]
    originY = geotransform[3]
    pixelWidth = geotransform[1]
    pixelHeight = geotransform[5]
    rows = base_raster.RasterYSize
    cols = base_raster.RasterXSize

    driver = gdal.GetDriverByName('MEM')
    number_of_bands = 1
    outRaster = driver.Create('', cols, rows, number_of_bands, data_type)
    outRaster.SetGeoTransform((originX, pixelWidth, 0, originY, 0, pixelHeight))

    outband = outRaster.GetRasterBand(1)
    outband.SetNoDataValue(0)

    # set the projection of the tif file same as that of the base_raster file
    outRasterSRS = osr.SpatialReference()
    outRasterSRS.ImportFromWkt(base_raster.GetProjectionRef())
    outRaster.SetProjection(outRasterSRS.ExportToWkt())
    return outRaster


def _create_parameter_attribute_table_text_file(parreg, parreg_in, shp, att, att_tmin, att_tmax, att_cmin, att_cmax,
                                                att_phimin, att_phimax, att_soildens):

    param_ids = []
    if shp or parreg_in:
        # read the parameter grid file to create a list of unique grid cell values
        # which can then be used to populate the SiID column of the parameter text file
        parreg_raster = gdal.Open(parreg)
        parreg_band = parreg_raster.GetRasterBand(1)
        no_data = parreg_band.GetNoDataValue()

        for row in range(0, parreg_band.YSize):
            current_row_data_array = parreg_band.ReadAsArray(xoff=0, yoff=row, win_xsize=parreg_band.XSize, win_ysize=1)
            # get a list of unique data values in the current row
            current_row_unique_data = set(current_row_data_array[0])
            for data in current_row_unique_data:
                if data != no_data:
                    if data not in param_ids:
                        param_ids.append(data)

        param_ids.sort()
    else:
        param_ids.append(1)

    with open(att, 'w') as file_obj:
        file_obj.write('SiID,tmin,tmax,cmin,cmax,phimin,phimax,SoilDens\n')
        for id in param_ids:
            file_obj.write(str(id) + ',' + str(att_tmin) + ',' + str(att_tmax) + ',' + str(att_cmin) + ',' +
                           str(att_cmax) + ',' + str(att_phimin) + ',' + str(att_phimax) + ',' +
                           str(att_soildens) + '\n')


if __name__ == '__main__':
    try:
        main()
        print("Region computation successful.")
    except Exception as e:
        print "Region computation failed."
        print(e.message)
        sys.exit(1)
