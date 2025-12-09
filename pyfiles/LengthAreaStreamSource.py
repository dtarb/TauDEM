# Script Name: LengthAreaStreamSource
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
plen = str(desc.catalogPath)
arcpy.AddMessage("\nInput Length Grid: " + plen)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
ad8 = str(desc.catalogPath)
arcpy.AddMessage("Input Contributing Area Grid: " + ad8)

threshold = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Threshold(M): " + threshold)

exponent = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Exponent(y): " + exponent)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
ss = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Stream Source Grid: "+ss)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' LengthArea -plen ' + '"' + plen + '"' + ' -ad8 ' + '"' + ad8 +\
      '"' + ' -ss ' + '"' + ss + '"' + ' -par ' + threshold + ' ' + exponent

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'LengthArea failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
