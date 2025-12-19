# Script Name: DinfFlowDirection
#
# Created By:  David Tarboton
# Date:        9/23/11

import arcpy
import Utils

# Input
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
fel = str(desc.catalogPath)
arcpy.AddMessage("\nInput Pit Filled Elevation file: " + fel)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(1)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
ang = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Output Dinf Flow Direction File: " + ang)
slp = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Output Dinf Slope File: " + slp)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' DinfFlowDir -fel ' + '"' + fel + '"' + ' -ang ' + '"' + ang + '"' + \
      ' -slp ' + '"' + slp + '"'
arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'DinfFlowDir failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
