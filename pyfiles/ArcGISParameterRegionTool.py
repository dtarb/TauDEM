# created by: Pabitra Dash

import arcpy
import os
import subprocess

# get the input parameters
dem_grid = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(dem_grid)
dem_grid = str(desc.catalogPath)

# parameter 1 is the region creation option - ignore this parameter

region_grid = arcpy.GetParameterAsText(2)
if arcpy.Exists(region_grid):
    desc = arcpy.Describe(region_grid)
    region_grid = str(desc.catalogPath)

region_feature_class = arcpy.GetParameterAsText(3)
if arcpy.Exists(region_feature_class):
    desc = arcpy.Describe(region_feature_class)
    region_feature_class = str(desc.catalogPath)

region_feature_class_selected_attribute = arcpy.GetParameterAsText(4)
output_region_grid = arcpy.GetParameterAsText(5)
calibration_table_text_file = arcpy.GetParameterAsText(6)

# construct command to execute
current_script_dir = os.path.dirname(os.path.realpath(__file__))
# put quotes around file paths in case they have spaces
dem_grid = '"' + dem_grid + '"'
if len(region_grid) > 0:
    region_grid = '"' + region_grid + '"'

if len(region_feature_class) > 0:
    region_feature_class = '"' + region_feature_class + '"'

output_region_grid = '"' + output_region_grid + '"'
calibration_table_text_file = '"' + calibration_table_text_file + '"'

py_script_to_execute = os.path.join(current_script_dir, 'SIRegionTool.py')
py_script_to_execute = '"' + py_script_to_execute + '"'
cmd = py_script_to_execute + \
      ' --dem ' + dem_grid + \
      ' --parreg ' + output_region_grid + \
      ' --att ' + calibration_table_text_file

if len(region_grid) > 0:
    cmd += ' --parreg-in ' + region_grid

if len(region_feature_class) > 0:
    cmd += ' --shp ' + region_feature_class
    cmd += ' --shp-att-name ' + region_feature_class_selected_attribute

# show executing command
arcpy.AddMessage('\nEXECUTING COMMAND:\n' + cmd)

# Capture the contents of shell command and print it to the arcgis dialog box
process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
arcpy.AddMessage('\nProcess started:\n')
start_message = "Please wait. It may take few seconds. Computation is in progress ..."
arcpy.AddMessage(start_message)
for line in process.stdout.readlines():
    if isinstance(line, bytes):	    # true in Python 3
        line = line.decode()
    if start_message not in line:
        arcpy.AddMessage(line)
