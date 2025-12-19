# created by: Pabitra Dash

import os
import sys

import arcpy

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

# Add the current script directory to sys.path to allow importing SIRegionTool
current_script_dir = os.path.dirname(os.path.realpath(__file__))
if current_script_dir not in sys.path:
    sys.path.insert(0, current_script_dir)

# Import the SIRegionTool module
import SIRegionTool

try:
    arcpy.AddMessage('\nStarting region computation...')

    # Call the validation function
    SIRegionTool._validate_args(
        dem_grid, region_grid if region_grid else None,
        region_feature_class if region_feature_class else None,
        region_feature_class_selected_attribute if region_feature_class_selected_attribute else 'ID',
        output_region_grid, calibration_table_text_file
        )

    # Create parameter region grid
    SIRegionTool._create_parameter_region_grid(
        dem_grid,
        region_feature_class if region_feature_class else None,
        region_feature_class_selected_attribute if region_feature_class_selected_attribute else 'ID',
        region_grid if region_grid else None,
        output_region_grid
        )

    # Create parameter attribute table
    SIRegionTool._create_parameter_attribute_table_text_file(
        output_region_grid,
        region_grid if region_grid else None,
        region_feature_class if region_feature_class else None,
        calibration_table_text_file,
        2.708, 2.708, 0.0, 0.25, 30.0, 45.0, 2000.0
        )

    arcpy.AddMessage('\nRegion computation successful.')

except Exception as e:
    arcpy.AddError('\n' + '='*50)
    arcpy.AddError('REGION COMPUTATION FAILED')
    arcpy.AddError('='*50)
    arcpy.AddError(str(e))
    import traceback
    arcpy.AddError('\nFull traceback:')
    arcpy.AddError(traceback.format_exc())
    # let ArcGIS know the execution failed
    raise
