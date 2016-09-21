# Script Name: PeukarDouglasStreamDef
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

weightcenter = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Weight Center: " + weightcenter)

weightside = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Weight Side: " + weightside)

weightdiag = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Weight Diagonal: " + weightdiag)

accthresh = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Accumulation Threshold: " + accthresh)

contcheck = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Edge contamination checking: " + contcheck)

ogrlyr = arcpy.GetParameterAsText(7)
if arcpy.Exists(ogrlyr):
    desc = arcpy.Describe(ogrlyr)
    shfl1 = str(desc.catalogPath)
    extn = os.path.splitext(shfl1)[1]   # get extension of a file
    # if extention is shapfile do not convert into gjson other wise convert
    if extn == ".shp":
        shfl = shfl1
    else:
        arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
        basename = os.path.basename(shfl1)  # get last part of the path
        dirname = os.path.dirname(p)  # get directory
        arcpy.env.workspace = dirname   # does not work without specifying the workspace
        arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")  # convert feature to json
        shfl = os.path.join(dirname, basename + ".json")

    arcpy.AddMessage("Using Outlets file: " + shfl)

masklyr = arcpy.GetParameterAsText(8)
if arcpy.Exists(masklyr):
    desc = arcpy.Describe(masklyr)
    mask = str(desc.catalogPath)
    arcpy.AddMessage("Input Mask Grid: " + mask)

ad8lyr = arcpy.GetParameterAsText(9)
if arcpy.Exists(ad8lyr):
    desc = arcpy.Describe(ad8lyr)
    ad8 = str(desc.catalogPath)
    arcpy.AddMessage("Input D8 Contributing Area for Drop Analysis: " + ad8)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(10)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
ss = arcpy.GetParameterAsText(11)
arcpy.AddMessage("Output Stream Source Grid: " + ss)

ssa = arcpy.GetParameterAsText(12)
arcpy.AddMessage("Output Accumulated Stream Source Grid: " + ssa)

src = arcpy.GetParameterAsText(13)
arcpy.AddMessage("Output Stream Raster Grid: " + src)

drp=arcpy.GetParameterAsText(14)
if arcpy.Exists(drp):
    arcpy.AddMessage("Output Drop Analysis Table: " + drp)

usedroprange = arcpy.GetParameterAsText(15)
arcpy.AddMessage("Select Threshold by Drop Analysis: " + usedroprange)

minthresh = arcpy.GetParameterAsText(16)
arcpy.AddMessage("Minimum Threshold Value: " + minthresh)

maxthresh =arcpy.GetParameterAsText(17)
arcpy.AddMessage("Maximum Threshold Value: " + maxthresh)

numthresh = arcpy.GetParameterAsText(18)
arcpy.AddMessage("Number of Threshold Values: " + numthresh)

logspace = arcpy.GetParameterAsText(19)
arcpy.AddMessage("Logarithmic Spacing: " + logspace)

# Construct first command
cmd = 'mpiexec -n ' + inputProc + ' PeukerDouglas -fel ' + '"' + fel + '"' + ' -ss ' + '"' + ss + '"' + \
      ' -par ' + weightcenter + ' ' + weightside + ' ' + weightdiag
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
arcpy.CalculateStatistics_management(ss)

# Construct second command
cmd = 'mpiexec -n ' + inputProc + ' AreaD8 -p ' + '"' + p + '"' + ' -ad8 ' + '"' + ssa + '"'
cmd = cmd + ' -wg ' + '"' + ss + '"'
if arcpy.Exists(ogrlyr):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if contcheck == 'false':
    cmd = cmd + ' -nc'
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
arcpy.CalculateStatistics_management(ssa)

if (usedroprange == 'true') and arcpy.Exists(ogrlyr):
    # Construct third command
    cmd = 'mpiexec -n ' + inputProc + ' DropAnalysis -fel ' + '"' + fel + '"' + ' -p ' + '"' + p + '"' + \
          ' -ad8 ' + '"' + ad8 + '"'
    cmd = cmd + ' -ssa ' + '"' + ssa + '"' + ' -o ' + '"' + shfl + '"' + ' -drp ' + '"' + drp + '"' + \
        ' -par ' + minthresh + ' ' + maxthresh + ' ' + numthresh + ' '
    if logspace == 'false':
        cmd = cmd + '1'
    else:
        cmd = cmd + '0'

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

    # (threshold,rest)=line.split(' ',1)

    drpfile = open(drp,"r")
    theContents = drpfile.read()
    beg, threshold = theContents.rsplit(' ', 1)
    drpfile.close()

# Construct fourth command
cmd = 'mpiexec -n ' + inputProc + ' Threshold -ssa ' + '"' + ssa + '"' + ' -src ' + '"' + src + '"'
if (usedroprange == 'true') and arcpy.Exists(ogrlyr):
    cmd = cmd + ' -thresh ' + threshold
else:
    cmd = cmd + ' -thresh ' + accthresh
if arcpy.Exists(masklyr):
    cmd = cmd + ' -mask ' + '"' + mask + '"'
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
arcpy.CalculateStatistics_management(src)
# remove converted json file
if arcpy.Exists(ogrlyr):
    extn_json = os.path.splitext(shfl)[1]   # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)
