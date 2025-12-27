# Script Name: DinfTransLimitAccum
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import os
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
ang = str(desc.catalogPath)
arcpy.AddMessage("\nInput D-Infinity Flow Direction Grid: " + ang)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
tsup = str(desc.catalogPath)
arcpy.AddMessage("Input Supply Grid: " + tsup)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
tc = str(desc.catalogPath)
arcpy.AddMessage("Input Transport Capacity Grid: " + tc)

inlyr3 = arcpy.GetParameterAsText(3)
if arcpy.Exists(inlyr3):
    desc = arcpy.Describe(inlyr3)
    cs = str(desc.catalogPath)
    arcpy.AddMessage("Input Concentration Grid: " + cs)

ogrfile=arcpy.GetParameterAsText(4)
if arcpy.Exists(ogrfile):
    desc = arcpy.Describe(ogrfile)
    shfl1 = str(desc.catalogPath)
    extn = os.path.splitext(shfl1)[1]     # get extension of a file
    # if extention is shapfile do not convert into gjson other wise convert
    if extn == ".shp":
       shfl = shfl1
    else:
      arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
      basename = os.path.basename(shfl1)    # get last part of the path
      dirname = os.path.dirname(ang)    # get directory
      arcpy.env.workspace = dirname     # does not work without specifying the workspace
      arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")    # convert feature to json
      shfl = os.path.join(dirname, basename + ".json")
    arcpy.AddMessage("Input Outlets Shapefile: " + shfl)

edgecontamination = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Edge Contamination: " + edgecontamination)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
tla = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Output Transport Limited Accumulation Grid: " + tla)

tdep = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Output Deposition Grid: " + tdep)

ctpt = arcpy.GetParameterAsText(9)
if arcpy.Exists(inlyr3):
    arcpy.AddMessage("Output Concentration Grid: " + ctpt)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' DinfTransLimAccum -ang ' + '"' + ang + '"' + ' -tsup ' + '"' + \
      tsup + '"' + ' -tc ' + '"' + tc + '"' + ' -tla ' + '"' + tla + '"' + ' -tdep ' + '"' + tdep + '"'
if arcpy.Exists(inlyr3):
    cmd = cmd + ' -cs ' + '"' + cs + '"' + ' -ctpt ' + '"' + ctpt + '"'
if arcpy.Exists(ogrfile):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if edgecontamination == 'false':
    cmd = cmd + ' -nc '

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# remove converted json file
if arcpy.Exists(ogrfile):
    # get extension of the converted json file
    extn_json = os.path.splitext(shfl)[1]
    if extn_json == ".json":
        os.remove(shfl)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'DinfTransLimAccum failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
