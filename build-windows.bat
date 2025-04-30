@echo off
setlocal enabledelayedexpansion

REM Check for build type parameter (default to Debug if not provided)
set BUILD_TYPE=Debug
if "%1"=="release" (
    set BUILD_TYPE=Release
    echo Setting build type to Release
)

echo Starting build process for x64 target in %BUILD_TYPE% mode...

REM Set build directory based on build type
set BUILD_DIR=src\build
if /i "%BUILD_TYPE%"=="Release" set BUILD_DIR=src\build-release

REM Set GDAL environment variables for build
echo Setting up GDAL environment variables for build...
set GDAL_DATA=C:\dev\vcpkg\installed\x64-windows\share\gdal
set PROJ_LIB=C:\dev\vcpkg\installed\x64-windows\share\proj
set GDAL_DRIVER_PATH=C:\dev\vcpkg\installed\x64-windows\bin
set OGR_DRIVER_PATH=C:\dev\vcpkg\installed\x64-windows\bin
set OGR_ENABLED_DRIVERS=ESRI Shapefile SQLite GeoJSON
set GDAL_REGISTRY=SQLITE:sqlite3.dll
set CPL_DEBUG=ON

REM Clean build directory if it exists
if exist %BUILD_DIR% (
    echo Cleaning build directory...
    rmdir /s /q %BUILD_DIR%
)

REM Create build directory
echo Creating build directory...
mkdir %BUILD_DIR%

echo Changing to build directory...
cd %BUILD_DIR%

echo Running CMake configuration for x64 target (%BUILD_TYPE%)...
echo cmake .. -C ../../config.cmake -G "Visual Studio 17 2022" -A x64
cmake .. -C ../../config.cmake -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed with error code %ERRORLEVEL%
    goto error
)

echo Building project in %BUILD_TYPE% mode...
echo cmake --build . --config %BUILD_TYPE%
cmake --build . --config %BUILD_TYPE%
if %ERRORLEVEL% NEQ 0 (
    echo Build failed with error code %ERRORLEVEL%
    goto error
)

echo Build completed successfully!
goto end

:error
echo An error occurred during the build process.
echo Please check the output above for more details.

:end
cd ..\..
echo Build process finished.
pause
endlocal