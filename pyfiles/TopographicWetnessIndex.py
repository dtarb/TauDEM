# Script Name: SlopeOverAreaRatio
#
# Created By:  David Tarboton
# Date:        9/22/11

import arcpy
import Utils


# Input
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
sca = str(desc.catalogPath)
arcpy.AddMessage("\nInput Secific Catchment Area Grid: " + sca)

inlyr2 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr2)
slp = str(desc.catalogPath)
arcpy.AddMessage("Input Slope Grid: " + slp)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
twi = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Output Topographic Wetness Index Grid: " + twi)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' TWI -sca ' + '"' + sca + '"' + ' -slp ' + '"' + slp + '"' + ' -twi ' + \
      '"' + twi + '"'
arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'TWI failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
