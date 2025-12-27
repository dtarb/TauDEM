# Script Name: Remove Pits
#
# Created By:  David Tarboton
# Date:        9/21/11

import arcpy
import Utils

# Get and describe the first argument
inLyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inLyr)
inZfile = str(desc.catalogPath)
arcpy.AddMessage("\nInput Elevation file: " + inZfile)

considering4way = arcpy.GetParameterAsText(1)
arcpy.AddMessage("Considering4way: " + considering4way)

maskgrid = arcpy.GetParameterAsText(2)
if arcpy.Exists(maskgrid):
    desc = arcpy.Describe(maskgrid)
    mkgr=str(desc.catalogPath)
    arcpy.AddMessage("Input Mask Grid: "+mkgr)

# Get the Input No. of Processes
inputProc=arcpy.GetParameterAsText(3)
arcpy.AddMessage(" Number of Processes: "+inputProc)

# Get the output file
outFile = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Output Pit Removed Elevation file: " + outFile)

# Construct the taudem command line.  Put quotes around file names in case there are spaces
# Construct command
cmd = 'mpiexec -n ' + inputProc + ' pitremove -z ' + '"' + inZfile + '"' + ' -fel ' + '"' + outFile + '"'
if considering4way == 'true':
    cmd = cmd + ' -4way '
if arcpy.Exists(maskgrid):
    cmd = cmd + ' -depmask ' + '"' + mkgr + '"'
if arcpy.Exists(maskgrid) and considering4way == 'true':
    cmd = cmd + ' -depmask ' + '"' + mkgr + '"' + ' -4way '

arcpy.AddMessage("\nCommand Line: "+cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'PitRemove failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()
