#  Makefile for building executables on a UNIX System.
#
#    David Tarboton, Dan Watson, Jeremy Neff
#    Utah State University
#    May 23, 2010
#
#  Copyright (C) 2010  David Tarboton, Utah State University
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License 
#  version 2, 1991 as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  A copy of the full GNU General Public License is included in file 
#  gpl.html. This is also available at:
#  http://www.gnu.org/copyleft/gpl.html
#  or from:
#  The Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
#  Boston, MA  02111-1307, USA.
#
#  If you wish to use or incorporate this program (or parts of it) into 
#  other software that does not meet the GNU General Public License 
#  conditions contact the author to request permission.
#  David G. Tarboton  
#  Utah State University 
#  8200 Old Main Hill 
#  Logan, UT 84322-8200 
#  USA 
#  http://www.engineering.usu.edu/dtarb/ 
#  email:  dtarb@usu.edu 
#
#
#  This software is distributed from http://hydrology.usu.edu/taudem/



#SHAPEFILES includes all files in the shapefile library
#These should be compiled using the makefile in the shape directory
SHAPEFILES = ReadOutlets.o

#OBJFILES includes classes, structures, and constants common to all files
OBJFILES = commonLib.o tiffIO.o

D8FILES = aread8mn.o aread8.o $(OBJFILES) $(SHAPEFILES)
DINFFILES = areadinfmn.o areadinf.o $(OBJFILES) $(SHAPEFILES)
D8 = D8FlowDirmn.o d8.o Node.o $(OBJFILES) $(SHAPEFILES)
D8EXTREAMUP = D8flowpathextremeup.o D8FlowPathExtremeUpmn.o  $(OBJFILES) $(SHAPEFILES)
D8HDIST = D8HDistToStrm.o D8HDistToStrmmn.o  $(OBJFILES) 
D8VDIST = D8VDistToStrm.o D8VDistToStrmmn.o  $(OBJFILES) 
DINFAVA = DinfAvalanche.o DinfAvalanchemn.o $(OBJFILES)
DINFCONCLIM = DinfConcLimAccum.o DinfConcLimAccummn.o $(OBJFILES) $(SHAPEFILES)
DINFDECAY = dinfdecayaccum.o DinfDecayAccummn.o $(OBJFILES) $(SHAPEFILES)
DINFDISTDOWN = DinfDistDown.o DinfDistDownmn.o $(OBJFILES)
DINFDISTUP = DinfDistUp.o DinfDistUpmn.o $(OBJFILES)
DINF = DinfFlowDirmn.o dinf.o Node.o  $(OBJFILES) $(SHAPEFILES)
DINFREVACCUM = DinfRevAccum.o DinfRevAccummn.o $(OBJFILES)
DINFTRANSLIMACCUM = DinfTransLimAccum.o DinfTransLimAccummn.o $(OBJFILES) $(SHAPEFILES)
DINFUPDEPEND = DinfUpDependence.o DinfUpDependencemn.o $(OBJFILES)
DROPANALYSISFILES = DropAnalysis.o DropAnalysismn.o $(OBJFILES) $(SHAPEFILES)
GRIDNET = gridnetmn.o gridnet.o $(OBJFILES) $(SHAPEFILES)
LENGTHAREA = LengthArea.o LengthAreamn.o $(OBJFILES)
MVOUTLETSTOSTRMFILES = MoveOutletsToStrm.o MoveOutletsToStrmmn.o $(OBJFILES) $(SHAPEFILES)
PEUKERDOUGLAS = PeukerDouglas.o PeukerDouglasmn.o $(OBJFILES)
PITREMOVE = flood.o PitRemovemn.o $(OBJFILES)
SLOPEAREA = SlopeArea.o SlopeAreamn.o $(OBJFILES)
SLOPEAREARATIO = SlopeAreaRatio.o SlopeAreaRatiomn.o $(OBJFILES)
SLOPEAVEDOWN = SlopeAveDown.o SlopeAveDownmn.o $(OBJFILES)
STREAMNET = streamnetmn.o streamnet.o $(OBJFILES) $(SHAPEFILES)
THRESHOLD = Threshold.o Thresholdmn.o $(OBJFILES)
TWI = TWI.o TWImn.o $(OBJFILES)
GAGEWATERSHED = gagewatershed.o gagewatershedmn.o $(OBJFILES) $(SHAPEFILES)
CONNECTDOWN = ConnectDown.o ConnectDownmn.o $(OBJFILES) $(SHAPEFILES)
CATCHHYDROGEO = CatchHydroGeo.o CatchHydroGeomn.o $(OBJFILES) $(SHAPEFILES)
INUNMAP = InunMap.o InunMapmn.o $(OBJFILES) $(SHAPEFILES)
EDITRASTER = EditRaster.o EditRastermn.o $(OBJFILES)
CATCHOUTLETS = CatchOutlets.o CatchOutletsmn.o $(OBJFILES) $(SHAPEFILES)
FLOWDIRCOND = flowdircond.o flowdirconditionmn.o $(OBJFILES) 
RETLIMFLOW = RetlimFlow.o RetLimFlowmn.o $(OBJFILES) 
SETREGION = SetRegion.o SetRegionmn.o $(OBJFILES) 
SINMAPSI = SinmapSI.o SinmapSImn.o $(OBJFILES) 



#The following are compiler flags common to all building rules
#CC = mpic++
CC = mpicxx
#CFLAGS=-g -Wall -DDEBUG -std=c++11
CFLAGS=-O2 -std=c++11
LARGEFILEFLAG= -D_FILE_OFFSET_BITS=64
#INCDIRS=-I/usr/lib/openmpi/include -I/usr/include/gdal
INCDIRS=`gdal-config --cflags`
INCDIRS+=`nc-config --cflags`
#LIBDIRS=-lgdal
#LDLIBS=-L/usr/local/lib -lgdal
LDLIBS=`gdal-config --libs`
LDLIBS+=`nc-config --libs`

#Rules: when and how to make a file
all : ../bin/areadinf ../bin/aread8 ../bin/moveoutletstostrm ../bin/dropanalysis ../bin/streamnet ../bin/gridnet ../bin/dinfflowdir ../bin/d8flowdir ../bin/d8flowpathextremeup ../bin/d8hdisttostrm ../bin/d8vdisttostrm ../bin/dinfavalanche ../bin/dinfconclimaccum ../bin/dinfdecayaccum ../bin/dinfdistdown ../bin/dinfdistup ../bin/dinfrevaccum ../bin/dinftranslimaccum ../bin/dinfupdependence ../bin/lengtharea ../bin/peukerdouglas ../bin/pitremove ../bin/slopearea ../bin/slopearearatio ../bin/slopeavedown ../bin/threshold ../bin/twi ../bin/gagewatershed ../bin/connectdown ../bin/catchhydrogeo ../bin/inunmap ../bin/editraster ../bin/catchoutlets ../bin/flowdircond ../bin/retlimflow ../bin/setregion ../bin/sinmapsi clean 

../bin/aread8 : $(D8FILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8FILES) $(LDLIBS) $(LDFLAGS)

../bin/areadinf : $(DINFFILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFFILES) $(LDLIBS) $(LDFLAGS)

../bin/d8flowdir : $(D8)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8) $(LDLIBS) $(LDFLAGS)

../bin/d8flowpathextremeup : $(D8EXTREAMUP)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8EXTREAMUP) $(LDLIBS) $(LDFLAGS)

../bin/d8hdisttostrm : $(D8HDIST)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8HDIST) $(LDLIBS) $(LDFLAGS)

../bin/d8vdisttostrm : $(D8VDIST)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8VDIST) $(LDLIBS) $(LDFLAGS)

../bin/dinfavalanche : $(DINFAVA)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFAVA) $(LDLIBS) $(LDFLAGS)

../bin/dinfconclimaccum : $(DINFCONCLIM)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFCONCLIM) $(LDLIBS) $(LDFLAGS)

../bin/dinfdecayaccum : $(DINFDECAY)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFDECAY) $(LDLIBS) $(LDFLAGS)

../bin/dinfdistdown : $(DINFDISTDOWN)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFDISTDOWN) $(LDLIBS) $(LDFLAGS)

../bin/dinfdistup : $(DINFDISTUP)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFDISTUP) $(LDLIBS) $(LDFLAGS)

../bin/dinfflowdir : $(DINF)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINF) $(LDLIBS) $(LDFLAGS) 

../bin/dinfrevaccum : $(DINFREVACCUM)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFREVACCUM) $(LDLIBS) $(LDFLAGS) 

../bin/dinftranslimaccum : $(DINFTRANSLIMACCUM)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFTRANSLIMACCUM) $(LDLIBS) $(LDFLAGS) 

../bin/dinfupdependence : $(DINFUPDEPEND)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFUPDEPEND) $(LDLIBS) $(LDFLAGS) 

../bin/dropanalysis : $(DROPANALYSISFILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DROPANALYSISFILES) $(LDLIBS) $(LDFLAGS)

../bin/gridnet : $(GRIDNET)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(GRIDNET) $(LDLIBS) $(LDFLAGS) 

../bin/lengtharea : $(LENGTHAREA)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(LENGTHAREA) $(LDLIBS) $(LDFLAGS) 

../bin/moveoutletstostrm : $(MVOUTLETSTOSTRMFILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(MVOUTLETSTOSTRMFILES) $(LDLIBS) $(LDFLAGS)

../bin/peukerdouglas : $(PEUKERDOUGLAS)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(PEUKERDOUGLAS) $(LDLIBS) $(LDFLAGS)

../bin/pitremove : $(PITREMOVE)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(PITREMOVE) $(LDLIBS) $(LDFLAGS)

../bin/slopearea : $(SLOPEAREA)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SLOPEAREA) $(LDLIBS) $(LDFLAGS)

../bin/slopearearatio : $(SLOPEAREARATIO)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SLOPEAREARATIO) $(LDLIBS) $(LDFLAGS)

../bin/slopeavedown : $(SLOPEAVEDOWN)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SLOPEAVEDOWN) $(LDLIBS) $(LDFLAGS)

../bin/streamnet : $(STREAMNET)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(STREAMNET) $(LDLIBS) $(LDFLAGS) 

../bin/threshold : $(THRESHOLD)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(THRESHOLD) $(LDLIBS) $(LDFLAGS) 

../bin/twi : $(TWI)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(TWI) $(LDLIBS) $(LDFLAGS)

../bin/gagewatershed : $(GAGEWATERSHED)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(GAGEWATERSHED) $(LDLIBS) $(LDFLAGS)

../bin/connectdown : $(CONNECTDOWN)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(CONNECTDOWN) $(LDLIBS) $(LDFLAGS)
	
../bin/catchhydrogeo : $(CATCHHYDROGEO)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(CATCHHYDROGEO) $(LDLIBS) $(LDFLAGS)
	
../bin/inunmap : $(INUNMAP)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(INUNMAP) $(LDLIBS) $(LDFLAGS)
	
../bin/editraster : $(EDITRASTER)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(EDITRASTER) $(LDLIBS) $(LDFLAGS)

../bin/catchoutlets : $(CATCHOUTLETS)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(CATCHOUTLETS) $(LDLIBS) $(LDFLAGS)

../bin/flowdircond : $(FLOWDIRCOND)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(FLOWDIRCOND) $(LDLIBS) $(LDFLAGS)

../bin/retlimflow : $(RETLIMFLOW)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(RETLIMFLOW) $(LDLIBS) $(LDFLAGS)

../bin/setregion : $(SETREGION)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SETREGION) $(LDLIBS) $(LDFLAGS)

../bin/sinmapsi : $(SINMAPSI)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SINMAPSI) $(LDLIBS) $(LDFLAGS)

#Inference rule - states a general rule for compiling .o files
%.o : %.cpp
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

clean :
	rm *.o 


