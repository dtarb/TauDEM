# Created by: Pabitra Dash

import os

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

    cmd = 'stabilityindex --params ' + Utils.quote(si_control_file)
    arcpy.AddMessage('\nCommand Line: ' + cmd)

    return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)
    if return_code != 0:
        raise Exception('stabilityindex failed with return code ' + str(return_code))

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
