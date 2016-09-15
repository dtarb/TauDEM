# Script Name: DinfConcenLimitAccum
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
wg = str(desc.catalogPath)
arcpy.AddMessage("Input Effective Runoff Weight Grid: " + wg)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
dg = str(desc.catalogPath)
arcpy.AddMessage("Input Disturbance Indicator Grid: " + dg)

inlyr3 = arcpy.GetParameterAsText(3)
desc = arcpy.Describe(inlyr3)
dm = str(desc.catalogPath)
arcpy.AddMessage("Input Decay Multiplier Grid: " + dm)

ogrfile=arcpy.GetParameterAsText(4)
if arcpy.Exists(ogrfile):
    desc = arcpy.Describe(ogrfile)
    shfl1 = str(desc.catalogPath)
    extn = os.path.splitext(shfl1)[1]   # get extension of a file
    # if extention is shapfile do not convert into gjson other wise convert
    if extn == ".shp":
       shfl = shfl1
    else:
      arcpy.AddMessage("Extracting json outlet file from: "+shfl1)
      basename = os.path.basename(shfl1)   # get last part of the path
      dirname = os.path.dirname(ang)    # get directory
      arcpy.env.workspace = dirname     # does not work without specifying the workspace
      arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")   # convert feature to json
      shfl = os.path.join(dirname, basename + ".json")
    arcpy.AddMessage("Using Outlets file: " + shfl)

concthresh = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Concentration Threshold: " + concthresh)

edgecontamination = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Edge Contamination: " + edgecontamination)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
q = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Output Overland Flow Specific Discharge Grid: " + q)

ctpt = arcpy.GetParameterAsText(9)
arcpy.AddMessage("Output Concentration Grid: " + ctpt)

# Construct command 1
cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -sca ' + '"' + q + '"' + \
      ' -wg ' + '"' + wg + '"'
arcpy.AddMessage("\nCommand Line: " + cmd)

# Submit command to operating system
os.system(cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)

message = "\n"
for line in process.stdout.readlines():
    if isinstance(line, bytes):	    # true in Python 3
        line = line.decode()
    message = message+line
arcpy.AddMessage(message)

# Calculate statistics on the output so that it displays properly
arcpy.AddMessage('Calculate Statistics\n')
arcpy.CalculateStatistics_management(q)

# Construct command 2
cmd = 'mpiexec -n ' + inputProc + ' DinfConcLimAccum -ang ' + '"' + ang + '"' + ' -dg ' + '"' + dg + \
      '"' + ' -dm ' + '"' + dm + '"' + ' -ctpt ' + '"' + ctpt + '"' + ' -q ' + '"' + q + '"' + \
      ' -csol ' + concthresh
if arcpy.Exists(ogrfile):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if edgecontamination == 'false':
    cmd = cmd + ' -nc '

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
arcpy.CalculateStatistics_management(ctpt)
# remove converted json file
if arcpy.Exists(ogrfile):
    extn_json = os.path.splitext(shfl)[1]    # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)



