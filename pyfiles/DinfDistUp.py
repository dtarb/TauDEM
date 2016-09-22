# Script Name: DinfDistUp
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
ang = str(desc.catalogPath)
arcpy.AddMessage("\nInput D-Infinity Flow Direction Grid: " + ang)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
fel = str(desc.catalogPath)
arcpy.AddMessage("Input Pit Filled Elevation Grid: " + fel)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
slp = str(desc.catalogPath)
arcpy.AddMessage("Input Slope Grid: " + slp)

propthresh=arcpy.GetParameterAsText(3)
arcpy.AddMessage("Input Proportion Threshold: "+propthresh)

statisticalmethod = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Statistical Method: " + statisticalmethod)

distancemethod = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Distance Method: " + distancemethod)

edgecontamination=arcpy.GetParameterAsText(6)
arcpy.AddMessage("Edge Contamination: "+edgecontamination)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
du = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Output D-Infinity Distance Up: " + du)

# Construct command
if statisticalmethod == 'Average':
    statmeth = 'ave'
if statisticalmethod == 'Maximum':
    statmeth = 'max'
if statisticalmethod == 'Minimum':
    statmeth = 'min'
if distancemethod == 'Horizontal':
    distmeth = 'h'
if distancemethod == 'Vertical':
    distmeth = 'v'
if distancemethod == 'Pythagoras':
    distmeth = 'p'
if distancemethod == 'Surface':
    distmeth = 's'
cmd = 'mpiexec -n ' + inputProc + ' DinfDistUp -fel ' + '"' + fel + '"' + ' -ang ' + '"' + ang + '"' + \
      ' -slp ' + '"' + slp + '"' + ' -du ' + '"' + du + '"' + ' -m ' + statmeth + ' ' + distmeth + \
      ' -thresh ' + propthresh
if edgecontamination == 'false':
    cmd = cmd + ' -nc '

arcpy.AddMessage("\nCommand Line: " + cmd)

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)

message = "\n"
for line in process.stdout.readlines():
    if isinstance(line, bytes):	   # true in Python 3
        line = line.decode()
    message = message + line
arcpy.AddMessage(message)

# Calculate statistics on the output so that it displays properly
arcpy.AddMessage('Calculate Statistics\n')
arcpy.CalculateStatistics_management(du)
