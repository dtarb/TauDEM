cmake_minimum_required(VERSION 3.10)
project(TauDEM)

# Update to C++17 for filesystem support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add filesystem library for older compilers
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    link_libraries(stdc++fs)
endif()

set (shape_srcs ReadOutlets.cpp)
set (common_srcs commonLib.cpp tiffIO.cpp)

set (AREAD8 aread8mn.cpp aread8.cpp ${common_srcs} ${shape_srcs})
set (AREADINF areadinfmn.cpp areadinf.cpp ${common_srcs} ${shape_srcs})
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
set (DROPANALYSIS DropAnalysis.cpp DropAnalysismn.cpp
     ${common_srcs} ${shape_srcs})
set (GRIDNET gridnetmn.cpp gridnet.cpp
     ${common_srcs} ${shape_srcs})
set (LENGTHAREA LengthArea.cpp LengthAreamn.cpp ${common_srcs})
set (MVOUTLETSTOSTRM MoveOutletsToStrm.cpp MoveOutletsToStrmmn.cpp
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
set (TWI TWImn.cpp TWI.cpp ${common_srcs})
set (CONNECTDOWN ConnectDownmn.cpp ConnectDown.cpp ${common_srcs} ${shape_srcs})
set (CATCHHYDROGEO CatchHydroGeo.cpp CatchHydroGeomn.cpp ${common_srcs} ${shape_srcs})
set (CATCHOUTLETS CatchOutletsmn.cpp CatchOutlets.cpp ${common_srcs} ${shape_srcs})
set (EDITRASTER EditRastermn.cpp EditRaster.cpp ${common_srcs})
set (FLOWDIRCOND flowdirconditionmn.cpp flowdircond.cpp ${common_srcs})
set (RETLIMFLOW RetLimFlowmn.cpp RetlimFlow.cpp ${common_srcs})
set (SETREGION SetRegionmn.cpp SetRegion.cpp ${common_srcs})
set (SINMAPSI SinmapSImn.cpp SinmapSI.cpp ${common_srcs})
set (INUNMAP InunMapmn.cpp InunMap.cpp ${common_srcs})

# MPI configuration for Windows
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(MS_MPI_ARCH_DIR "x64")
    else()
        set(MS_MPI_ARCH_DIR "x86")
    endif()
    
    # Try to find MS-MPI through vcpkg first
    find_package(MPI)
    
    if(NOT MPI_FOUND)
        # Fallback to standard MS-MPI paths
        set(MPI_C_INCLUDE_PATH "C:/Program Files (x86)/Microsoft SDKs/MPI/Include")
        set(MPI_CXX_INCLUDE_PATH "C:/Program Files (x86)/Microsoft SDKs/MPI/Include")
        set(MPI_C_LIBRARIES "C:/Program Files (x86)/Microsoft SDKs/MPI/Lib/${MS_MPI_ARCH_DIR}/msmpi.lib")
        set(MPI_CXX_LIBRARIES "C:/Program Files (x86)/Microsoft SDKs/MPI/Lib/${MS_MPI_ARCH_DIR}/msmpi.lib")
    endif()
else()
    # Non-Windows platforms
    find_package(MPI REQUIRED)
endif()

include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS}")

# GDAL is required - try CONFIG mode first (for vcpkg)
find_package(GDAL CONFIG QUIET)
if(NOT GDAL_FOUND)
    # Fall back to MODULE mode if CONFIG mode fails
    message(STATUS "GDAL CONFIG mode failed, trying MODULE mode...")
    find_package(GDAL REQUIRED)
    include_directories(${GDAL_INCLUDE_DIR})
    message(STATUS "Found GDAL using MODULE mode: ${GDAL_VERSION}")
else()
    message(STATUS "Found GDAL using CONFIG mode: ${GDAL_VERSION}")
endif()

find_package(MPI)
message(STATUS "MPI_FOUND: ${MPI_FOUND}")
message(STATUS "MPI_LIBRARIES: ${MPI_LIBRARIES}")
message(STATUS "MPI_C_LIBRARIES: ${MPI_C_LIBRARIES}")
message(STATUS "MPI_CXX_LIBRARIES: ${MPI_CXX_LIBRARIES}")

add_executable (aread8 ${AREAD8})
add_executable (areadinf ${AREADINF})
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
add_executable (dropanalysis ${DROPANALYSIS})
add_executable (gridnet ${GRIDNET})
add_executable (lengtharea ${LENGTHAREA})
add_executable (moveoutletstostrm ${MVOUTLETSTOSTRM})
add_executable (peukerdouglas ${PEUKERDOUGLAS})
add_executable (pitremove ${PITREMOVE})
add_executable (slopearea ${SLOPEAREA})
add_executable (slopearearatio ${SLOPEAREARATIO})
add_executable (slopeavedown ${SLOPEAVEDOWN})
add_executable (streamnet ${STREAMNET})
add_executable (threshold ${THRESHOLD})
add_executable (gagewatershed ${GAGEWATERSHED})
add_executable (twi ${TWI})
add_executable (connectdown ${CONNECTDOWN})
add_executable (catchhydrogeo ${CATCHHYDROGEO})
add_executable (catchoutlets ${CATCHOUTLETS})
add_executable (editraster ${EDITRASTER})
add_executable (flowdircond ${FLOWDIRCOND})
add_executable (retlimflow ${RETLIMFLOW})
add_executable (setregion ${SETREGION})
add_executable (sinmapsi ${SINMAPSI})
add_executable (inunmap ${INUNMAP})

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
                gagewatershed
                twi
                connectdown
                catchhydrogeo
                catchoutlets
                editraster
                flowdircond
                retlimflow
                setregion
                sinmapsi
                inunmap)


foreach(c_target ${MY_TARGETS})
    # Link the target to MPI and GDAL
    if(TARGET GDAL::GDAL)
        # Modern GDAL target (from CONFIG mode)
        target_link_libraries(${c_target} PRIVATE ${MPI_LIBRARIES} GDAL::GDAL)
    else()
        # Old-style GDAL variables (from MODULE mode)
        target_link_libraries(${c_target} PRIVATE ${MPI_LIBRARIES} ${GDAL_LIBRARY})
    endif()
    
    # Add MPI include directories
    target_include_directories(${c_target} PRIVATE ${MPI_INCLUDE_PATH})
    
    # Add MPI compile flags
    target_compile_options(${c_target} PRIVATE ${MPI_COMPILE_FLAGS})
    
    # Add MPI link flags
    target_link_libraries(${c_target} PRIVATE ${MPI_LINK_FLAGS})

    # Install the targets to the 'taudem' directory
    install(TARGETS ${c_target} DESTINATION taudem)
endforeach()