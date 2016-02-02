cd E:\USU_Research_work\MMW_PROJECT\TauDEM_Project\OGR_Input_Test
Set TDIR=E:\USU_Research_work\MMW_PROJECT\TauDEM_Project\TauDEM_deploy_6\Taudem5PCVS2010\x64\Release
set MDIR=C:\Program Files\Microsoft HPC Pack 2012\Bin\
set GDIR=C:\Program Files\GDAL\
set path=%MDIR%;%TDIR%;%GDIR%

Rem  
cd AreaD8_data
mpiexec -n 7 aread8 -p loganp.tif -o Loganoutlet.shp -ad8 loganad8_1.tif

test using sqlite file
mpiexec -n 1 aread8 -p loganp.tif -o LoganSample.sqlite -lyrno 3 -ad8 loganad8_2.tif
mpiexec -n 3 aread8 -p loganp.tif -o LoganSample.sqlite -lyrname LoganOutlet -ad8 loganad8_3.tif


test using json file
mpiexec -n 4 aread8 -p loganp.tif -o LoganOutlet.json -ad8 loganad8_4.tif
mpiexec -n 6 aread8 -p loganp.tif -o LoganOutlet.json -lyrname OGRGeoJson -ad8 loganad8_5.tif

test using geodatabase file
mpiexec -n 7 aread8 -p loganp.tif -o Logan.gdb -ad8 loganad8_6.tif
mpiexec -n 5 aread8 -p loganp.tif -o Logan.gdb -lyrname Outlet -ad8 loganad8_7.tif
mpiexec -n 2 aread8 -p loganp.tif -o Logan.gdb -lyrno 0 -ad8 loganad8_8.tif

cd ../AreaDinf
Rem  
mpiexec -n 7 areadinf -ang loganang.tif -o Loganoutlet.shp -sca logansca_1.tif

test using sqlite file
mpiexec -n 1 areadinf -ang loganang.tif -o LoganSample.sqlite -lyrno 1 -sca logansca_2.tif
mpiexec -n 3 areadinf -ang loganang.tif -o LoganSample.sqlite -lyrname LoganOutlet -sca logansca_3.tif
mpiexec -n 5 areadinf -ang loganang.tif -o LoganSample.sqlite -lyrno 1 -sca logansca_4.tif

test using json file
mpiexec -n 3 areadinf -ang loganang.tif -o LoganOutlet.json -sca logansca_5.tif
mpiexec -n 5 areadinf -ang loganang.tif -o LoganOutlet.json -lyrname OGRGeoJson -sca logansca_6.tif

test using geodatabase file
mpiexec -n 5 areadinf -ang loganang.tif -o Logan.gdb -sca logansca_7.tif
mpiexec -n 3 areadinf -ang loganang.tif -o Logan.gdb -lyrname Outlet -sca logansca_8.tif
mpiexec -n 6 areadinf -ang loganang.tif -o Logan.gdb -lyrno 0 -sca logansca_9.tif



cd ../Gridnet

mpiexec -n 7 Gridnet -p loganp.tif -plen loganplen1.tif -tlen logantlen1.tif -gord logangord1.tif -o Loganoutlet.shp 
mpiexec -n 6 Gridnet -p loganp.tif -plen loganplen3.tif -tlen logantlen3.tif -gord logangord3.tif -o LoganSample.sqlite -lyrname LoganOutlet
mpiexec -n 3 Gridnet -p loganp.tif -plen loganplen4.tif -tlen logantlen4.tif -gord logangord4.tif -o LoganSample.sqlite -lyrno 1
mpiexec -n 3 Gridnet -p loganp.tif -plen loganplen5.tif -tlen logantlen5.tif -gord logangord5.tif -o LoganOutlet.json
mpiexec -n 7 Gridnet -p loganp.tif -plen loganplen6.tif -tlen logantlen6.tif -gord logangord6.tif -o LoganOutlet.json -lyrname OGRGeoJson 
mpiexec -n 1 Gridnet -p loganp.tif -plen loganplen7.tif -tlen logantlen7.tif -gord logangord7.tif -o Logan.gdb
mpiexec -n 3 Gridnet -p loganp.tif -plen loganplen8.tif -tlen logantlen8.tif -gord logangord8.tif -o Logan.gdb -lyrname Outlet
mpiexec -n 2 Gridnet -p loganp.tif -plen loganplen9.tif -tlen logantlen9.tif -gord logangord9.tif -o Logan.gdb -lyrno 0



Rem stream network peuker douglas
cd ../peukerDouglas
mpiexec -n 1 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp1.txt -o Loganoutlet.shp -par 5 500 10 0 
mpiexec -n 5 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp3.txt -o LoganSample.sqlite -lyrname LoganOutlet -par 5 500 10 0 
mpiexec -n 2 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp4.txt -o LoganSample.sqlite -lyrno 1 -par 5 500 10 0 
mpiexec -n 5 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp5.txt -o LoganOutlet.json -par 5 500 10 0 
mpiexec -n 4 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp6.txt -o LoganOutlet.json -lyrname OGRGeoJson  -par 5 500 10 0 
mpiexec -n 3 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp7.txt -o Logan.gdb -par 5 500 10 0 
mpiexec -n 5 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp8.txt -o Logan.gdb -lyrname Outlet -par 5 500 10 0 
mpiexec -n 6 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp9.txt -o Logan.gdb -lyrno 0 -par 5 500 10 0 


cd ../streamnet_data
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -net logannet1.shp -w loganw.tif -o Loganoutlet.shp
mpiexec -n 3 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -net LoganSample.sqlite -netlyr Mynetwork.shp -w loganw.tif -o LoganSample.sqlite -lyrname LoganOutlet
mpiexec -n 7 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -net logannet3.kml -netlyr logannet3 -w loganw.tif -o Logan.gdb -lyrname Outlet
mpiexec -n 7 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -net logannet3.json -netlyr logannet3 -w loganw.tif -o Logan.gdb -lyrno 0
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -net logannet4.json -netlyr logannet4 -w loganw.tif -o LoganOutlet.json -lyrno 0

Rem stream network slope area
cd ../D8flowextreme
mpiexec -n 3 D8FlowpathExtremeUp -p loganp.tif -sa logansa.tif -ssa loganssa1.tif -o Loganoutlet.shp
mpiexec -n 5 D8FlowpathExtremeUp -p loganp.tif -sa logansa.tif -ssa loganssa2.tif -o LoganSample.sqlite -lyrname Loganoutlet
mpiexec -n 7 D8FlowpathExtremeUp -p loganp.tif -sa logansa.tif -ssa loganssa3.tif -o LoganSample.sqlite -lyrno 1
mpiexec -n 1 D8FlowpathExtremeUp -p loganp.tif -sa logansa.tif -ssa loganssa4.tif -o Logan.gdb 
mpiexec -n 8 D8FlowpathExtremeUp -p loganp.tif -sa logansa.tif -ssa loganssa5.tif -o Logan.gdb -lyrno 0


Rem downslope influence
cd ../DinfConcLimAccum
mpiexec -n 1 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpto1.img -q logansca.tif  -o Loganoutlet.shp -csol 2.4
mpiexec -n 5 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpto2.tif -q logansca.tif  -o LoganSample.sqlite -lyrname Loganoutlet -csol 2.4
mpiexec -n 3 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpto3.tif -q logansca.tif  -o LoganSample.sqlite -lyrno 0 -csol 2.4
mpiexec -n 7 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpto4.tif -q logansca.tif  -o Logan.gdb -csol 2.4
mpiexec -n 8 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpto5.tif -q logansca.tif  -o Logan.gdb -lyrno 0 -csol 2.4

Rem Trans lim accum
cd ../DinfTransLimAcc
mpiexec -n 2 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla1.img -tdep logantdep1.tif -o Loganoutlet.shp -cs logandg.tif -ctpt loganctpt1.tif 
mpiexec -n 6 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla2.tif -tdep logantdep2.tif -o LoganSample.sqlite -lyrname Loganoutlet  -cs logandg.tif -ctpt loganctpt2.tif 
mpiexec -n 3 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla3.tif -tdep logantdep3.tif -o LoganSample.sqlite -lyrno 0 -cs logandg.tif -ctpt loganctpt3.tif 
mpiexec -n 5 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla4.tif -tdep logantdep4.tif -o Logan.gdb  -cs logandg.tif -ctpt loganctpt4.tif 
mpiexec -n 7 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla5.tif -tdep logantdep5.tif -o Loganoutlet.json -cs logandg.tif -ctpt loganctpt5.tif 


rem MOVEOUTLETS
cd ../MovedOutletstoStream_data
mpiexec -np 1 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -o OutletstoMove.shp -om Outletsmoved.shp -md 20 
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -o OutletstoMove.json -om Outletsmoved.kml -md 20 
mpiexec -np 5 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -o LoganSample.sqlite -om Outletsmoved.kml -md 20 
mpiexec -np 4 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -o LoganSample.sqlite -lyrno 2 -om Outletsmoved5.kml -md 20 
mpiexec -np 7 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -o Logan.gdb -lyrno 0 -om Outletsmove.json  Outletsmove -md 20 




rem gagewatershed test
cd ../GageWatershed
mpiexec -n 1 gagewatershed -p loganp.tif -o LoganOutlet.shp -gw logangw1.tif -id gwid1.txt
mpiexec -n 4 gagewatershed -p loganp.tif -o loganSample.sqlite -gw logangw2.tif -id gwid2.txt
mpiexec -n 3 gagewatershed -p loganp.tif -o Logan.gdb -gw logangw3.tif -id gwid3.txt
mpiexec -n 5 gagewatershed -p loganp.tif -o Logan.gdb -lyrno 0 -gw logangw3.tif -id gwid3.txt
mpiexec -n 7 gagewatershed -p loganp.tif -o loganoutlet.json -gw logangw4.tif -id gwid4.txt


rem Connect down
cd ../ConnectDown
mpiexec -n 1 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganOutlets1.shp -od loganOutlets_Moved1.shp -d 1
mpiexec -n 3 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganOutlets2.shp -olyr loganOutlets2  -od loganOutlets_Moved2.shp -odlyr loganOutlets2 -d 1
mpiexec -n 5 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganSample.sqlite -olyr myoutlet1  -od loganSample.sqlite -odlyr myoutlet2 -d 1
mpiexec -n 7 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganOutlets2.kml -od loganOutlets_Moved2.kml -d 1
mpiexec -n 1 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganOutlets3.kml -olyr loganOutlets3 -od loganOutlets_Moved3.kml -odlyr loganOutlets_Moved3 -d 1
mpiexec -n 3 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganOutlets4.json -od loganOutlets_Moved2.json -d 1
mpiexec -n 8 ConnectDown -p loganp.tif -ad8 loganad8.tif -w logangw.tif -o loganOutlets3.kml -olyr loganOutlets3 -od loganOutlets_Moved3.kml -odlyr loganOutlets_Moved3 -d 1



































rem  Geographic tests

cd Geographic
Rem  Basic grid analysis
mpiexec -n 7 aread8 -p enogeop.tif -o Outlets.shp -ad8 enogeoad8o.tif
mpiexec -n 1 areadinf -ang enogeoang.tif -o Outlets.shp -sca enogeoscao.tif

mpiexec -n 7 Gridnet -p enogeop.tif -plen enogeoplen2.tif -tlen enogeotlen2.tif -gord enogeogord2.tif -o Outlets.shp 

Rem stream network peuker douglas
mpiexec -n 4 Dropanalysis -p enogeop.tif -fel enogeofel.tif -ad8 enogeoad8.tif -ssa enogeossa.tif -drp enogeodrp.txt -o Outlets.shp -par 5 500 10 0 
mpiexec -n 5 Streamnet -fel enogeofel.tif -p enogeop.tif -ad8 enogeoad8.tif -src enogeosrc.tif -ord enogeoord3.tif -tree enogeotree.dat -coord enogeocoord.dat -net enogeonet.shp -w enogeow.tif -o Outlets.shp

Rem stream network slope area

mpiexec -n 8 D8FlowpathExtremeUp -p enogeop.tif -sa enogeosa.tif -ssa enogeossa1.tif -o Outlets.shp
mpiexec -n 4 Dropanalysis -p enogeop.tif -fel enogeofel.tif -ad8 enogeoad8.tif -ssa enogeossa1.tif -drp enogeodrp1.txt -o Outlets.shp -par 10 2000 10 1 
mpiexec -n 8 Streamnet -fel enogeofel.tif -p enogeop.tif -ad8 enogeoad8.tif -src enogeosrc1.tif -ord enogeoord5.tif -tree enogeotree1.dat -coord enogeocoord1.dat -net enogeonet1.shp -w enogeow1.tif -o Outlets.shp -sw
mpiexec -np 3 MoveOutletstoStreams -p enogeop.tif -src enogeosrc.tif -o Outlets.shp -om Outletsmoved.shp -md 20 

rem gagewatershed test
mpiexec -n 7 gagewatershed -p enogeop.tif -o Outletsmoved.shp -gw enogeogw.tif -id gwid.txt

