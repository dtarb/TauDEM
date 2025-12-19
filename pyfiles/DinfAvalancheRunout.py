# Script Name: DinfAvalancheRunout
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
fel = str(desc.catalogPath)
arcpy.AddMessage("\nInput Pit Filled Elevation Grid: " + fel)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
ang = str(desc.catalogPath)
arcpy.AddMessage("Input D-Infinity Flow Direction Grid: " + ang)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
ass = str(desc.catalogPath)
arcpy.AddMessage("Input Avalanche Source Site Grid: " + ass)

propthresh = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Input Proportion Threshold: " + propthresh)

alphthresh = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Input Alpha Angle Threshold: " + alphthresh)

pathdistance = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Path Distance Method: " + pathdistance)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
rz = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Output Runout Zone Grid: " + rz)

dfs = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Output Path Distance Grid: " + dfs)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' DinfAvalanche -fel ' + '"' + fel + '"' + ' -ang ' + '"' + ang + '"' + \
      ' -ass ' + '"' + ass + '"' + ' -rz ' + '"' + rz + '"' + ' -dfs ' + '"' + dfs + '"' + ' -thresh ' + \
      propthresh + ' -alpha ' + alphthresh
if pathdistance == 'Straight Line':
    cmd = cmd + ' -direct '

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'DinfAvalancheRunout failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
