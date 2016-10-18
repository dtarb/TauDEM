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



#The following are compiler flags common to all building rules
CC = mpic++
CFLAGS=-O2
CFLAGS=-g -Wall
CFLAGS=-g
LARGEFILEFLAG= -D_FILE_OFFSET_BITS=64
#INCDIRS=-I/usr/lib/openmpi/include -I/usr/include/gdal
INCDIRS=`gdal-config --cflags`
#LIBDIRS=-lgdal
#LDLIBS=-L/usr/local/lib -lgdal
LDLIBS=`gdal-config --libs`

#Rules: when and how to make a file
all : ../areadinf ../aread8 ../moveoutletstostrm ../dropanalysis ../streamnet ../gridnet ../dinfflowdir ../d8flowdir ../d8flowpathextremeup ../d8hdisttostrm ../dinfavalanche ../dinfconclimaccum ../dinfdecayaccum ../dinfdistdown ../dinfdistup ../dinfrevaccum ../dinftranslimaccum ../dinfupdependence ../lengtharea ../peukerdouglas ../pitremove ../slopearea ../slopearearatio ../slopeavedown ../threshold ../twi ../gagewatershed ../connectdown clean 

../aread8 : $(D8FILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8FILES) $(LDLIBS) $(LDFLAGS)

../areadinf : $(DINFFILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFFILES) $(LDLIBS) $(LDFLAGS)

../d8flowdir : $(D8)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8) $(LDLIBS) $(LDFLAGS)

../d8flowpathextremeup : $(D8EXTREAMUP)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8EXTREAMUP) $(LDLIBS) $(LDFLAGS)

../d8hdisttostrm : $(D8HDIST)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8HDIST) $(LDLIBS) $(LDFLAGS)

../dinfavalanche : $(DINFAVA)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFAVA) $(LDLIBS) $(LDFLAGS)

../dinfconclimaccum : $(DINFCONCLIM)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFCONCLIM) $(LDLIBS) $(LDFLAGS)

../dinfdecayaccum : $(DINFDECAY)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFDECAY) $(LDLIBS) $(LDFLAGS)

../dinfdistdown : $(DINFDISTDOWN)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFDISTDOWN) $(LDLIBS) $(LDFLAGS)

../dinfdistup : $(DINFDISTUP)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFDISTUP) $(LDLIBS) $(LDFLAGS)

../dinfflowdir : $(DINF)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINF) $(LDLIBS) $(LDFLAGS) 

../dinfrevaccum : $(DINFREVACCUM)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFREVACCUM) $(LDLIBS) $(LDFLAGS) 

../dinftranslimaccum : $(DINFTRANSLIMACCUM)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFTRANSLIMACCUM) $(LDLIBS) $(LDFLAGS) 

../dinfupdependence : $(DINFUPDEPEND)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DINFUPDEPEND) $(LDLIBS) $(LDFLAGS) 

../dropanalysis : $(DROPANALYSISFILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(DROPANALYSISFILES) $(LDLIBS) $(LDFLAGS)

../gridnet : $(GRIDNET)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(GRIDNET) $(LDLIBS) $(LDFLAGS) 

../lengtharea : $(LENGTHAREA)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(LENGTHAREA) $(LDLIBS) $(LDFLAGS) 

../moveoutletstostrm : $(MVOUTLETSTOSTRMFILES)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(MVOUTLETSTOSTRMFILES) $(LDLIBS) $(LDFLAGS)

../peukerdouglas : $(PEUKERDOUGLAS)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(PEUKERDOUGLAS) $(LDLIBS) $(LDFLAGS)

../pitremove : $(PITREMOVE)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(PITREMOVE) $(LDLIBS) $(LDFLAGS)

../slopearea : $(SLOPEAREA)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SLOPEAREA) $(LDLIBS) $(LDFLAGS)

../slopearearatio : $(SLOPEAREARATIO)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SLOPEAREARATIO) $(LDLIBS) $(LDFLAGS)

../slopeavedown : $(SLOPEAVEDOWN)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SLOPEAVEDOWN) $(LDLIBS) $(LDFLAGS)

../streamnet : $(STREAMNET)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(STREAMNET) $(LDLIBS) $(LDFLAGS) 

../threshold : $(THRESHOLD)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(THRESHOLD) $(LDLIBS) $(LDFLAGS) 

../twi : $(TWI)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(TWI) $(LDLIBS) $(LDFLAGS)

../gagewatershed : $(GAGEWATERSHED)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(GAGEWATERSHED) $(LDLIBS) $(LDFLAGS)

../connectdown : $(CONNECTDOWN)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(CONNECTDOWN) $(LDLIBS) $(LDFLAGS)
	
#Inference rule - states a general rule for compiling .o files
%.o : %.cpp
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

clean :
	rm *.o 


