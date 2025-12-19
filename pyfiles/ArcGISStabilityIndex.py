# Created by: Pabitra Dash

import os
import sys

import arcpy

import Utils

# get the input parameters
slp_raster_file = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(slp_raster_file)
slp_raster_file = str(desc.catalogPath)

sca_raster_file = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(sca_raster_file)
sca_raster_file = str(desc.catalogPath)

cal_raster_file = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(cal_raster_file)
cal_raster_file = str(desc.catalogPath)

capl_text_file = arcpy.GetParameterAsText(3)
min_terr_recharge = arcpy.GetParameterAsText(4)
max_terr_recharge = arcpy.GetParameterAsText(5)
si_raster_file = arcpy.GetParameterAsText(6)
sat_raster_file = arcpy.GetParameterAsText(7)
temp_output_files_directory = arcpy.GetParameterAsText(8)
is_delete_intermediate_output_files = arcpy.GetParameterAsText(9)

# create the cis_inputs.txt file from the provided above parameters

# Add the current script directory to sys.path to allow importing StabilityIndex
this_script_dir = os.path.dirname(os.path.realpath(__file__))
if this_script_dir not in sys.path:
    sys.path.insert(0, this_script_dir)

# Import the StabilityIndex module
import StabilityIndex

si_control_file = os.path.join(temp_output_files_directory, 'Si_Control.txt')

with open(si_control_file, 'w') as file_obj:
    file_obj.write('# input parameters for combined stability index computation with road impact\n')
    file_obj.write('# input files\n')
    file_obj.write('slp=' + slp_raster_file + '\n')
    file_obj.write('sca=' + sca_raster_file + '\n')
    file_obj.write('cal=' + cal_raster_file + '\n')
    file_obj.write('calpar=' + capl_text_file + '\n')

    file_obj.write('# output files\n')
    file_obj.write('si=' + si_raster_file + '\n')
    file_obj.write('sat=' + sat_raster_file + '\n')

    file_obj.write('# Additional parameters' + '\n')
    file_obj.write('minimumterrainrecharge=' + min_terr_recharge + '\n')
    file_obj.write('maximumterrainrecharge=' + max_terr_recharge + '\n')
    file_obj.write('# temporary output file directory\n')
    file_obj.write('temporary_output_files_directory=' + temp_output_files_directory + '\n')
    if str(is_delete_intermediate_output_files) == 'true':
        file_obj.write('is_delete_intermediate_output_files=True\n')
    else:
        file_obj.write('is_delete_intermediate_output_files=False\n')

try:
    arcpy.AddMessage('\nStarting Stability Index computation...')

    # Initialize parameters dictionary
    params_dict = StabilityIndex._get_initialized_parameters_dict()

    # Validate and populate parameters from control file
    StabilityIndex._validate_args(si_control_file, params_dict)

    # Process base raster file for weight grids if demang_file is provided
    base_raster_file = params_dict[StabilityIndex.ParameterNames.dinf_slope_file]
    if params_dict[StabilityIndex.ParameterNames.demang_file]:
        temp_raster_file_weight_min = os.path.join(
            params_dict[StabilityIndex.ParameterNames.temporary_output_files_directory],
            StabilityIndex.IntermediateFiles.weight_min_raster
        )
        temp_raster_file_weight_max = os.path.join(
            params_dict[StabilityIndex.ParameterNames.temporary_output_files_directory],
            StabilityIndex.IntermediateFiles.weight_max_raster
        )
        Utils.initialize_output_raster_file(base_raster_file, temp_raster_file_weight_min)
        Utils.initialize_output_raster_file(base_raster_file, temp_raster_file_weight_max)

        # Generate catchment areas
        temp_raster_file_sca_min = os.path.join(
            params_dict[StabilityIndex.ParameterNames.temporary_output_files_directory],
            StabilityIndex.IntermediateFiles.sca_min_raster
        )
        temp_raster_file_sca_max = os.path.join(
            params_dict[StabilityIndex.ParameterNames.temporary_output_files_directory],
            StabilityIndex.IntermediateFiles.sca_max_raster
        )

        messages = StabilityIndex._taudem_area_dinf(
            temp_raster_file_weight_min,
            params_dict[StabilityIndex.ParameterNames.demang_file],
            temp_raster_file_sca_min
        )
        for msg in messages:
            arcpy.AddMessage(msg)

        messages = StabilityIndex._taudem_area_dinf(
            temp_raster_file_weight_max,
            params_dict[StabilityIndex.ParameterNames.demang_file],
            temp_raster_file_sca_max
        )
        for msg in messages:
            arcpy.AddMessage(msg)

    # Generate combined stability index grid
    messages = StabilityIndex._generate_combined_stability_index_grid(params_dict)
    for msg in messages:
        arcpy.AddMessage(msg)

    # Delete intermediate files if requested
    if params_dict[StabilityIndex.ParameterNames.is_delete_intermediate_output_files] == 'True':
        StabilityIndex._delete_intermediate_output_files(params_dict)

    # Only show success message if we reach here without exceptions
    arcpy.AddMessage('\nStability Index computation completed successfully.')

except Exception as e:
    arcpy.AddError('\n' + '='*50)
    arcpy.AddError('STABILITY INDEX COMPUTATION FAILED')
    arcpy.AddError('='*50)
    arcpy.AddError(str(e))
    import traceback
    arcpy.AddError('\nFull traceback:')
    arcpy.AddError(traceback.format_exc())
    # let ArcGIS know the execution failed
    raise
