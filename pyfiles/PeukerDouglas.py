# Script Name: PeukerDouglas
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
fel = str(desc.catalogPath)
arcpy.AddMessage("\nInput Elevation file: " + fel)

centerweight = arcpy.GetParameterAsText(1)
arcpy.AddMessage("Center Smoothing Weight: " + centerweight)

sideweight = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Side Smoothing Weight: " + sideweight)

diagonalweight = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Diagonal Smoothing Weight: " + diagonalweight)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
ss = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Stream Source file: " + ss)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' PeukerDouglas -fel ' + '"' + fel + '"' + ' -ss ' + '"' + ss + '"' + \
      ' -par ' + centerweight + ' ' + sideweight + ' ' + diagonalweight

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'PeukerDouglas failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
