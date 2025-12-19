# Script Name: D8DistanceToStreams
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
src = str(desc.catalogPath)
arcpy.AddMessage("Input Stream Raster Grid: " + src)

thresh = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Threshold: " + thresh)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
dist = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Output Distance To Streams: " + dist)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' D8HDistToStrm -p ' + '"' + p + '"' + ' -src ' + '"' + src + '"' + \
      ' -dist ' + '"' + dist + '"' + ' -thresh ' + thresh

arcpy.AddMessage("\nCommand Line: "+cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'D8HDistToStrm failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
