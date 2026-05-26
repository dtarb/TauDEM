# Script Name: InunDepth
#
# Created By:  TauDEM Team
# Date:        2/18/2026

import arcpy
import Utils

# Inputs
in_lyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(in_lyr)
hand = str(desc.catalogPath)
arcpy.AddMessage("\nInput HAND Grid: " + hand)

in_lyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(in_lyr1)
catch = str(desc.catalogPath)
arcpy.AddMessage("Input Catchment COMID Grid: " + catch)

mask_lyr = arcpy.GetParameterAsText(2)
if arcpy.Exists(mask_lyr):
    desc = arcpy.Describe(mask_lyr)
    mask = str(desc.catalogPath)
    arcpy.AddMessage("Input Mask Grid: " + mask)

forecast_file = arcpy.GetParameterAsText(3)
if arcpy.Exists(forecast_file):
    desc = arcpy.Describe(forecast_file)
    fc = str(desc.catalogPath)
else:
    fc = forecast_file
arcpy.AddMessage("Input Forecast CSV File: " + fc)

hydroprop_file = arcpy.GetParameterAsText(4)
if arcpy.Exists(hydroprop_file):
    desc = arcpy.Describe(hydroprop_file)
    hp = str(desc.catalogPath)
else:
    hp = hydroprop_file
arcpy.AddMessage("Input Hydro Property CSV File: " + hp)

# Outputs
inun_out = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Inundation Grid: " + inun_out)

depth_out = arcpy.GetParameterAsText(6)
if depth_out:
    arcpy.AddMessage("Output Inundation Depth CSV File: " + depth_out)

# Input Number of Processes
input_proc = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Number of Processes: " + input_proc)

# Construct command
cmd = 'mpiexec -n ' + input_proc + ' inundepth -hand ' + '"' + hand + '"' + ' -catch ' + '"' + catch + '"' + \
      ' -fc ' + '"' + fc + '"' + ' -hp ' + '"' + hp + '"' + ' -inun ' + '"' + inun_out + '"'
if arcpy.Exists(mask_lyr):
    cmd = cmd + ' -mask ' + '"' + mask + '"'
if depth_out:
    cmd = cmd + ' -depth ' + '"' + depth_out + '"'

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'InunDepth failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
