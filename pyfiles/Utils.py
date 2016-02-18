__author__ = 'Pabitra'

from collections import namedtuple
from osgeo import gdal, osr
from gdalconst import *
import numpy as np

NO_DATA_VALUE = -9999


class ValidationException(Exception):
    pass


FileDriver = namedtuple('FileDriver', 'ShapeFile, TifFile')
GDALFileDriver = FileDriver("ESRI Shapefile", "GTiff")


def initialize_output_raster_file(base_raster_file, output_raster_file, initial_data=0.0, data_type=gdal.GDT_Float32):

    """
    Creates an raster file based on the dimension, projection, and cell size of an input raster file using specified
    initial data value of specified data type

    :param base_raster_file: raster file based on which the new raster file to be created with initial_data
    :param output_raster_file: name and location of of the output raster file to be created
    :param initial_data: data to be used in creating the output raster file
    :param data_type: GDAL data type to be used in creating the output raster file
    :return: None
    """
    base_raster = gdal.Open(base_raster_file, GA_ReadOnly)
    geotransform = base_raster.GetGeoTransform()
    originX = geotransform[0]
    originY = geotransform[3]
    pixelWidth = geotransform[1]
    pixelHeight = geotransform[5]
    rows = base_raster.RasterYSize
    cols = base_raster.RasterXSize

    driver = gdal.GetDriverByName(GDALFileDriver.TifFile)
    number_of_bands = 1
    outRaster = driver.Create(output_raster_file, cols, rows, number_of_bands, data_type)
    outRaster.SetGeoTransform((originX, pixelWidth, 0, originY, 0, pixelHeight))

    # initialize the newly created tif file with zeros
    if data_type == gdal.GDT_Float32:
        grid_initial_data = np.zeros((rows, cols), dtype=np.float32)
        grid_initial_data[:] = float(initial_data)
    else:
        grid_initial_data = np.zeros((rows, cols), dtype=np.int)
        grid_initial_data[:] = int(initial_data)

    outband = outRaster.GetRasterBand(1)
    outband.SetNoDataValue(NO_DATA_VALUE)
    outband.WriteArray(grid_initial_data)

    # set the projection of the tif file same as that of the base_raster file
    outRasterSRS = osr.SpatialReference()
    outRasterSRS.ImportFromWkt(base_raster.GetProjectionRef())
    outRaster.SetProjection(outRasterSRS.ExportToWkt())

    outRaster = None