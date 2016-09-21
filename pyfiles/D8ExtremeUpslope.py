# Script Name: D8ExtremeUpslope
#
# Created By:  David Tarboton
# Date:        9/29/11

# Import ArcPy site-package and os modules
import arcpy
import os
import subprocess

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

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)

message = "\n"
for line in process.stdout.readlines():
    if isinstance(line, bytes):  # true in Python 3
        line = line.decode()
    message = message + line
arcpy.AddMessage(message)

# Calculate statistics on the output so that it displays properly
arcpy.AddMessage('Calculate Statistics\n')
arcpy.CalculateStatistics_management(ssa)
# remove converted json file
extn_json = os.path.splitext(shfl)[1]     # get extension of the converted json file
if extn_json == ".json":
    os.remove(shfl)
