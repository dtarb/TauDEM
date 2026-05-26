# Script Name: Catchment Hydraulic Properties
#
# Created By:  Pabitra Dash
# Date:        02/18/2026

import arcpy
import Utils

# Get and describe the HAND (Height Above Nearest Drainage) input
inHAND = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inHAND)
handfile = str(desc.catalogPath)
arcpy.AddMessage("\nInput HAND file: " + handfile)

# Get and describe the catchment grid input
inCatch = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inCatch)
catchfile = str(desc.catalogPath)
arcpy.AddMessage("Input Catchment Grid file: " + catchfile)

# Get the catchment ID list file
catchlistfile = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Input Catchment ID List file: " + catchlistfile)

# Get and describe the D-inf slope input
inSlope = arcpy.GetParameterAsText(3)
desc = arcpy.Describe(inSlope)
slpfile = str(desc.catalogPath)
arcpy.AddMessage("Input D-inf Slope file: " + slpfile)

# Get the stage height file
hfile = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Input Stage Height file: " + hfile)

# Get the output hydraulic properties table file
hpfile = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Hydraulic Properties Table file: " + hpfile)

# Get the Input No. of Processes
inputProc = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Construct the taudem command line. Put quotes around file names in case there are spaces
cmd = 'mpiexec -n ' + inputProc + ' catchhydrogeo -hand ' + '"' + handfile + '"' + \
      ' -catch ' + '"' + catchfile + '"' + ' -catchlist ' + '"' + catchlistfile + '"' + \
      ' -slp ' + '"' + slpfile + '"' + ' -h ' + '"' + hfile + '"' + \
      ' -table ' + '"' + hpfile + '"'

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'CatchHydroGeo failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    raise arcpy.ExecuteError()

