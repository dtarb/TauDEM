# Script Name: SlopeAreaCombination
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
slp = str(desc.catalogPath)
arcpy.AddMessage("\nInput Slope Grid: " + slp)

inlyr = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr)
sca = str(desc.catalogPath)
arcpy.AddMessage("Input Area Grid: " + sca)

slopeexponent = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Slope Exponent(m): " + slopeexponent)

areaexponent = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Area Exponent(n): " + areaexponent)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
sa = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Slope Area Grid: " + sa)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' SlopeArea -slp ' + '"' + slp + '"' + ' -sca ' + '"' + sca + '"' + \
      ' -sa ' + '"' + sa + '"' + ' -par ' + slopeexponent + ' ' + areaexponent

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
arcpy.CalculateStatistics_management(sa)
