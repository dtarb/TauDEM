# Script Name: SlopeOverAreaRatio
#
# Created By:  David Tarboton
# Date:        9/22/11

import arcpy
import Utils

# Input
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
slp = str(desc.catalogPath)
arcpy.AddMessage("\nInput Slope Grid: " + slp)

inlyr2 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr2)
sca = str(desc.catalogPath)
arcpy.AddMessage("Input Secific Catchment Area Grid: " + sca)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(2)
arcpy.AddMessage(" Number of Processes: " + inputProc)

# Outputs
sar = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Output Slope Divided By Area Ratio Grid: " + sar)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' SlopeAreaRatio -slp ' + '"' + slp + '"' + ' -sca ' + '"' + sca + '"' + \
      ' -sar ' + '"' + sar + '"'
arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'SlopeAreaRatio failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
