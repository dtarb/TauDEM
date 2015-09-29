cd E:\USU_Research_work\MMW_PROJECT\TauDEM_Project\TauDEM_Geo_Deliverables\Tested_Results\GEO_TEST\New folder
Set TDIR=E:\USU_Research_work\MMW_PROJECT\TauDEM_Project\TauDEM_Geo_Deliverables\Taudem5PCVS2010Soln_512_long_int32\Taudem5PCVS2010\x64\Release
set MDIR1=C:\Program Files\Microsoft HPC Pack 2012\Bin\
set MDIR2=C:\Program Files\GDAL
set path=%MDIR1%;%MDIR2%;%TDIR%
Rem  Basic grid analysis
mpiexec -np 3 PitRemove enogeo.tif  
mpiexec -n 5 D8Flowdir -p enogeop.tif -sd8 enogeosd8.tif -fel enogeofel.tif 
mpiexec -n 4 DinfFlowdir -ang enogeoang.tif -slp enogeoslp.tif -fel enogeofel.tif
mpiexec -np 4 AreaD8 enogeo.tif
mpiexec -np 12 AreaDinf enogeo.tif

mpiexec -n 7 aread8 -p enogeop.tif -o Outlets.shp -ad8 enogeoad8o.tif
mpiexec -n 1 areadinf -ang enogeoang.tif -o Outlets.shp -sca enogeoscao.tif
mpiexec -n 5 Gridnet -p enogeop.tif -plen enogeoplen.tif -tlen enogeotlen.tif -gord enogeogord.tif 
mpiexec -n 5 Gridnet -p enogeop.tif -plen enogeoplen1.tif -tlen enogeotlen1.tif -gord enogeogord1.tif -mask enogeoad8.tif -thresh 100 
mpiexec -n 7 Gridnet -p enogeop.tif -plen enogeoplen2.tif -tlen enogeotlen2.tif -gord enogeogord2.tif -o Outlets.shp 

Rem stream network peuker douglas
mpiexec -np 7 PeukerDouglas -fel enogeofel.tif -ss enogeoss.tif
mpiexec -n 4 Aread8 -p enogeop.tif -o Outlets.shp -ad8 enogeossa.tif -wg enogeoss.tif
mpiexec -n 4 Dropanalysis -p enogeop.tif -fel enogeofel.tif -ad8 enogeoad8.tif -ssa enogeossa.tif -drp enogeodrp.txt -o Outlets.shp -par 5 500 10 0 
mpiexec -n 5 Threshold -ssa enogeossa.tif -src enogeosrc.tif -thresh 180
mpiexec -n 5 Streamnet -fel enogeofel.tif -p enogeop.tif -ad8 enogeoad8.tif -src enogeosrc.tif -ord enogeoord3.tif -tree enogeotree.dat -coord enogeocoord.dat -net enogeonet.shp -w enogeow.tif -o Outlets.shp

Rem stream network slope area
mpiexec -n 3 SlopeArea -slp enogeoslp.tif -sca enogeosca.tif -sa enogeosa.tif -par 2 1
mpiexec -n 8 D8FlowpathExtremeUp -p enogeop.tif -sa enogeosa.tif -ssa enogeossa1.tif -o Outlets.shp
mpiexec -n 4 Dropanalysis -p enogeop.tif -fel enogeofel.tif -ad8 enogeoad8.tif -ssa enogeossa1.tif -drp enogeodrp1.txt -o Outlets.shp -par 5000 50000 10 1 
mpiexec -n 5 Threshold -ssa enogeossa1.tif -src enogeosrc1.tif -thresh 15000
mpiexec -n 8 Streamnet -fel enogeofel.tif -p enogeop.tif -ad8 enogeoad8.tif -src enogeosrc1.tif -ord enogeoord5.tif -tree enogeotree1.dat -coord enogeocoord1.dat -net enogeonet1.shp -w enogeow1.tif -o Outlets.shp -sw
mpiexec -n 3 LengthArea -plen enogeoplen.tif -ad8 enogeoad8.tif -ss enogeolass.tif -par 0.03 1.3

Rem Specialized grid analysis
mpiexec -n 3 SlopeAreaRatio -slp enogeoslp.tif -sca enogeosca.tif -sar enogeosar.tif
mpiexec -np 7 D8HDisttoStrm -p enogeop.tif -src enogeoad8.tif -dist enogeodist1.tif -thresh 200 
mpiexec -np 5 D8HDisttoStrm -p enogeop.tif -src enogeosrc.tif -dist enogeodist.tif 

rem MOVEOUTLETS
mpiexec -n 5 Threshold -ssa enogeoad8.tif -src enogeosrc2.tif -thresh 200
mpiexec -np 3 MoveOutletstoStreams -p enogeop.tif -src enogeosrc.tif -o Outlets.shp -om Outletsmoved.shp -md 20 

rem DISTDOWN
mpiexec -n 1 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddhave.tif
mpiexec -n 2 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddhmin.tif -m min h
mpiexec -n 3 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddhmax.tif -m max h
mpiexec -n 4 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddvave.tif -m ave v
mpiexec -n 5 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddvmin.tif -m min v
mpiexec -n 6 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddvmax.tif -m max v
mpiexec -n 7 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddsave.tif -m ave s
mpiexec -n 8 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddsmin.tif -m min s
mpiexec -n 1 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddsmax.tif -m max s
mpiexec -n 2 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddpave.tif -m ave p
mpiexec -n 3 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddpmin.tif -m min p
mpiexec -n 4 dinfdistdown -ang enogeoang.tif -fel enogeofel.tif -src enogeosrc.tif -dd enogeoddpmax.tif -m max p

rem DISTANCE UP
mpiexec -n 1 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeoduhave.tif
mpiexec -n 2 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeoduhmin.tif -m min h -thresh 0.5
mpiexec -n 6 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeoduvmax.tif -m max v
mpiexec -n 7 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeodusave.tif -m ave s -thresh 0.9
mpiexec -n 8 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeodusmin.tif -m min s
mpiexec -n 1 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeodusmax.tif -m max s
mpiexec -n 2 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeodupave.tif -m ave p
mpiexec -n 3 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeodupmin.tif -m min p
mpiexec -n 4 dinfdistup -ang enogeoang.tif -fel enogeofel.tif -du enogeodupmax.tif -m max p

rem SLOPEAVEDOWN
mpiexec -n 3 slopeavedown -p enogeop.tif -fel enogeofel.tif -slpd enogeoslpd.tif

rem gagewatershed test
mpiexec -n 7 gagewatershed -p enogeop.tif -o Outletsmoved.shp -gw enogeogw.tif -id gwid.txt

