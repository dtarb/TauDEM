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

Dependencies
------------
Dependencies include GDAL, MPI and C++ 2011.
On Windows Dependencies are provided in the Windows Installer

On Linux dependencies can be tricky.  I've added the following scripts to help, though you may need to adjust based on your system.  
GDAL:  GDAL.sh installs from GDAL source. You could also try
apt-get install gdal-bin libgdal-dev
apt-get install gdal-bin=2.1.3+dfsg-1~xenial2

C++: The script GCC.sh contains some commands I've used to get the required compiler.

MPI: MPICH2.sh installs from mpich.org stable distribution source.
sudo apt-get install openmpi-bin may also be an option, but I have not tried this.

Testing
-------
See the repository https://github.com/dtarb/TauDEM-Test-Data for test data and scripts that exercise every function.  These can also serve as examples for using some of the functions.

