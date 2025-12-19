# Script Name: SlopeAveDown
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
p = str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction Grid: " + p)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
fel = str(desc.catalogPath)
arcpy.AddMessage("Input Pit Filled Elevation Grid: " + fel)

distance = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Distance: " + distance)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
slpd = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Output Slope Average Down Grid: " + slpd)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' SlopeAveDown -p ' + '"' + p + '"' + ' -fel ' + '"' + fel + '"' + \
      ' -slpd ' + '"' + slpd + '"' + ' -dn ' + distance

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'SlopeAveDown failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
