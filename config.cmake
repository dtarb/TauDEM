# Detect platform (all settings here are default fallback values for specific platforms)
if(APPLE)
    # macOS settings
    # Set GDAL paths first
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/gdal" CACHE PATH "GDAL installation path")
    set(GDAL_INCLUDE_DIR "/opt/homebrew/opt/gdal/include" CACHE PATH "GDAL include directory")
    set(GDAL_LIBRARY "/opt/homebrew/opt/gdal/lib/libgdal.dylib" CACHE PATH "GDAL library")

    # Now try to find GDAL
    find_package(GDAL REQUIRED)
    message(STATUS "Found GDAL version: ${GDAL_VERSION}")

    # MPI settings
    set(MPI_C_COMPILER "/opt/homebrew/bin/mpicc" CACHE PATH "MPI C compiler")
    set(MPI_CXX_COMPILER "/opt/homebrew/bin/mpic++" CACHE PATH "MPI C++ compiler")
    set(MPI_C_LIBRARIES "/opt/homebrew/lib/libmpi.dylib" CACHE PATH "MPI libraries")
    set(MPI_C_INCLUDE_PATH "/opt/homebrew/include" CACHE PATH "MPI include path")
elseif(UNIX AND NOT APPLE)
    # Linux/Docker settings
    # MPI settings
    set(MPI_C_COMPILER "/usr/bin/mpicc" CACHE PATH "MPI C compiler")
    set(MPI_CXX_COMPILER "/usr/bin/mpicxx" CACHE PATH "MPI C++ compiler")
    set(MPI_C_LIBRARIES "/usr/lib/aarch64-linux-gnu/libmpi.so" CACHE PATH "MPI libraries")
    set(MPI_CXX_LIBRARIES "/usr/lib/aarch64-linux-gnu/libmpi.so" CACHE PATH "MPI C++ libraries")
    set(MPI_C_INCLUDE_PATH "/usr/lib/aarch64-linux-gnu/openmpi/include" CACHE PATH "MPI include path")
    set(MPI_CXX_INCLUDE_PATH "/usr/lib/aarch64-linux-gnu/openmpi/include" CACHE PATH "MPI C++ include path")

    # GDAL settings
    set(CMAKE_PREFIX_PATH "/usr/lib/gdal" CACHE PATH "GDAL installation path")
elseif(WIN32)
    message(STATUS "Configuring for Windows x64 platform")
    
    # Set vcpkg toolchain file and target triplet
    set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "Vcpkg toolchain file")
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")
    
    # Add vcpkg installed path to prefix path
    list(APPEND CMAKE_PREFIX_PATH "C:/dev/vcpkg/installed/x64-windows")
    
    # Set specific library and include directories for GDAL
    set(GDAL_ROOT "C:/dev/vcpkg/installed/x64-windows" CACHE PATH "GDAL root directory")
    set(GDAL_INCLUDE_DIR "${GDAL_ROOT}/include" CACHE PATH "GDAL include directory")
    set(GDAL_LIBRARY "${GDAL_ROOT}/lib/gdal.lib" CACHE PATH "GDAL library")
    
    # Set MPI paths (using vcpkg's MPI)
    set(MPI_ROOT "C:/dev/vcpkg/installed/x64-windows" CACHE PATH "MPI root directory")
    
    # Windows-specific compiler flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_WIN32_WINNT=0x0A00" CACHE STRING "C flags" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0A00" CACHE STRING "CXX flags" FORCE)
endif()

# Common settings for all platforms
if(NOT CMAKE_INSTALL_PREFIX)
    # Use absolute path with CMAKE_CURRENT_LIST_DIR to ensure consistency
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install" CACHE PATH "Installation directory")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug/Release)")
endif()

# Platform-specific compiler settings
if(APPLE)
    if(NOT CMAKE_C_COMPILER)
        set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE PATH "C compiler")
    endif()
    if(NOT CMAKE_CXX_COMPILER)
        set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE PATH "C++ compiler")
    endif()
endif()