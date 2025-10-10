@echo off
setlocal enabledelayedexpansion

REM Check for build type parameter (default to Debug if not provided)
set BUILD_TYPE=Debug
set TARGET=
if "%1"=="release" (
    set BUILD_TYPE=Release
    echo Setting build type to Release
    set TARGET=%2
) else if "%1"=="debug" (
    set BUILD_TYPE=Debug
    echo Setting build type to Debug
    set TARGET=%2
) else if not "%1"=="" (
    set TARGET=%1
)

if not "%TARGET%"=="" (
    echo Building specific target: %TARGET%
)

echo Starting build process for x64 target in %BUILD_TYPE% mode...

REM Set build directory based on build type
set BUILD_DIR=build\debug
if /i "%BUILD_TYPE%"=="Release" set BUILD_DIR=build\release

REM Set GDAL environment variables for build
echo Setting up GDAL environment variables for build...
set GDAL_DATA=C:\dev\vcpkg\installed\x64-windows\share\gdal
set PROJ_LIB=C:\dev\vcpkg\installed\x64-windows\share\proj
set GDAL_DRIVER_PATH=C:\dev\vcpkg\installed\x64-windows\bin
set OGR_DRIVER_PATH=C:\dev\vcpkg\installed\x64-windows\bin
set OGR_ENABLED_DRIVERS=ESRI Shapefile SQLite GeoJSON KML
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

echo Running CMake configuration for x64 target (%BUILD_TYPE%)...
echo cmake -S . -B %BUILD_DIR% -C config.cmake -G "Visual Studio 17 2022" -A x64
cmake -S . -B %BUILD_DIR% -C config.cmake -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed with error code %ERRORLEVEL%
    goto error
)

echo Building project in %BUILD_TYPE% mode...
if not "%TARGET%"=="" (
    echo cmake --build %BUILD_DIR% --config %BUILD_TYPE% --target %TARGET%
    cmake --build %BUILD_DIR% --config %BUILD_TYPE% --target %TARGET%
) else (
    echo cmake --build %BUILD_DIR% --config %BUILD_TYPE%
    cmake --build %BUILD_DIR% --config %BUILD_TYPE%
)
if %ERRORLEVEL% NEQ 0 (
    echo Build failed with error code %ERRORLEVEL%
    goto error
)

echo Build completed successfully!

REM Copy files for release build
if /i "%BUILD_TYPE%"=="Release" (
    echo Copying release files to Taudem_Installation_Source/TauDEM_Exe/win_64...

    set "TARGET_DIR=%~dp0Taudem_Installation_Source\TauDEM_Exe\win_64"
    set "SOURCE_DIR=%~dp0!BUILD_DIR!\src\Release"

    echo Debug: BUILD_DIR is !BUILD_DIR!
    echo Debug: Checking source directory: !SOURCE_DIR!

    REM Check if source directory exists
    if not exist "!SOURCE_DIR!" (
        echo Warning: Source directory "!SOURCE_DIR!" does not exist
        echo Checking alternative paths...
        set "SOURCE_DIR=%~dp0!BUILD_DIR!\Release"
        echo Debug: Trying alternative: !SOURCE_DIR!
        if exist "!SOURCE_DIR!" (
            echo Using "!SOURCE_DIR!" instead
        ) else (
            echo Error: No Release executables found in build directory
            echo Checked: %~dp0!BUILD_DIR!\src\Release
            echo Checked: %~dp0!BUILD_DIR!\Release
            dir "%~dp0!BUILD_DIR!" /B
            goto error
        )
    )

    REM Create target directory if it doesn't exist
    if not exist "!TARGET_DIR!" (
        echo Creating target directory...
        mkdir "!TARGET_DIR!"
    )

    REM Copy all files from release directory to target
    echo Copying from "!SOURCE_DIR!" to "!TARGET_DIR!"
    if exist "!SOURCE_DIR!\*.*" (
        copy "!SOURCE_DIR!\*.*" "!TARGET_DIR!\" /Y
        if !ERRORLEVEL! EQU 0 (
            echo Release files copied successfully!
        ) else (
            echo Warning: Some files may not have been copied successfully
        )
    ) else (
        echo Warning: No files found in "!SOURCE_DIR!"
        echo Directory contents:
        dir "!SOURCE_DIR!" /B
    )
)

goto end

:error
echo An error occurred during the build process.
echo Please check the output above for more details.

:end
echo Build process finished.
pause
endlocal