# Script Name: SlopeAreaCombination
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
slp = str(desc.catalogPath)
arcpy.AddMessage("\nInput Slope Grid: " + slp)

inlyr = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr)
sca = str(desc.catalogPath)
arcpy.AddMessage("Input Area Grid: " + sca)

slopeexponent = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Slope Exponent(m): " + slopeexponent)

areaexponent = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Area Exponent(n): " + areaexponent)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
sa = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Slope Area Grid: " + sa)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' SlopeArea -slp ' + '"' + slp + '"' + ' -sca ' + '"' + sca + '"' + \
      ' -sa ' + '"' + sa + '"' + ' -par ' + slopeexponent + ' ' + areaexponent

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'SlopeArea failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
