cd D:\Scratch\TestSuite\gridtypes
Set TDIR=D:\Dropbox\Projects\TauDEM\Programming\TauDEM5GDAL\Taudem5PCVS2010\x64\Release
set MDIR=C:\Program Files\Microsoft HPC Pack 2012\Bin\
set GDIR=C:\Program Files\GDAL\
set path=%MDIR%;%TDIR%;%GDIR%

Rem  Basic grid analysis
mpiexec -np 3 PitRemove logan.tif
mpiexec -np 3 PitRemove -z logan.tif -fel loganfelim.img
mpiexec -np 3 PitRemove -z logan.tif -fel loganfelsd.sdat
mpiexec -np 3 PitRemove -z logan.tif -fel loganfel.bil
mpiexec -np 3 PitRemove -z logan.tif -fel loganfel1.bin
mpiexec -np 3 PitRemove -z logan.tif -fel loganfel.bil
mpiexec -np 3 D8Flowdir -fel loganfel1.bin -p bilp.bil -sd8 binsd8.bin
mpiexec -n 5 aread8 -p bilp.bil -ad8 loganad8.img
mpiexec -n 2 dinfflowdir -fel loganfel.bil -ang ang.ang -slp slp.slp
mpiexec -np 3 PitRemove -z logan.tif -fel loganfel2.lg
mpiexec -np 3 PitRemove -z logan.tif -fel loganfel3





