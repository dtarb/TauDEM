# Script Name: PeukerDouglas
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
arcpy.AddMessage("\nInput Elevation file: " + fel)

centerweight = arcpy.GetParameterAsText(1)
arcpy.AddMessage("Center Smoothing Weight: " + centerweight)

sideweight = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Side Smoothing Weight: " + sideweight)

diagonalweight = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Diagonal Smoothing Weight: " + diagonalweight)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
ss = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Stream Source file: " + ss)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' PeukerDouglas -fel ' + '"' + fel + '"' + ' -ss ' + '"' + ss + '"' + \
      ' -par ' + centerweight + ' ' + sideweight + ' ' + diagonalweight

arcpy.AddMessage("\nCommand Line: " + cmd)

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
arcpy.CalculateStatistics_management(ss)
