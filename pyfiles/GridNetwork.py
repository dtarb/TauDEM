# Script Name: GridNetwork
#
# Created By:  David Tarboton
# Date:        9/28/11

import arcpy
import os
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
p = str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction file: " + p)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(1)
arcpy.AddMessage("Number of Processes: " + inputProc)

ogrfile = arcpy.GetParameterAsText(2)
if arcpy.Exists(ogrfile):
    desc = arcpy.Describe(ogrfile)
    shfl1 = str(desc.catalogPath)
    extn = os.path.splitext(shfl1)[1]   # get extension of a file
    # if extention is shapfile do not convert into gjson other wise convert
    if extn == ".shp":
       shfl = shfl1
    else:
        arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
        basename = os.path.basename(shfl1)  # get last part of the path
        dirname = os.path.dirname(p)  # get directory
        arcpy.env.workspace = dirname   # does not work without specifying the workspace
        arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")    # convert feature to json
        shfl = os.path.join(dirname, basename + ".json")
    arcpy.AddMessage("Using Outlets file: " + shfl)

maskgrid = arcpy.GetParameterAsText(3)
if arcpy.Exists(maskgrid):
    desc = arcpy.Describe(maskgrid)
    mkgr = str(desc.catalogPath)
    arcpy.AddMessage("Input Mask Grid: " + mkgr)

maskthreshold = arcpy.GetParameterAsText(4)
if maskthreshold:
    arcpy.AddMessage("Input Mask Threshold Value: " + maskthreshold)

# Outputs
gord = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Strahler Network Order Grid: " + gord)

plen = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Output Longest Upslope Length Grid: " + plen)

tlen = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Output Total Upslope Length Grid: " + tlen)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' GridNet -p ' + '"' + p + '"' + ' -plen ' + '"' + plen + '"' + \
      ' -tlen ' + '"' + tlen + '"' + ' -gord ' + '"' + gord + '"'
if arcpy.Exists(ogrfile):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if arcpy.Exists(maskgrid):
    cmd = cmd + ' -mask ' + '"' + mkgr + '"' + ' -thresh ' + maskthreshold

arcpy.AddMessage("\nCommand Line: " + cmd)

return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# remove converted json file
if arcpy.Exists(ogrfile):
    # get extension of the converted json file
    extn_json = os.path.splitext(shfl)[1]
    if extn_json == ".json":
        os.remove(shfl)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'GridNetwork failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
