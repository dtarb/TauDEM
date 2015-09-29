cd D:\Save\Projects\TauDEM\Data\TestSuite\Result
Set TDIR=D:\Save\Projects\TauDEM\Programming\TauDEM5\Taudem5PCVS2010\x64\Release
set MDIR=C:\Program Files\Microsoft HPC Pack 2012\Bin\
set path=%MDIR%;%TDIR%

Rem  Basic grid analysis
mpiexec -np 3 PitRemove logan.tif
mpiexec -np 3 PitRemove -z logan.tif -fel loganfel4.tif -4way
mpiexec -np 3 PitRemove -z logan.tif -fel loganfeldm.tif -depmask logpitmask.tif
mpiexec -np 3 PitRemove -z logan.tif -fel loganfeldm4.tif -depmask logpitmask.tif -4way
mpiexec -n 5 D8Flowdir -p loganp.tif -sd8 logansd8.tif -fel loganfel.tif
mpiexec -n 4 DinfFlowdir -ang loganang.tif -slp loganslp.tif -fel loganfel.tif
mpiexec -n 6 D8Flowdir -p loganpnf.tif -sd8 logansd8nf.tif -fel logan.tif
mpiexec -n 3 DinfFlowdir -ang loganangnf.tif -slp loganslpnf.tif -fel logan.tif
mpiexec -np 4 AreaD8 logan.tif
mpiexec -np 12 AreaDinf logan.tif
mpiexec -np 4 AreaD8 -p loganpnf.tif -ad8 loganad8nf.tif
mpiexec -np 12 AreaDinf -ang loganangnf.tif -sca loganscanf.tif

mpiexec -n 7 aread8 -p loganp.tif -o outlet.shp -ad8 loganad8o.tif
mpiexec -n 1 areadinf -ang loganang.tif -o outlet.shp -sca loganscao.tif
mpiexec -n 5 Gridnet -p loganp.tif -plen loganplen.tif -tlen logantlen.tif -gord logangord.tif 
mpiexec -n 5 Gridnet -p loganp.tif -plen loganplen1.tif -tlen logantlen1.tif -gord logangord1.tif -mask loganad8.tif -thresh 100 
mpiexec -n 7 Gridnet -p loganp.tif -plen loganplen2.tif -tlen logantlen2.tif -gord logangord2.tif -o outlet.shp 

Rem stream network peuker douglas
mpiexec -np 7 PeukerDouglas -fel loganfel.tif -ss loganss.tif
mpiexec -n 4 Aread8 -p loganp.tif -o outlet.shp -ad8 loganssa.tif -wg loganss.tif
mpiexec -n 4 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa.tif -drp logandrp.txt -o outlet.shp -par 5 500 10 0 
mpiexec -n 5 Threshold -ssa loganssa.tif -src logansrc.tif -thresh 180
mpiexec -n 5 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc.tif -ord loganord3.tif -tree logantree.dat -coord logancoord.dat -net logannet.shp -w loganw.tif -o outlet.shp

Rem stream network slope area
mpiexec -n 3 SlopeArea -slp loganslp.tif -sca logansca.tif -sa logansa.tif -par 2 1
mpiexec -n 8 D8FlowpathExtremeUp -p loganp.tif -sa logansa.tif -ssa loganssa1.tif -o outlet.shp
mpiexec -n 4 Dropanalysis -p loganp.tif -fel loganfel.tif -ad8 loganad8.tif -ssa loganssa1.tif -drp logandrp1.txt -o outlet.shp -par 5000 50000 10 1 
mpiexec -n 5 Threshold -ssa loganssa1.tif -src logansrc1.tif -thresh 15000
mpiexec -n 8 Streamnet -fel loganfel.tif -p loganp.tif -ad8 loganad8.tif -src logansrc1.tif -ord loganord5.tif -tree logantree1.dat -coord logancoord1.dat -net logannet1.shp -w loganw1.tif -o outlet.shp -sw

mpiexec -n 3 LengthArea -plen loganplen.tif -ad8 loganad8.tif -ss loganlass.tif -par 0.03 1.3

Rem Specialized grid analysis
mpiexec -n 3 SlopeAreaRatio -slp loganslp.tif -sca logansca.tif -sar logansar.tif
mpiexec -np 7 D8HDisttoStrm -p loganp.tif -src loganad8.tif -dist logandist1.tif -thresh 200 
mpiexec -np 5 D8HDisttoStrm -p loganp.tif -src logansrc.tif -dist logandist.tif 

Rem downslope influence
mpiexec -np 1 AreaDinf -ang loganang.tif -wg logandg.tif -sca logandi.tif
mpiexec -n 2 dinfupdependence -ang loganang.tif -dg logandg.tif -dep logandep.tif
mpiexec -n 7 dinfdecayaccum -ang loganang.tif -dm logandm08.tif -dsca logandsca.tif 
mpiexec -n 3 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpt.tif -q logansca.tif 
mpiexec -n 5 dinfConcLimAccum -ang loganang.tif -dm logandm08.tif -dg logandg.tif -ctpt loganctpto.tif -q logansca.tif  -o outlet.shp -csol 2.4

Rem Trans lim accum
mpiexec -n 7 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla.tif -tdep logantdep.tif
mpiexec -n 6 dinfTransLimAccum -ang loganang.tif -tsup logantsup.tif -tc logantc.tif -tla logantla1.tif -tdep logantdep1.tif -o outlet.shp -cs logandg.tif -ctpt loganctpt1.tif 

rem REVERSE ACCUMULATION
mpiexec -n 4 dinfRevAccum -ang loganang.tif -wg logandg.tif -racc loganracc.tif -dmax loganrdmax.tif

rem MOVEOUTLETS
mpiexec -n 5 Threshold -ssa loganad8.tif -src logansrc2.tif -thresh 200
mpiexec -np 3 MoveOutletstoStreams -p loganp.tif -src logansrc.tif -o OutletstoMove.shp -om Outletsmoved.shp -md 20 

rem DISTDOWN
mpiexec -n 1 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddhave.tif
mpiexec -n 2 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddhmin.tif -m min h
mpiexec -n 3 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddhmax.tif -m max h
mpiexec -n 4 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddvave.tif -m ave v
mpiexec -n 5 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddvmin.tif -m min v
mpiexec -n 6 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddvmax.tif -m max v
mpiexec -n 7 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddsave.tif -m ave s
mpiexec -n 8 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddsmin.tif -m min s
mpiexec -n 1 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddsmax.tif -m max s
mpiexec -n 2 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddpave.tif -m ave p
mpiexec -n 3 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddpmin.tif -m min p
mpiexec -n 4 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddpmax.tif -m max p
mpiexec -n 2 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddvavenc.tif -m ave v -nc
mpiexec -n 3 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddhminnc.tif -m min h -nc
mpiexec -n 4 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddpmaxnc.tif -m max p -nc
mpiexec -n 4 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddsmaxnc.tif -m max s -nc
mpiexec -n 4 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddsmaxwg.tif -m max s -wg logandwg.tif
mpiexec -n 4 dinfdistdown -ang loganang.tif -fel loganfel.tif -src logansrc.tif -dd loganddhavewg.tif -m ave h -wg logandwg.tif

rem DISTANCE UP
mpiexec -n 1 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduhave.tif
mpiexec -n 2 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduhmin.tif -m min h -thresh 0.5
mpiexec -n 3 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduhmax.tif -m max h -thresh 0.8
mpiexec -n 4 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduvave.tif -m ave v
mpiexec -n 5 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduvmin.tif -m min v
mpiexec -n 6 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduvmax.tif -m max v
mpiexec -n 7 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandusave.tif -m ave s -thresh 0.9
mpiexec -n 8 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandusmin.tif -m min s
mpiexec -n 1 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandusmax.tif -m max s
mpiexec -n 2 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandupave.tif -m ave p
mpiexec -n 3 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandupmin.tif -m min p
mpiexec -n 4 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandupmax.tif -m max p
mpiexec -n 2 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduvavenc.tif -m ave v -nc
mpiexec -n 3 dinfdistup -ang loganang.tif -fel loganfel.tif -du loganduhminnc.tif -m min h -nc
mpiexec -n 4 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandupmaxnc.tif -m max p -nc
mpiexec -n 4 dinfdistup -ang loganang.tif -fel loganfel.tif -du logandusmaxnc.tif -m max s -nc

rem AVALANCHE
mpiexec -n 2 DinfAvalanche -ang loganang.tif -fel loganfel.tif -ass loganass.tif -rz loganrz.tif -dfs logandfs.tif
mpiexec -n 3 DinfAvalanche -ang loganang.tif -fel loganfel.tif -ass loganass.tif -rz loganrz1.tif -dfs logandfs1.tif -thresh 0.1 -alpha 10
mpiexec -n 4 DinfAvalanche -ang loganang.tif -fel loganfel.tif -ass loganass.tif -rz loganrz2.tif -dfs logandfs2.tif -direct -thresh 0.01 -alpha 5


rem SLOPEAVEDOWN
mpiexec -n 3 slopeavedown -p loganp.tif -fel loganfel.tif -slpd loganslpd.tif
mpiexec -n 3 slopeavedown -p loganp.tif -fel loganfel.tif -slpd loganslpd1.tif -dn 1000

rem test case for tiffio partitions within a stripe
mpiexec -n 6 areadinf -ang topo3fel1ang.tif -sca topo3fel1sca.tif 

rem test case for double precision file
mpiexec -n 4 areadinf -ang demDoubleang.tif -sca demDoublesca.tif -wg demDoublewgt.tif

rem gagewatershed test
mpiexec -n 7 gagewatershed -p loganp.tif -o Outletsmoved.shp -gw logangw.tif -id gwid.txt

cd fts
rem tests on ft steward data with stream buffer
mpiexec -n 3 pitremove fs_small.tif
mpiexec -n 4 dinfflowdir fs_small.tif
mpiexec -n 4 d8flowdir fs_small.tif
mpiexec -n 1 aread8 fs_small.tif
mpiexec -n 2 threshold -ssa fs_smallad8.tif -src fs_smallsrc.tif -thresh 500
mpiexec -n 5 dinfdistdown -ang fs_smallang.tif -fel fs_smallfel.tif -src fs_smallsrc.tif -dd fs_smallddhavewg.tif -m ave h -wg streambuffreclass2.tif

cd ..
rem test with compressed 16 bit unsigned integer that a user had trouble with
mpiexec -n 8 pitremove MED_01_01.tif
rem
mpiexec -n 8 pitremove -z LoganVRT\output.vrt -fel loganvrtfel.tif
rem test with img file format
mpiexec -n 8 pitremove -z loganIMG\logan.img -fel loganimgfel.tif
rem test with ESRIGRID file format
mpiexec -n 8 pitremove -z loganESRIGRID\logan -fel loganesrigridfel.tif
