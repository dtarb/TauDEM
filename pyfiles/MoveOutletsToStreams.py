# Script Name: MoveOuletsToStreams
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
coord_sys = desc.spatialReference
arcpy.AddMessage("Spatial Reference: " + str(coord_sys.name))

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
src = str(desc.catalogPath)
arcpy.AddMessage("Input Stream Raster Grid: " + src)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
shfl1 = str(desc.catalogPath)
extn = os.path.splitext(shfl1)[1]   # get extension of a file

# if extention is shapfile do not convert into gjson other wise convert
if extn == ".shp":
    shfl = shfl1
else:
    arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
    basename = os.path.basename(shfl1)  # get last part of the path
    dirname = os.path.dirname(p)   # get directory
    arcpy.env.workspace = dirname   # does not work without specifying the workspace
    arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")  # convert feature to json
    shfl = os.path.join(dirname, basename + ".json")

arcpy.AddMessage("Using Outlets ogrfile: " + shfl)

maxdistance = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Minimum Threshold Value: " + maxdistance)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Input Number of Processes: " + inputProc)

# Output
om = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Outlet file: " + om)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' MoveOutletsToStreams -p ' + '"' + p + '"' + ' -src ' + '"' + src + \
      '"' + ' -o ' + '"' + shfl + '"' + ' -om ' + '"' + om + '"' + ' -md ' + maxdistance

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

#arcpy.DefineProjection_management(om, coord_sys)
# remove converted json file
if arcpy.Exists(inlyr2):
    extn_json = os.path.splitext(shfl)[1]   # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)
