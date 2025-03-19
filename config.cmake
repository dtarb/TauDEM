# Detect platform
if(APPLE)
    # macOS settings
    set(MPI_C_COMPILER "/opt/homebrew/bin/mpicc" CACHE PATH "MPI C compiler")
    set(MPI_CXX_COMPILER "/opt/homebrew/bin/mpic++" CACHE PATH "MPI C++ compiler")
    set(MPI_C_LIBRARIES "/opt/homebrew/lib/libmpi.dylib" CACHE PATH "MPI libraries")
    set(MPI_C_INCLUDE_PATH "/opt/homebrew/include" CACHE PATH "MPI include path")
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/gdal" CACHE PATH "GDAL installation path")
elseif(UNIX AND NOT APPLE)
    # Linux settings
    set(MPI_C_COMPILER "/usr/bin/mpicc" CACHE PATH "MPI C compiler")
    set(MPI_CXX_COMPILER "/usr/bin/mpic++" CACHE PATH "MPI C++ compiler")
    set(MPI_C_LIBRARIES "/usr/lib/libmpi.so" CACHE PATH "MPI libraries")
    set(MPI_C_INCLUDE_PATH "/usr/include" CACHE PATH "MPI include path")
    set(CMAKE_PREFIX_PATH "/usr/lib/gdal" CACHE PATH "GDAL installation path")
elseif(WIN32)
    # TODO: Windows settings
    # Add Windows-specific paths here
endif()

# Common settings for all platforms
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Installation directory")
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