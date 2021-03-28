# Script Name: StreamDropAnalysis
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
fel = str(desc.catalogPath)
arcpy.AddMessage("\nInput Pit Filled Elevation Grid: " + fel)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
p = str(desc.catalogPath)
arcpy.AddMessage("Input D8 Flow Direction Grid: " + p)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
ad8 = str(desc.catalogPath)
arcpy.AddMessage("Input D8 Contributing Area Grid: " + ad8)

inlyr3 = arcpy.GetParameterAsText(3)
desc = arcpy.Describe(inlyr3)
ssa = str(desc.catalogPath)
arcpy.AddMessage("Input Accumulated Stream Source Grid: " + ssa)

ogrfile = arcpy.GetParameterAsText(4)
desc = arcpy.Describe(ogrfile)
shfl1 = str(desc.catalogPath)
extn = os.path.splitext(shfl1)[1]   # get extension of a file
# if extention is shapfile do not convert into gjson other wise convert
if extn == ".shp":
    shfl=shfl1
else:
    arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
    basename = os.path.basename(shfl1)  # get last part of the path
    dirname = os.path.dirname(p)    # get directory
    arcpy.env.workspace = dirname   # does not work without specifying the workspace
    arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")  # convert feature to json
    shfl = os.path.join(dirname, basename + ".json")

arcpy.AddMessage("Using Outlets: " + shfl)

minthresh = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Minimum Threshold Value: " + minthresh)

maxthresh = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Maximum Threshold Value: " + maxthresh)

numthresh = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Number of Threshold Values: " + numthresh)

logspace = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Logarithmic Spacing: " + logspace)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(9)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
drp = arcpy.GetParameterAsText(10)
arcpy.AddMessage("Output Drop Analysis Text File: " + drp)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' DropAnalysis -fel ' + '"' + fel + '"' + ' -p ' + '"' + p + '"' + \
      ' -ad8 ' + '"' + ad8 + '"' + ' -ssa ' + '"' + ssa + '"' + ' -o ' + '"' + shfl + '"' + ' -drp ' + \
      '"' + drp + '"' + ' -par ' + minthresh + ' ' + maxthresh + ' ' + numthresh + ' '
if logspace == 'false':
    cmd = cmd + '1'
else:
    cmd = cmd + '0'

arcpy.AddMessage("\nCommand Line: " + cmd)

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
arcpy.AddMessage('\nProcess started:\n')
for line in process.stdout.readlines():
    if isinstance(line, bytes):	    # true in Python 3
        line = line.decode()
    arcpy.AddMessage(line)

# Calculate statistics on the output so that it displays properly
#arcpy.AddMessage('Calculate Statistics\n')
#arcpy.CalculateStatistics_management(drp)  # DGT 12/7/19 does not make sense to calculate statistics on this drop file
if arcpy.Exists(ogrfile):
    extn_json = os.path.splitext(shfl)[1]   # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)
