# Script Name: ConnectDown
#
# Created By:  Nazmus Sazib
# Date:        11/16/2015

# Import ArcPy site-package and os modules
import arcpy
import os
import subprocess

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
p=str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction Grid: " + p)
coord_sys = desc.spatialReference
arcpy.AddMessage("Spatial Reference: " + str(coord_sys.name))

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
ad8 = str(desc.catalogPath)
arcpy.AddMessage("Input D8Contributing Area Grid: " + ad8)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
ws = str(desc.catalogPath)
arcpy.AddMessage("Input Watershed Grid: " + ws)

mvdistance = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Number of Grid cell: " + mvdistance)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Input Number of Processes: " + inputProc)

# Output
om = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Outlet file: "+om)

# Output
omd = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Output MovedOutlet file: "+omd)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' ConnectDown -p ' + '"' + p + '"' + ' -ad8 ' + '"' + \
      ad8 + '"' + ' -w ' + '"' + ws + '"' + ' -o ' + '"' + om + '"' + ' -od ' + '"' + omd + '"' + ' -d ' + mvdistance

arcpy.AddMessage("\nCommand Line: "+cmd)

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


