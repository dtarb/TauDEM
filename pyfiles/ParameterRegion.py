# created by: Pabitra Dash

import arcpy

import Utils

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


try:
    arcpy.AddMessage('\nStarting region computation...')

    # Construct siregion command line. Put quotes around file names in case there are spaces.
    cmd = 'siregion' + \
          ' --dem ' + Utils.quote(dem_grid) + \
          ' --parreg ' + Utils.quote(output_region_grid) + \
          ' --att ' + Utils.quote(calibration_table_text_file)

    if region_grid:
        cmd += ' --parreg-in ' + Utils.quote(region_grid)

    if region_feature_class:
        cmd += ' --shp ' + Utils.quote(region_feature_class)

    if region_feature_class_selected_attribute:
        cmd += ' --shp-att-name ' + Utils.quote(region_feature_class_selected_attribute)

    arcpy.AddMessage('\nCommand Line: ' + cmd)

    return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)
    if return_code != 0:
        raise Exception('siregion failed with return code ' + str(return_code))

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
