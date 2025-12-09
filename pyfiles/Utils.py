__author__ = 'Pabitra'

from collections import namedtuple
import os
import subprocess

from osgeo import gdal, osr
from osgeo.gdalconst import GA_ReadOnly
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


def get_adjusted_env():
    # Modify environment to remove ArcGIS paths from PATH to prevent GDAL version conflicts
    # This ensures TauDEM tools like pitremove use the system/TauDEM GDAL instead of ArcGIS's GDAL
    env = os.environ.copy()
    if 'PATH' in env:
        path_dirs = env['PATH'].split(os.pathsep)
        # Filter out any paths containing 'ArcGIS' (case-insensitive)
        clean_path_dirs = [p for p in path_dirs if 'arcgis' not in p.lower()]
        env['PATH'] = os.pathsep.join(clean_path_dirs)

    # Remove GDAL_DATA, PROJ_LIB, and GDAL_DRIVER_PATH if they refer to ArcGIS paths
    # This prevents version mismatch errors where TauDEM's GDAL tries to read ArcGIS's GDAL data files or plugins
    for var in ['GDAL_DATA', 'PROJ_LIB', 'GDAL_DRIVER_PATH']:
        if var in env and 'arcgis' in env[var].lower():
            del env[var]

    return env


def run_taudem_command(cmd, msg_callback=None):
    """
    Runs a TauDEM command using subprocess, handling environment variables and output filtering.

    :param cmd: The command string to execute.
    :param msg_callback: A function to call with output messages (e.g., arcpy.AddMessage).
    :return: The return code of the process.
    """
    env = get_adjusted_env()
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
    stdout, stderr = process.communicate()

    if stdout and msg_callback:
        for line in stdout.splitlines():
            msg_callback(line.rstrip())
            msg_callback("\n")

    if stderr:
        filtered_stderr = []
        patterns = [
            "This run may take on the order of",
            "This estimate is very approximate",
            "Run time is highly uncertain",
            "speed and memory of the computer",
            "dual quad core Dell Xeon"
        ]
        for line in stderr.strip().split('\n'):
            if not line.strip():
                continue
            if any(pattern in line for pattern in patterns):
                # this is not really an error, so don't include it in filtered_stderr
                if msg_callback:
                    msg_callback(line)
                    msg_callback("\n")
                continue
            filtered_stderr.append(line)

        if filtered_stderr and msg_callback:
            msg_callback('\nERROR OUTPUT:')
            for line in filtered_stderr:
                msg_callback(line)
                msg_callback("\n")

    return process.returncode
