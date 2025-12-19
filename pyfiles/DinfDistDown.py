# Script Name: DinfDistDown
#
# Created By:  David Tarboton
# Date:        9/29/11

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
ang = str(desc.catalogPath)
arcpy.AddMessage("\nInput D-Infinity Flow Direction Grid: " + ang)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
fel = str(desc.catalogPath)
arcpy.AddMessage("Input Pit Filled Elevation Grid: " + fel)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
src = str(desc.catalogPath)
arcpy.AddMessage("Input Stream Raster Grid: " + src)

statisticalmethod = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Statistical Method: " + statisticalmethod)

distancemethod = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Distance Method: " + distancemethod)

edgecontamination = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Edge Contamination: " + edgecontamination)

weightgrid = arcpy.GetParameterAsText(6)
if arcpy.Exists(weightgrid):
    desc = arcpy.Describe(weightgrid)
    wg = str(desc.catalogPath)
    arcpy.AddMessage("Input Weight Path Grid: " + wg)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(7)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
dd = arcpy.GetParameterAsText(8)
arcpy.AddMessage("Output D-Infinity Drop to Stream Grid: " + dd)

# Construct command
if statisticalmethod == 'Average':
    statmeth = 'ave'
if statisticalmethod == 'Maximum':
    statmeth = 'max'
if statisticalmethod == 'Minimum':
    statmeth = 'min'
if distancemethod == 'Horizontal':
    distmeth = 'h'
if distancemethod == 'Vertical':
    distmeth = 'v'
if distancemethod == 'Pythagoras':
    distmeth = 'p'
if distancemethod == 'Surface':
    distmeth = 's'
cmd = 'mpiexec -n ' + inputProc + ' DinfDistDown -fel ' + '"' + fel + '"' + ' -ang ' + '"' + ang + '"' + \
      ' -src ' + '"' + src + '"' + ' -dd ' + '"' + dd + '"' + ' -m ' + statmeth + ' ' + distmeth
if arcpy.Exists(weightgrid):
    cmd = cmd + ' -wg ' + '"' + wg + '"'
if edgecontamination == 'false':
    cmd = cmd + ' -nc '

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'DinfDistDown failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
