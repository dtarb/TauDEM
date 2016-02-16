__author__ = 'Pabitra'
"""
 Note: This script is a subset of the same script (StabilityIndex.py) in GRAIP-2 PythonTools
"""
import os
import sys
import subprocess
import argparse

from osgeo import ogr, gdal, osr

import Utils

# TODO: Need to find out how to catch gdal exceptions

gdal.UseExceptions()


class IntermediateFiles(object):
    weight_min_raster = 'weightmin.tif'
    weight_max_raster = 'weightmax.tif'
    sca_min_raster = 'scamin.tif'
    sca_max_raster = 'scamax.tif'
    si_control_file = 'Si_Control.txt'

# for the format of the input control file see python script file "ArcGISStabilityIndex.py"


def main():
    parser = argparse.ArgumentParser(description='Computes terrain stability index')
    parser.add_argument('--params', help='Input control text file that contains the values for needed parameters')
    args = parser.parse_args()
    params_dict = _get_initialized_parameters_dict()
    _validate_args(args.params, params_dict)

    base_raster_file = params_dict[ParameterNames.dinf_slope_file]
    if params_dict[ParameterNames.demang_file]:
        temp_raster_file_weight_min = os.path.join(params_dict[ParameterNames.temporary_output_files_directory],
                                                   IntermediateFiles.weight_min_raster)
        temp_raster_file_weight_max = os.path.join(params_dict[ParameterNames.temporary_output_files_directory],
                                                   IntermediateFiles.weight_max_raster)
        Utils.initialize_output_raster_file(base_raster_file, temp_raster_file_weight_min)
        Utils.initialize_output_raster_file(base_raster_file, temp_raster_file_weight_max)

        # generate catchment areas
        temp_raster_file_sca_min = os.path.join(params_dict[ParameterNames.temporary_output_files_directory],
                                                IntermediateFiles.sca_min_raster)
        temp_raster_file_sca_max = os.path.join(params_dict[ParameterNames.temporary_output_files_directory],
                                                IntermediateFiles.sca_max_raster)
        _taudem_area_dinf(temp_raster_file_weight_min, params_dict[ParameterNames.demang_file], temp_raster_file_sca_min)
        _taudem_area_dinf(temp_raster_file_weight_max, params_dict[ParameterNames.demang_file], temp_raster_file_sca_max)

    messages = _generate_combined_stability_index_grid(params_dict)
    for msg in messages:
        print(msg)

    if params_dict[ParameterNames.is_delete_intermediate_output_files] == 'True':
        _delete_intermediate_output_files(params_dict)

    print("done....")


def _get_initialized_parameters_dict():
    params = {}
    params[ParameterNames.dinf_slope_file] = None
    params[ParameterNames.demang_file] = None
    params[ParameterNames.dinf_sca_file] = None
    params[ParameterNames.cal_csv_file] = None
    params[ParameterNames.cal_grid_file] = None
    params[ParameterNames.csi_grid_file] = None
    params[ParameterNames.sat_grid_file] = None
    params[ParameterNames.min_terrain_recharge] = None
    params[ParameterNames.max_terrain_recharge] = None
    params[ParameterNames.gravity] = '9.81'
    params[ParameterNames.rhow] = '1000'
    params[ParameterNames.temporary_output_files_directory] = None
    params[ParameterNames.is_delete_intermediate_output_files] = 'True'
    return params


class ParameterNames(object):
    dinf_slope_file = 'slp'
    demang_file = 'ang'
    dinf_sca_file = 'sca'
    cal_csv_file = 'calpar'
    cal_grid_file = 'cal'
    csi_grid_file = 'si'
    sat_grid_file = 'sat'
    min_terrain_recharge = 'minimumterrainrecharge'
    max_terrain_recharge = 'maximumterrainrecharge'
    gravity = 'g'
    rhow = 'rhow'
    temporary_output_files_directory = 'temporary_output_files_directory'
    is_delete_intermediate_output_files = 'is_delete_intermediate_output_files'


def _validate_args(params, params_dict):
    driver = ogr.GetDriverByName(Utils.GDALFileDriver.ShapeFile)
    with open(params, 'r') as file_obj:
        for line in file_obj:
            line = line.strip(' ')
            line = line.strip('\n')
            if len(line) > 0:
                if not line.startswith('#'):
                    try:
                        key, value = line.split('=')
                        if key not in params_dict:
                            raise Utils.ValidationException("Invalid parameter name in the input file (%s)." % params)
                        else:
                            params_dict[key] = value.rstrip('\n')
                    except:
                        raise Utils.ValidationException("Input control file (%s) has invalid data format." % params)

    for key in params_dict:
        if not params_dict[key]:
            if not params_dict[ParameterNames.demang_file]:
                continue

        if not params_dict[key]:
            if key == ParameterNames.demang_file:
                raise Utils.ValidationException("Invalid input control file (%s). Value for one or more parameters is "
                                                "missing." % params)

        if key in (ParameterNames.min_terrain_recharge, ParameterNames.max_terrain_recharge,
                   ParameterNames.gravity, ParameterNames.rhow):
            try:
                float(params_dict[key])
            except:
                raise Utils.ValidationException("Invalid input control file (%s). Parameter (%s) needs to have a "
                                                "numeric value." % (params, key))

        # check that certain parameters that have file path values, that those file/directory exists
        if key in (ParameterNames.demang_file, ParameterNames.cal_csv_file,
                   ParameterNames.dinf_sca_file, ParameterNames.dinf_slope_file, ParameterNames.cal_grid_file):
            if key == ParameterNames.demang_file:
                if not params_dict[key]:
                    continue

            input_file = params_dict[key]
            if not os.path.dirname(input_file):
                input_file = os.path.join(os.getcwd(), params_dict[key])

            if not os.path.exists(input_file):
                raise Utils.ValidationException("Invalid input control file (%s). %s file/directory can't "
                                                "be found." % (params, params_dict[key]))

        # check that all other input grid files can be opened.
        if key in (ParameterNames.demang_file, ParameterNames.dinf_sca_file, ParameterNames.dinf_slope_file,
                   ParameterNames.cal_grid_file):

            input_file = params_dict[key]
            if not input_file:
                # check if it is one of the optional input files
                if key == ParameterNames.demang_file:
                    continue

            if not os.path.dirname(input_file):
                input_file = os.path.join(os.getcwd(), params_dict[key])
            try:
                dem = gdal.Open(input_file)
                dem = None
            except Exception as ex:
                raise Utils.ValidationException(ex.message)

        # check that the output grid file path exists
        if key in (ParameterNames.csi_grid_file, ParameterNames.sat_grid_file):
            if params_dict[key]:
                grid_file_dir = os.path.dirname(os.path.abspath(params_dict[key]))
                if not os.path.exists(grid_file_dir):
                    raise Utils.ValidationException("Invalid output file (%s). File path (%s) for grid output file "
                                                    "does not exist. Invalid parameter (%s) value."
                                                    % (params, grid_file_dir, key))

        if key == ParameterNames.is_delete_intermediate_output_files:
            if params_dict[ParameterNames.is_delete_intermediate_output_files] not in ('True', 'False'):
                raise Utils.ValidationException("Invalid input control file. Invalid value for parameter (%s). "
                                                "Parameter value should be either True or False ", key)

    # check that 'temporary_output_files_directory' is in fact a directory
    if not os.path.isdir(params_dict[ParameterNames.temporary_output_files_directory]):
        raise Utils.ValidationException("The specified temporary output files directory (%s) is not a directory." %
                                        params_dict[ParameterNames.temporary_output_files_directory])


def _taudem_area_dinf(weight_grid_file, demang_grid_file, output_sca_file):

    # mpiexec -n 4 Areadinf -ang demang.tif -wg demdpsi.tif -sca demsac.tif
    # taudem_funtion_to_run = 'mpiexec -n 4 Areadinf'
    taudem_function_to_run = 'Areadinf'
    cmd = taudem_function_to_run + \
          ' -ang ' + demang_grid_file + \
          ' -wg ' + weight_grid_file + \
          ' -sca ' + output_sca_file

    taudem_messages = []
    taudem_messages.append('Areadinf started:')
    taudem_messages.append(cmd)

    # Capture the contents of shell command and print it to the arcgis dialog box
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in process.stdout.readlines():
        taudem_messages.append(line.rstrip())
    return taudem_messages


def _generate_combined_stability_index_grid(params_dict):

    # TauDEMm SinmapSI calling format
    # mpiexec -n 4 SinmapSI -slp demslp.tif -sca demsca.tif -calpar demcalp.txt -cal demcal.tif -si demsi.tif -sat demsat.tif -par 0.0009 0.00135 9.81 1000 -scamin scamin.tif -scamax scamax.tif

    #taudem_function_to_run = r'E:\SoftwareProjects\TauDEM\Taudem5PCVS2010\x64\Release\SinmapSI'
    taudem_function_to_run = 'SinmapSI'

    cmd = taudem_function_to_run + \
          ' -slp ' + params_dict[ParameterNames.dinf_slope_file] + \
          ' -sca ' + params_dict[ParameterNames.dinf_sca_file] + \
          ' -calpar ' + params_dict[ParameterNames.cal_csv_file] + \
          ' -cal ' + params_dict[ParameterNames.cal_grid_file] + \
          ' -si ' + params_dict[ParameterNames.csi_grid_file] + \
          ' -sat ' + params_dict[ParameterNames.sat_grid_file] + \
          ' -par ' + params_dict[ParameterNames.min_terrain_recharge] + ' ' +\
          params_dict[ParameterNames.max_terrain_recharge] + ' ' + params_dict[ParameterNames.gravity] + ' ' + \
          params_dict[ParameterNames.rhow]

    if params_dict[ParameterNames.demang_file]:
        cmd += ' -scamin ' + os.path.join(params_dict[ParameterNames.temporary_output_files_directory],
                                          IntermediateFiles.sca_min_raster) + \
               ' -scamax ' + os.path.join(params_dict[ParameterNames.temporary_output_files_directory],
                                          IntermediateFiles.sca_max_raster)

    taudem_messages = []
    taudem_messages.append('SinmapSI started:')
    taudem_messages.append(cmd)
    # Capture the contents of shell command and print it to the arcgis dialog box
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in process.stdout.readlines():
        taudem_messages.append(line.rstrip())
    return taudem_messages


def _delete_intermediate_output_files(parm_dict):
    file_names_to_delete = [IntermediateFiles.weight_min_raster,
                            IntermediateFiles.weight_max_raster,
                            IntermediateFiles.sca_min_raster,
                            IntermediateFiles.sca_max_raster,
                            IntermediateFiles.si_control_file]

    for file_name in file_names_to_delete:
        file_to_delete = os.path.join(parm_dict[ParameterNames.temporary_output_files_directory], file_name)
        if os.path.isfile(file_to_delete):
            os.remove(file_to_delete)

if __name__ == '__main__':
    try:
        main()
        sys.exit(0)
    except Exception as e:
        print("Combined stability index computation failed.\n")
        print(e.message)
        sys.exit(1)
