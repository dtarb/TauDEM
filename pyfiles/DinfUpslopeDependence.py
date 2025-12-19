# Script Name: DinfUpslopeDependence
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
dg = str(desc.catalogPath)
arcpy.AddMessage("Input Destination Grid: " + dg)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(2)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Output
dep = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Output Upslope Dependence Grid: " + dep)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' DinfUpDependence -ang ' + '"' + ang + '"' + ' -dg ' + '"' + dg + \
      '"' + ' -dep ' + '"' + dep + '"'

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'DinfUpDependence failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
