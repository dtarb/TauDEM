cmake_minimum_required(VERSION 2.8.9)
project(TauDEM)

set (shape_srcs ReadOutlets.cpp)
set (common_srcs commonLib.cpp tiffIO.cpp)

set (D8FILES aread8mn.cpp aread8.cpp ${common_srcs} ${shape_srcs})
set (DINFFILES areadinfmn.cpp areadinf.cpp ${common_srcs} ${shape_srcs})
set (D8 D8FlowDirmn.cpp d8.cpp Node.cpp ${common_srcs} ${shape_srcs})
set (D8EXTREAMUP D8flowpathextremeup.cpp D8FlowPathExtremeUpmn.cpp
     ${common_srcs} ${shape_srcs})
set (D8HDIST D8HDistToStrm.cpp D8HDistToStrmmn.cpp ${common_srcs})
set (DINFAVA DinfAvalanche.cpp DinfAvalanchemn.cpp ${common_srcs})
set (DINFCONCLIM DinfConcLimAccum.cpp DinfConcLimAccummn.cpp
     ${common_srcs} ${shape_srcs})
set (DINFDECAY dinfdecayaccum.cpp DinfDecayAccummn.cpp
     ${common_srcs} ${shape_srcs})
set (DINFDISTDOWN DinfDistDown.cpp DinfDistDownmn.cpp ${common_srcs})
set (DINFDISTUP DinfDistUp.cpp DinfDistUpmn.cpp ${common_srcs})
set (DINF DinfFlowDirmn.cpp dinf.cpp Node.cpp
     ${common_srcs} ${shape_srcs})
set (DINFREVACCUM DinfRevAccum.cpp DinfRevAccummn.cpp ${common_srcs})
set (DINFTRANSLIMACCUM DinfTransLimAccum.cpp DinfTransLimAccummn.cpp
     ${common_srcs} ${shape_srcs})
set (DINFUPDEPEND DinfUpDependence.cpp DinfUpDependencemn.cpp ${common_srcs})
set (DROPANALYSISFILES DropAnalysis.cpp DropAnalysismn.cpp
     ${common_srcs} ${shape_srcs})
set (GRIDNET gridnetmn.cpp gridnet.cpp
     ${common_srcs} ${shape_srcs})
set (LENGTHAREA LengthArea.cpp LengthAreamn.cpp ${common_srcs})
set (MVOUTLETSTOSTRMFILES MoveOutletsToStrm.cpp MoveOutletsToStrmmn.cpp
     ${common_srcs} ${shape_srcs})
set (PEUKERDOUGLAS PeukerDouglas.cpp PeukerDouglasmn.cpp ${common_srcs})
set (PITREMOVE flood.cpp PitRemovemn.cpp ${common_srcs})
set (SLOPEAREA SlopeArea.cpp SlopeAreamn.cpp ${common_srcs})
set (SLOPEAREARATIO SlopeAreaRatio.cpp SlopeAreaRatiomn.cpp ${common_srcs})
set (SLOPEAVEDOWN SlopeAveDown.cpp SlopeAveDownmn.cpp ${common_srcs})
set (STREAMNET streamnetmn.cpp streamnet.cpp
     ${common_srcs} ${shape_srcs})
set (THRESHOLD Threshold.cpp Thresholdmn.cpp ${common_srcs})
set (GAGEWATERSHED gagewatershedmn.cpp gagewatershed.cpp ${common_srcs} ${shape_srcs})

# MPI is required
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAG ${CMAKE_CXX_FLAG} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

# GDAL is required
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})

add_executable (aread8 ${D8FILES})
add_executable (areadinf ${DINFFILES})
add_executable (d8flowdir ${D8})
add_executable (d8flowpathextremeup ${D8EXTREAMUP})
add_executable (d8hdisttostrm ${D8HDIST})
add_executable (dinfavalanche ${DINFAVA})
add_executable (dinfconclimaccum ${DINFCONCLIM})
add_executable (dinfdecayaccum ${DINFDECAY})
add_executable (dinfdistdown ${DINFDISTDOWN})
add_executable (dinfdistup ${DINFDISTUP})
add_executable (dinfflowdir ${DINF})
add_executable (dinfrevaccum ${DINFREVACCUM})
add_executable (dinftranslimaccum ${DINFTRANSLIMACCUM})
add_executable (dinfupdependence ${DINFUPDEPEND})
add_executable (dropanalysis ${DROPANALYSISFILES})
add_executable (gridnet ${GRIDNET})
add_executable (lengtharea ${LENGTHAREA})
add_executable (moveoutletstostrm ${MVOUTLETSTOSTRMFILES})
add_executable (peukerdouglas ${PEUKERDOUGLAS})
add_executable (pitremove ${PITREMOVE})
add_executable (slopearea ${SLOPEAREA})
add_executable (slopearearatio ${SLOPEAREARATIO})
add_executable (slopeavedown ${SLOPEAVEDOWN})
add_executable (streamnet ${STREAMNET})
add_executable (threshold ${THRESHOLD})
add_executable (gagewatershed ${GAGEWATERSHED})

set (MY_TARGETS aread8 
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
		gagewatershed)

foreach( c_target ${MY_TARGETS} )
    target_link_libraries(${c_target} ${MPI_LIBRARIES} ${GDAL_LIBRARY})
    install(TARGETS ${c_target} DESTINATION bin)
endforeach( c_target ${MY_TARGETS} )
