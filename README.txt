TauDEM (Terrain Analysis Using Digital Elevation Models) is a suite of Digital Elevation Model (DEM) tools for the extraction and analysis of hydrologic information from topography as represented by a DEM.

For more information on the development of TauDEM please refer to the wiki https://github.com/dtarb/TauDEM/wiki.

For the latest release and detailed documentation please refer to the website: http://hydrology.usu.edu/taudem.


Building on Linux
-----------------
Both make and Cmake options are available to accommodate different preferences and system demands
Using make
cd src
make
The executables are written to bin directory

Using Cmake
cd src && mkdir build && cd build
cmake ..
make && make install
The executables are written to /usr/local/taudem directory.  This can be changed at the second last line (following DESTINATION) if desired.
