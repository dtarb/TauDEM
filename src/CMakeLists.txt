#############################################################
# CMake settings

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

SET(CMAKE_COLOR_MAKEFILE ON)
SET(BUILD_SHARED_LIBS OFF)

#############################################################
# search for dependencies
FIND_PACKAGE(MPI REQUIRED)
IF (NOT MPI_CXX_FOUND)
  MESSAGE(FATAL_ERROR "Couldn't find MPI CXX compiler")
ENDIF (NOT MPI_CXX_FOUND)

#############################################################
# SHAPE_SRCS includes all files in the shapefile library
# These should be compiled using the makefile in the shape directory
SET(SHAPE_SRCS
  shape/cell.cpp
  shape/dbf.cpp
  shape/exception.cpp
  shape/field.cpp
  shape/item.cpp
  shape/point.cpp
  shape/record.cpp
  shape/shape.cpp
  shape/shapefile.cpp
  shape/shapemain.cpp
  shape/shp_point.cpp
  shape/shp_polygon.cpp
  shape/shp_polyline.cpp
  shape/ReadOutlets.cpp
)

#############################################################
# COMMON_SRCS includes classes, structures, and constants common to all files
SET(COMMON_SRCS
  commonLib.cpp
  tiffIO.cpp
  tifFile.cpp
)

#############################################################
#

SET(D8FILES
  aread8mn.cpp
  aread8.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(DINFFILES
  areadinfmn.cpp
  areadinf.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(D8
  D8FlowDirmn.cpp
  d8.cpp
  Node.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(D8EXTREAMUP
  D8flowpathextremeup.cpp
  D8FlowPathExtremeUpmn.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(D8HDIST
  D8HDistToStrm.cpp
  D8HDistToStrmmn.cpp
  ${COMMON_SRCS}
)

SET(DINFAVA
  DinfAvalanche.cpp
  DinfAvalanchemn.cpp
  ${COMMON_SRCS}
)

SET(DINFCONCLIM
  DinfConcLimAccum.cpp
  DinfConcLimAccummn.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(DINFDECAY
  dinfdecayaccum.cpp
  DinfDecayAccummn.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(DINFDISTDOWN
  DinfDistDown.cpp
  DinfDistDownmn.cpp
  ${COMMON_SRCS}
)

SET(DINFDISTUP
  DinfDistUp.cpp
  DinfDistUpmn.cpp
  ${COMMON_SRCS}
)

SET(DINF
  DinfFlowDirmn.cpp
  dinf.cpp
  Node.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(DINFREVACCUM
  DinfRevAccum.cpp
  DinfRevAccummn.cpp
  ${COMMON_SRCS}
)

SET(DINFTRANSLIMACCUM
  DinfTransLimAccum.cpp
  DinfTransLimAccummn.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(DINFUPDEPEND
  DinfUpDependence.cpp
  DinfUpDependencemn.cpp
  ${COMMON_SRCS}
)

SET(DROPANALYSISFILES
  DropAnalysis.cpp
  DropAnalysismn.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(GRIDNET
  gridnetmn.cpp
  gridnet.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(LENGTHAREA
  LengthArea.cpp
  LengthAreamn.cpp
  ${COMMON_SRCS}
)

SET(MVOUTLETSTOSTRMFILES
  MoveOutletsToStrm.cpp
  MoveOutletsToStrmmn.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(PEUKERDOUGLAS
  PeukerDouglas.cpp
  PeukerDouglasmn.cpp
  ${COMMON_SRCS}
)

SET(PITREMOVE
  flood.cpp
  PitRemovemn.cpp
  ${COMMON_SRCS}
)

SET(SLOPEAREA
  SlopeArea.cpp
  SlopeAreamn.cpp
  ${COMMON_SRCS}
)

SET(SLOPEAREARATIO
  SlopeAreaRatio.cpp
  SlopeAreaRatiomn.cpp
  ${COMMON_SRCS}
)

SET(SLOPEAVEDOWN
  SlopeAveDown.cpp
  SlopeAveDownmn.cpp
  ${COMMON_SRCS}
)

SET(STREAMNET
  streamnetmn.cpp
  streamnet.cpp
  ${COMMON_SRCS}
  ${SHAPE_SRCS}
)

SET(THRESHOLD
  Threshold.cpp
  Thresholdmn.cpp
  ${COMMON_SRCS}
)

#############################################################
# TauDEM tools
ADD_EXECUTABLE(aread8 ${D8FILES})
ADD_EXECUTABLE(areadinf ${DINFFILES})
ADD_EXECUTABLE(d8flowdir ${D8})
ADD_EXECUTABLE(d8flowpathextremeup ${D8EXTREAMUP})
ADD_EXECUTABLE(d8hdisttostrm ${D8HDIST})
ADD_EXECUTABLE(dinfavalanche ${DINFAVA})
ADD_EXECUTABLE(dinfconclimaccum ${DINFCONCLIM})
ADD_EXECUTABLE(dinfdecayaccum ${DINFDECAY})
ADD_EXECUTABLE(dinfdistdown ${DINFDISTDOWN})
ADD_EXECUTABLE(dinfdistup ${DINFDISTUP})
ADD_EXECUTABLE(dinfflowdir ${DINF})
ADD_EXECUTABLE(dinfrevaccum ${DINFREVACCUM})
ADD_EXECUTABLE(dinftranslimaccum ${DINFTRANSLIMACCUM})
ADD_EXECUTABLE(dinfupdependence ${DINFUPDEPEND})
ADD_EXECUTABLE(dropanalysis ${DROPANALYSISFILES})
ADD_EXECUTABLE(gridnet ${GRIDNET})
ADD_EXECUTABLE(lengtharea ${LENGTHAREA})
ADD_EXECUTABLE(moveoutletstostrm ${MVOUTLETSTOSTRMFILES})
ADD_EXECUTABLE(peukerdouglas ${PEUKERDOUGLAS})
ADD_EXECUTABLE(pitremove ${PITREMOVE})
ADD_EXECUTABLE(slopearea ${SLOPEAREA})
ADD_EXECUTABLE(slopearearatio ${SLOPEAREARATIO})
ADD_EXECUTABLE(slopeavedown ${SLOPEAVEDOWN})
ADD_EXECUTABLE(streamnet ${STREAMNET})
ADD_EXECUTABLE(threshold ${THRESHOLD})

#############################################################
# Install
INSTALL(TARGETS
  aread8
  areadinf
  d8flowdir
  d8flowpathextremeup
  d8hdisttostrm
  dinfavalanche
  dinfconclimaccum
  dinfdecayaccum
  dinfdistdown
  dinfdistup
  dinfflowdir
  dinfrevaccum
  dinftranslimaccum
  dinfupdependence
  dropanalysis
  gridnet
  lengtharea
  moveoutletstostrm
  peukerdouglas
  pitremove
  slopearea
  slopearearatio
  slopeavedown
  streamnet
  threshold
DESTINATION bin)
