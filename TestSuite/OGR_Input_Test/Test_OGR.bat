cd E:\USU_Research_work\MMW_PROJECT\TauDEM_Project\OGR_Input_Test
Set TDIR=E:\USU_Research_work\MMW_PROJECT\TauDEM_Project\TauDEM_deploy_6\Taudem5PCVS2010\x64\Release
set MDIR=C:\Program Files\Microsoft HPC Pack 2012\Bin\
set GDIR=C:\Program Files\GDAL\
set path=%MDIR%;%TDIR%;%GDIR%

cd AreaD8_data
aread8for Shapefile datasource
mpiexec -n 7 aread8 -p loganp.tif -ds LoganOutlet.shp -lyr  LoganOutlet -ad8 loganad8_shp.tif

aread8for sqlite data source
mpiexec -n 7 aread8 -p loganp.tif -ds LoganOutlet.sqlite -lyr LoganOutlet -ad8 loganad8_sqlite.img

aread8for geojson data source
mpiexec -n 7 aread8 -p loganp.tif -ds LoganOutlet.geojson -lyr  OGRgeojson -ad8 loganad8_geojson.tif

aread8for a directoy as data source
mpiexec -n 7 aread8 -p loganp.tif -ds LoganSampleSQLite -lyr  LoganOutlet -ad8 loganad8_dir.tif

cd ../streamnet_data

read outlet as shapefile and write streamnet as shapefile
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc logannet1.shp  -netlyr logannet1 -w loganw.tif -odsrc Outlet.shp -odlyr Outlet

read outlet as shapefile and write streamnet as sqlite file
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc logannet1.sqlite  -netlyr logannet1 -w loganw.tif -odsrc Outlet.shp -odlyr Outlet

read outlet as shapefile and write streamnet as geojson file
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc logannet1.geojson -netlyr logannet1.geojson -w loganw.tif -odsrc Outlet.shp -odlyr Outlet

read outlet as shapefile and write streamnet as shape file in same data source
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc LoganSampleSQLite  -netlyr logannets -w loganw.tif -odsrc LoganSampleSQLite -odlyr Outlet


read outlet as sqlite file and write streamnet as shapefile
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc logannet2.shp  -netlyr logannet2 -w loganw.tif -odsrc Outlet.sqlite -odlyr Outlet

read outlet as sqlite file and write streamnet as sqlite file
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc logannet2.sqlite  -netlyr logannet2 -w loganw.tif -odsrc Outlet.sqlite -odlyr Outlet

read outlet as sqlite file and write streamnet as geojson file
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -netsrc logannet2.geojson  -netlyr logannet2.geojson -w loganw.tif -odsrc Outlet.sqlite -odlyr Outlet


cd ../MovedOutletstoStream

read outlet as shapfile write move outlet as shapefile
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.shp -odlyr OutletstoMove  -omdsrc Outletsmoved1.shp  -omdlyr Outletsmoved1 -md 20 

read outlet as shapfile write move outlet as sqlite file
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.shp -odlyr OutletstoMove  -omdsrc Outletsmoved1.sqlite  -omdlyr Outletsmoved1 -md 20 

read outlet as shapfile write move outlet as geojson file
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.shp -odlyr OutletstoMove  -omdsrc Outletsmoved1.geojson  -omdlyr Outletsmoved1.geojson -md 20 

read outlet as shapfile write move outlet as shapefile for same data source
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc LoganSampleSQLite -odlyr OutletstoMove  -omdsrc LoganSampleSQLite  -omdlyr Outletsmoved1 -md 20 


read outlet as sqlite write move outlet as shapefile
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.sqlite -odlyr OutletstoMove  -omdsrc Outletsmoved2.shp  -omdlyr Outletsmoved2 -md 20 

read outlet as sqlite write move outlet as sqlite file
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.sqlite -odlyr OutletstoMove  -omdsrc Outletsmoved2.sqlite  -omdlyr Outletsmoved2 -md 20 

read outlet as sqlite write move outlet as geojson file
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.sqlite -odlyr OutletstoMove  -omdsrc Outletsmoved2.geojson  -omdlyr Outletsmoved2.geojson -md 20 


read outlet as geojson write move outlet as shapefile
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.geojson -odlyr OGRgeojson  -omdsrc Outletsmoved3.shp  -omdlyr Outletsmoved3 -md 20 

read outlet as geojson write move outlet as sqlite file
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.geojson -odlyr OGRgeojson  -omdsrc Outletsmoved3.sqlite  -omdlyr Outletsmoved3 -md 20 

read outlet as geojson write move outlet as geojson file
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -odsrc OutletstoMove.geojson -odlyr OGRgeojson  -omdsrc Outletsmoved3.geojson  -omdlyr Outletsmoved3.geojson -md 20 


cd ..
write as shapefile
mpiexec -n 8 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -osrc loganOutlets1.shp  -olyr loganOutlets1 -odsrc loganOutlets_Moved1.shp -odlyr loganOutlets_Moved1.shp -d 1

write as sqlite file
mpiexec -n 8 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -osrc loganOutlets1.sqlite  -olyr loganOutlets1 -odsrc loganOutlets_Moved1.sqlite -odlyr loganOutlets_Moved1 -d 1

write as geojson file
mpiexec -n 8 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -osrc loganOutlets1.geojson  -olyr loganOutlets1.gojson -odsrc loganOutlets_Moved1.geojson -odlyr loganOutlets_Moved1.geojson -d 1

write outlet as shp but move outlet as sqlite
mpiexec -n 8 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -osrc loganOutlets2.shp  -olyr loganOutlets2 -odsrc loganOutlets_Moved2.sqlite -odlyr loganOutlets_Moved2 -d 1

write outlet as shp but move outlet as geojson
mpiexec -n 8 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -osrc loganOutlets3.shp  -olyr loganOutlets3.shp  -odsrc loganOutlets_Moved2.geojson -odlyr loganOutlets_Moved2.geojson -d 1

