# Script Name: StreamReachAndWatershed
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
coord_sys = desc.spatialReference
arcpy.AddMessage("Spatial Reference: " + str(coord_sys.name))

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
p = str(desc.catalogPath)
arcpy.AddMessage("Input D8 Flow Direction Grid: " + p)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
ad8 = str(desc.catalogPath)
arcpy.AddMessage("Input D8 Drainage Area: " + ad8)

inlyr3 = arcpy.GetParameterAsText(3)
desc = arcpy.Describe(inlyr3)
src = str(desc.catalogPath)
arcpy.AddMessage("Input Stream Raster Grid: " + src)

ogrfile = arcpy.GetParameterAsText(4)
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
        dirname = os.path.dirname(p)    # get directory
        arcpy.env.workspace = dirname   # does not work without specifying the workspace
        arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")  # convert feature to json
        shfl = os.path.join(dirname, basename + ".json")

    arcpy.AddMessage("Using Outlets " + shfl)

delineate = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Delineate Single Watershed: " + delineate)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
ord = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Output Stream Order Grid: " + ord)

tree = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Output Network Connectivity Tree: " + tree)

coord = arcpy.GetParameterAsText(9)
arcpy.AddMessage("Output Network Coordinates: " + coord)

net = arcpy.GetParameterAsText(10)
arcpy.AddMessage("Output Stream Reach file: " + net)

w = arcpy.GetParameterAsText(11)
arcpy.AddMessage("Output Watershed Grid: " + w)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' StreamNet -fel ' + '"' + fel + '"' + ' -p ' + '"' + p + '"' + \
      ' -ad8 ' + '"' + ad8 + '"' + ' -src ' + '"' + src + '"' + ' -ord ' + '"' + ord + '"' + ' -tree ' + \
      '"' + tree + '"' + ' -coord ' + '"' + coord + '"' + ' -net ' + '"' + net + '"' + ' -w ' + '"' + w + \
      '"'
if arcpy.Exists(ogrfile):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if delineate == 'true':
    cmd = cmd + ' -sw '

arcpy.AddMessage("\nCommand Line: " + cmd)

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process=subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)

message = "\n"
for line in process.stdout.readlines():
    if isinstance(line, bytes):	    # true in Python 3
        line = line.decode()
    message = message + line
arcpy.AddMessage(message)

# Calculate statistics on the output so that it displays properly
arcpy.AddMessage('Calculate Statistics\n')
arcpy.CalculateStatistics_management(ord)
#arcpy.DefineProjection_management(net, coord_sys)

arcpy.CalculateStatistics_management(w)
# remove converted json file
if arcpy.Exists(ogrfile):
    extn_json = os.path.splitext(shfl)[1]   # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)
