# Script Name: GageWatershed
#
# Created By:  David Tarboton
# Date:        1/25/14

# Import ArcPy site-package and os modules
import arcpy
import os
import subprocess

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
p = str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction Grid: " + p)

ogrfile = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(ogrfile)
shfl1 = str(desc.catalogPath)
extn = os.path.splitext(shfl1)[1]   # get extension of a file

# if extention is shapfile do not convert into gjson other wise convert
if extn == ".shp":
    shfl = shfl1
else:
    arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
    basename = os.path.basename(shfl1)  # get last part of the path
    dirname =   os.path.dirname(p)  # get directory
    arcpy.env.workspace = dirname   # does not work without specifying the workspace
    arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")  # convert feature to json
    shfl = os.path.join(dirname, basename + ".json")
arcpy.AddMessage("Using Outlets file: " + shfl)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Input Number of Processes: " + inputProc)

# Output
gw = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Output GageWatershed Grid: " + gw)

# Output
idf = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Output Downstream ID Text File: " + idf)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' GageWatershed'
cmd = cmd+ ' -p ' + '"' + p + '"'
cmd = cmd + ' -o ' + '"' + shfl + '"'
cmd = cmd + ' -gw ' + '"' + gw + '"'
if idf != '':
    cmd = cmd + ' -id ' + '"' + idf + '"'

arcpy.AddMessage("\nCommand Line: " + cmd)

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)

message = "\n"
for line in process.stdout.readlines():
    if isinstance(line, bytes):	    # true in Python 3
        line = line.decode()
    message = message + line
arcpy.AddMessage(message)

# Calculate statistics on the output so that it displays properly
arcpy.AddMessage('Calculate Statistics\n')
arcpy.CalculateStatistics_management(gw)
# remove converted json file
if arcpy.Exists(ogrfile):
    extn_json = os.path.splitext(shfl)[1]   # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)

