## TauDEM
TauDEM (Terrain Analysis Using Digital Elevation Models) is a suite of Digital Elevation Model (DEM) tools for the extraction and analysis of hydrologic information from topography as represented by a DEM. 

For the latest release and detailed documentation please refer to the website: http://hydrology.usu.edu/taudem

For more information on the development of TauDEM please refer to the wiki: https://github.com/dtarb/TauDEM/wiki

#### Building on Linux
We use the CMake build system on Linux platforms. It takes care of finding GDAL and MPI on your system.

```
$ mkdir build && cd build
$ cmake ..
$ make
```

If GDAL is installed in a non-standard path, you can try using the GDAL_ROOT variable. You can also change the compiler with the CC/CXX variables. For example:

```
$ CC=icc CXX=icpc GDAL_ROOT=/path/to/gdal cmake ..
```



