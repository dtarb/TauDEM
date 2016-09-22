# Script Name: DinfContributingArea
#
# Created By:  David Tarboton
# Date:        9/28/11

# Import ArcPy site-package and os modules
import arcpy
import os
import subprocess

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
ang = str(desc.catalogPath)
arcpy.AddMessage("\nInput Dinf Flow Direction file: " + ang)

ogrfile = arcpy.GetParameterAsText(1)
if arcpy.Exists(ogrfile):
    desc = arcpy.Describe(ogrfile)
    shfl1 = str(desc.catalogPath)
    extn = os.path.splitext(shfl1)[1]     # get extension of a file
    # if extention is shapfile do not convert into gjson other wise convert
    if extn == ".shp":
       shfl = shfl1
    else:
      arcpy.AddMessage("Extracting json outlet file from: " + shfl1)
      basename = os.path.basename(shfl1)    # get last part of the path
      dirname = os.path.dirname(ang)  # get directory
      arcpy.env.workspace = dirname     # does not work without specifying the workspace
      arcpy.FeaturesToJSON_conversion(shfl1, basename + ".json")   # convert feature to json
      shfl = os.path.join(dirname, basename + ".json")
    arcpy.AddMessage("Using Outlets file: " + shfl)

weightgrid = arcpy.GetParameterAsText(2)
if arcpy.Exists(weightgrid):
    desc = arcpy.Describe(weightgrid)
    wtgr = str(desc.catalogPath)
    arcpy.AddMessage("Using Weight Grid: " + wtgr)

edgecontamination = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Edge Contamination: " + edgecontamination)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
sca = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Dinf Specific Catchment Area Grid: " + sca)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -sca ' + '"' + sca + '"'
if arcpy.Exists(ogrfile):
    cmd = cmd + ' -o ' + '"' + shfl + '"'
if arcpy.Exists(weightgrid):
    cmd = cmd + ' -wg ' + '"' + wtgr + '"'
if edgecontamination == 'false':
    cmd = cmd + ' -nc '

# TODO: Deleted these commented code lines (Pabitra, Dt:9/16/2016)
##if arcpy.Exists(shapefile) and arcpy.Exists(weightgrid) and edgecontamination == 'false':
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -o ' + '"' + shapefile + '"' + ' -sca ' + '"' + sca + '"' + ' -wg ' + '"' + weightgrid + '"' + ' -nc '
##elif arcpy.Exists(shapefile) and arcpy.Exists(weightgrid):
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -o ' + '"' + shapefile + '"' + ' -sca ' + '"' + sca + '"' + ' -wg ' + '"' + weightgrid + '"'
##elif arcpy.Exists(weightgrid) and edgecontamination == 'false':
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -sca ' + '"' + sca + '"' + ' -wg ' + '"' + weightgrid + '"' + ' -nc '
##elif arcpy.Exists(shapefile) and edgecontamination == 'false':
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -o ' + '"' + shapefile + '"' + ' -sca ' + '"' + sca + '"' + ' -nc '
##elif arcpy.Exists(shapefile):
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -o ' + '"' + shapefile + '"' + ' -sca ' + '"' + sca + '"'
##elif arcpy.Exists(weightgrid):
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -wg ' + '"' + weightgrid + '"' + ' -sca ' + '"' + sca + '"'
##elif edgecontamination == 'false':
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -sca ' + '"' + sca + '"' + ' -nc '
##else:
##    cmd = 'mpiexec -n ' + inputProc + ' AreaDinf -ang ' + '"' + ang + '"' + ' -sca ' + '"' + sca + '"'

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
arcpy.CalculateStatistics_management(sca)
# remove converted json file
if arcpy.Exists(ogrfile):
    extn_json = os.path.splitext(shfl)[1]    # get extension of the converted json file
    if extn_json == ".json":
        os.remove(shfl)
