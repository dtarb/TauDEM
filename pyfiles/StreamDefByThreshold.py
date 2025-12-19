# Script Name: StreamDefByThreshold
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils


# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
ssa = str(desc.catalogPath)
arcpy.AddMessage("\nInput Accumulated Stream Source Grid: " + ssa)

maskgrid = arcpy.GetParameterAsText(1)
if arcpy.Exists(maskgrid):
    desc = arcpy.Describe(maskgrid)
    mask = str(desc.catalogPath)
    arcpy.AddMessage("Input Mask Grid: " + mask)

threshold = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Threshold: " + threshold)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
src = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Output Stream Raster Grid: " + src)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' Threshold -ssa ' + '"' + ssa + '"' + ' -src ' + '"' + src + '"' + \
      ' -thresh ' + threshold
if arcpy.Exists(maskgrid):
    cmd = cmd + ' -mask ' + mask

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'Threshold failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
