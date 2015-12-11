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
p=str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction Grid: "+p)

shapefile=arcpy.GetParameterAsText(1)
desc = arcpy.Describe(shapefile)
shfl=str(desc.catalogPath)
arcpy.AddMessage("\nInput Outlets Shapefile: "+shfl)

# Input Number of Processes
inputProc=arcpy.GetParameterAsText(2)
arcpy.AddMessage("\nInput Number of Processes: "+inputProc)

# Output
gw = arcpy.GetParameterAsText(3)
arcpy.AddMessage("\nOutput GageWatershed Grid: "+gw)

# Output
idf = arcpy.GetParameterAsText(4)
arcpy.AddMessage("\nOutput Downstream ID Text File: "+idf)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' GageWatershed'
cmd = cmd+ ' -p ' + '"' + p + '"'
cmd = cmd + ' -o ' + '"' + shfl + '"'
cmd = cmd + ' -gw ' + '"' + gw + '"'
if idf != '':
    cmd=cmd + ' -id ' + '"' + idf + '"' 

arcpy.AddMessage("\nCommand Line: "+cmd)

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process=subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
arcpy.AddMessage('\nProcess started:\n')
for line in process.stdout.readlines():
    arcpy.AddMessage(line)

# Calculate statistics on the output so that it displays properly
arcpy.AddMessage('Executing: Calculate Statistics\n')
arcpy.CalculateStatistics_management(gw)
