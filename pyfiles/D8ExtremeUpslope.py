# Script Name: D8ExtremeUpslope
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import os
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
p = str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction Grid: " + p)

inlyr2 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr2)
sa = str(desc.catalogPath)
arcpy.AddMessage("Input Value Grid: " + sa)

maximumupslope = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Maximum Upslope: " + maximumupslope)

edgecontamination = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Edge Contamination: " + edgecontamination)

ogrfile=arcpy.GetParameterAsText(4)
if arcpy.Exists(ogrfile):
    desc = arcpy.Describe(ogrfile)
    shfl1 = str(desc.catalogPath)
    extn = os.path.splitext(shfl1)[1]   # get extension of a file

    # if extention is shapfile do not convert into gjson other wise convert
    if extn == ".shp":
       shfl = shfl1
    else:
      arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
      basename = os.path.basename(shfl1)    # get last part of the path
      dirname = os.path.dirname(p)  # get directory
      arcpy.env.workspace = dirname     # does not work without specifying the workspace
      arcpy.FeaturesToJSON_conversion(shfl1,basename + ".json")   # convert feature to json
      shfl = os.path.join(dirname,basename + ".json")
    arcpy.AddMessage("Using Outlets file: " + shfl)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
ssa = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Output Extreme Value Grid: " + ssa)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' D8FlowPathExtremeUp -p ' + '"' + p + '"' + ' -sa ' + '"' + sa + '"' + \
      ' -ssa ' + '"' + ssa + '"'
if arcpy.Exists(ogrfile):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if maximumupslope == 'false':
    cmd = cmd + ' -min '
if edgecontamination == 'false':
    cmd = cmd + ' -nc '

arcpy.AddMessage("\nCommand Line: "+cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# remove converted json file
extn_json = os.path.splitext(shfl)[1]     # get extension of the converted json file
if extn_json == ".json":
    os.remove(shfl)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'D8FlowPathExtremeUp failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
