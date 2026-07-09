import arcpy

import Utils

# get the input parameters
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
ang = str(desc.catalogPath)
arcpy.AddMessage("\nInput D-Infinity Flow Direction Grid: " + ang)

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
wg = str(desc.catalogPath)
arcpy.AddMessage("Input Weight Grid: " + wg)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
rc = str(desc.catalogPath)
arcpy.AddMessage("Input Retention Capacity Grid: " + rc)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Number of Processes: " + inputProc)

# Outputs
qrl = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Output Retention Limited Runoff Grid: " + qrl)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' retlimflow' + \
      ' -ang ' + Utils.quote(ang) + \
      ' -wg ' + Utils.quote(wg) + \
      ' -rc ' + Utils.quote(rc) + \
      ' -qrl ' + Utils.quote(qrl)

arcpy.AddMessage("\nCommand Line: " + cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'RetLimFlow failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
