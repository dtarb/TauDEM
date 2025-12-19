# Script Name: ConnectDown
#
# Created By:  Nazmus Sazib
# Date:        11/16/2015

import arcpy
import Utils

# Inputs
inlyr = arcpy.GetParameterAsText(0)
desc = arcpy.Describe(inlyr)
p=str(desc.catalogPath)
arcpy.AddMessage("\nInput D8 Flow Direction Grid: " + p)
coord_sys = desc.spatialReference
arcpy.AddMessage("Spatial Reference: " + str(coord_sys.name))

inlyr1 = arcpy.GetParameterAsText(1)
desc = arcpy.Describe(inlyr1)
ad8 = str(desc.catalogPath)
arcpy.AddMessage("Input D8Contributing Area Grid: " + ad8)

inlyr2 = arcpy.GetParameterAsText(2)
desc = arcpy.Describe(inlyr2)
ws = str(desc.catalogPath)
arcpy.AddMessage("Input Watershed Grid: " + ws)

mvdistance = arcpy.GetParameterAsText(3)
arcpy.AddMessage("Number of Grid cell: " + mvdistance)

# Input Number of Processes
inputProc = arcpy.GetParameterAsText(4)
arcpy.AddMessage("Input Number of Processes: " + inputProc)

# Output
om = arcpy.GetParameterAsText(5)
arcpy.AddMessage("Output Outlet file: "+om)

# Output
omd = arcpy.GetParameterAsText(6)
arcpy.AddMessage("Output MovedOutlet file: "+omd)

# Construct command
cmd = 'mpiexec -n ' + inputProc + ' ConnectDown -p ' + '"' + p + '"' + ' -ad8 ' + '"' + \
      ad8 + '"' + ' -w ' + '"' + ws + '"' + ' -o ' + '"' + om + '"' + ' -od ' + '"' + omd + '"' + ' -d ' + mvdistance

arcpy.AddMessage("\nCommand Line: "+cmd)

# Run the command using the shared utility function
return_code = Utils.run_taudem_command(cmd, arcpy.AddMessage)

# Check return code and add error message BEFORE raising exception
if return_code != 0:
    err_msg = f'ConnectDown failed with return code: {return_code}'
    arcpy.AddError(err_msg)
    # Include all messages in the exception so they're visible
    raise arcpy.ExecuteError()
