# Script Name: SlopeOverAreaRatio
#
# Created By:  David Tarboton
# Date:        9/22/11

# Import ArcPy site-package and os modules
import arcpy
import os
import subprocess

# Input
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
sca = str(desc.catalogPath)
arcpy.AddMessage("\nInput Secific Catchment Area Grid: " + sca)

inlyr2 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr2)
slp = str(desc.catalogPath)
arcpy.AddMessage("Input Slope Grid: " + slp)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
twi = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Output Topographic Wetness Index Grid: " + twi)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' TWI -sca ' + '"' + sca + '"' + ' -slp ' + '"' + slp + '"' + ' -twi ' + \
      '"' + twi + '"'
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
arcpy.CalculateStatistics_management(twi)
