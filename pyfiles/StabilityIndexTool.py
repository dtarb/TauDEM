import arcpy

import Utils

# get the input parameters
slp_raster_file = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(slp_raster_file)
slp_raster_file = str(desc.catalogPath)
arcpy.AddMessage("\nInput Slope raster file: " + slp_raster_file)

sca_raster_file = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(sca_raster_file)
sca_raster_file = str(desc.catalogPath)
arcpy.AddMessage("Input SCA raster file: " + sca_raster_file)

cal_raster_file = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(cal_raster_file)
cal_raster_file = str(desc.catalogPath)
arcpy.AddMessage("Input CAL raster file: " + cal_raster_file)

capl_text_file = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Input CAPL text file: " + capl_text_file)

min_terr_recharge = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Input Minimum Terrain Recharge: " + min_terr_recharge)

max_terr_recharge = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Input Maximum Terrain Recharge: " + max_terr_recharge)

si_raster_file = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Output Stability Index raster file: " + si_raster_file)

sat_raster_file = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Output Saturation raster file: " + sat_raster_file)

temp_output_files_directory = arcpy.GetParameterAsText(8)
is_delete_intermediate_output_files = arcpy.GetParameterAsText(9)

input_proc = arcpy.GetParameterAsText(10)
arcpy.AddMessage('Number of Processes: ' + input_proc)
# Note: temp_output_files_directory and is_delete_intermediate_output_files are retained
# as toolbox parameters for backward compatibility, but are not used when calling
# sinmapsi directly (no intermediate files are generated in this wrapper).
arcpy.AddMessage('\nStarting Stability Index computation...')
try:
    cmd = 'mpiexec -n ' + input_proc + ' SinmapSI' + \
          ' -slp ' + Utils.quote(slp_raster_file) + \
          ' -sca ' + Utils.quote(sca_raster_file) + \
          ' -calpar ' + Utils.quote(capl_text_file) + \
          ' -cal ' + Utils.quote(cal_raster_file) + \
          ' -si ' + Utils.quote(si_raster_file) + \
          ' -sat ' + Utils.quote(sat_raster_file) + \
          ' -par ' + str(min_terr_recharge) + ' ' + str(max_terr_recharge) + ' 9.81 1000'
    arcpy.AddMessage('\nCommand Line: ' + cmd)

    return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)
    if return_code != 0:
        raise Exception('SinmapSI failed with return code ' + str(return_code))

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
