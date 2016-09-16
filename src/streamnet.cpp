/*  StreamNet function to compute stream networks based on d8 directions.
     
  David Tarboton, Dan Watson, Jeremy Neff
  Utah State University  
  May 23, 2010 

*/

/*  Copyright (C) 2010  David Tarboton, Utah State University

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License 
version 2, 1991 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the full GNU General Public License is included in file 
gpl.html. This is also available at:
http://www.gnu.org/copyleft/gpl.html
or from:
The Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
Boston, MA  02111-1307, USA.

If you wish to use or incorporate this program (or parts of it) into 
other software that does not meet the GNU General Public License 
conditions contact the author to request permission.
David G. Tarboton  
Utah State University 
8200 Old Main Hill 
Logan, UT 84322-8200 
USA 
http://www.engineering.usu.edu/dtarb/ 
email:  dtarb@usu.edu 
*/

//  This software is distributed from http://hydrology.usu.edu/taudem/

// 1/25/14.  Modified to use shapelib by Chris George




#include <mpi.h>
#include <math.h>
#include <iomanip>
#include <queue>

#include "commonLib.h"
//#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include "tardemlib.h"
#include "linklib.h"
#include "streamnet.h"
#include <fstream>
#include "ogr_api.h"
#include <limits>
using namespace std;

OGRSFDriverH    driver;
OGRDataSourceH  hDS1;
OGRLayerH       hLayer1;
OGRFeatureDefnH hFDefn1;
OGRFieldDefnH   hFieldDefn1;
OGRFeatureH     hFeature1;
OGRGeometryH    geometry, line;
OGRSpatialReferenceH  hSRSraster,hSRSshapefile;
//void createStreamNetShapefile(char *streamnetshp)
//{
//	shp1 = SHPCreate(streamnetshp, SHPT_ARC);
//	char streamnetdbf[MAXLN];
//	nameadd(streamnetdbf, streamnetshp, ".dbf");
//	dbf1 = DBFCreate(streamnetdbf);
//	linknoIdx = DBFAddField(dbf1,"LINKNO",FTInteger,6,0);
//	dslinknoIdx = DBFAddField(dbf1,"DSLINKNO",FTInteger,6,0);
//	uslinkno1Idx = DBFAddField(dbf1,"USLINKNO1",FTInteger,6,0);
//	uslinkno2Idx = DBFAddField(dbf1,"USLINKNO2",FTInteger,6,0);
//	dsnodeidIdx = DBFAddField(dbf1,"DSNODEID",FTInteger,12,0);
//	orderIdx = DBFAddField(dbf1,"Order",FTInteger,6,0);
//	lengthIdx = DBFAddField(dbf1,"Length",FTDouble,16,1);
//	magnitudeIdx = DBFAddField(dbf1,"Magnitude",FTInteger,6,0);
//	dscontareaIdx  = DBFAddField(dbf1,"DS_Cont_Area",FTDouble,16,1);
//	dropIdx = DBFAddField(dbf1,"Drop",FTDouble,16,2);
//	slopeIdx = DBFAddField(dbf1,"Slope",FTDouble,16,12);
//	straightlengthIdx = DBFAddField(dbf1,"Straight_Length",FTDouble,16,1);
//	uscontareaIdx = DBFAddField(dbf1,"US_Cont_Area",FTDouble,16,1);
//	wsnoIdx = DBFAddField(dbf1,"WSNO",FTInteger,6,0);
//	doutendIdx = DBFAddField(dbf1,"DOUT_END",FTDouble,16,1);
//	doutstartIdx = DBFAddField(dbf1,"DOUT_START",FTDouble,16,1);
//	doutmidIdx = DBFAddField(dbf1,"DOUT_MID",FTDouble,16,1);
//}


void createStreamNetShapefile(char *streamnetsrc,char *streamnetlyr,OGRSpatialReferenceH hSRSraster){
   
    /* Register all OGR drivers */
    OGRRegisterAll();
    //const char *pszDriverName = "ESRI Shapefile";
	const char *pszDriverName;
	pszDriverName=getOGRdrivername(streamnetsrc);
            
	//get driver by name
    driver = OGRGetDriverByName( pszDriverName );
    if( driver == NULL )
    {
        printf( "%s warning: driver not available.\n", pszDriverName );
        //exit( 1 );
    }

    // open datasource if the datasoruce exists 
	if(pszDriverName=="SQLite") hDS1 = OGROpen(streamnetsrc, TRUE, NULL );
	// create new data source if data source does not exist 
   if (hDS1 ==NULL){ 
	   hDS1 = OGR_Dr_CreateDataSource(driver, streamnetsrc, NULL);}
   else { hDS1=hDS1 ;}
	
    if (hDS1 != NULL) {
 

      // layer name is file name without extension
	 if(strlen(streamnetlyr)==0){
		char *streamnetlayername;
		streamnetlayername=getLayername(streamnetsrc); // get layer name if the layer name is not provided
	    hLayer1= OGR_DS_CreateLayer( hDS1,streamnetlayername,hSRSraster, wkbLineString, NULL );} 

	 else {
		 hLayer1= OGR_DS_CreateLayer( hDS1,streamnetlyr,hSRSraster, wkbLineString, NULL ); }// provide same spatial reference as raster in streamnetshp file
    if( hLayer1 == NULL )
    {
        printf( "warning: Layer creation failed.\n" );
        //exit( 1 );
    }
 
	
	
	/* Add a few fields to the layer defn */ //need some work for setfiled width
	// add fields, field width and precision
    hFieldDefn1 = OGR_Fld_Create( "LINKNO", OFTInteger ); // create new field definition
	OGR_Fld_SetWidth( hFieldDefn1, 6); // set field width
	OGR_L_CreateField(hLayer1,  hFieldDefn1, 0); // create field 

    hFieldDefn1 = OGR_Fld_Create( "DSLINKNO", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 6);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	 
	hFieldDefn1 = OGR_Fld_Create( "USLINKNO1", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 6);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "USLINKNO2", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 6);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "DSNODEID", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 12);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "strmOrder", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 6);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "Length", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
	OGR_Fld_SetPrecision(hFieldDefn1, 1); // set precision
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "Magnitude", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 6);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	 
	hFieldDefn1 = OGR_Fld_Create( "DSContArea", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
	OGR_Fld_SetPrecision(hFieldDefn1, 1);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "strmDrop", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
	OGR_Fld_SetPrecision(hFieldDefn1, 2);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	 
	hFieldDefn1= OGR_Fld_Create( "Slope", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1,16);
    OGR_Fld_SetPrecision(hFieldDefn1, 12);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "StraightL", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1,16);
	OGR_Fld_SetPrecision(hFieldDefn1, 1);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	
	hFieldDefn1 = OGR_Fld_Create( "USContArea", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
	OGR_Fld_SetPrecision(hFieldDefn1, 1);
    OGR_L_CreateField(hLayer1,hFieldDefn1, 0);
	 
	hFieldDefn1 = OGR_Fld_Create( "WSNO", OFTInteger );
	OGR_Fld_SetWidth( hFieldDefn1, 6);
	OGR_Fld_SetPrecision(hFieldDefn1,0);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	 
	hFieldDefn1 = OGR_Fld_Create( "DOUTEND", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
	OGR_Fld_SetPrecision(hFieldDefn1, 1);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	 
	hFieldDefn1 = OGR_Fld_Create( "DOUTSTART", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
    OGR_Fld_SetPrecision(hFieldDefn1, 1);
    OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	 
	hFieldDefn1 = OGR_Fld_Create( "DOUTMID", OFTReal );
	OGR_Fld_SetWidth( hFieldDefn1, 16);
    OGR_Fld_SetPrecision(hFieldDefn1, 1);
	OGR_L_CreateField(hLayer1,  hFieldDefn1, 0);
	}
	}
// Write shape from tardemlib.cpp
int reachshape(long *cnet,float *lengthd, float *elev, float *area, double *pointx, double *pointy, long np,tiffIO &obj)
{
// Function to write stream network shapefile
	int nVertices;
	if (np < 2) {//singleton - will be duplicated
		nVertices = 2;
	}
	else {
		nVertices = np;
	}
	double *mypointx = new double[nVertices];
	double *mypointy = new double[nVertices];
	double x,y,length,glength,x1,y1,xlast,ylast,usarea,dsarea,dslast,dl,drop,slope;
	int istart,iend,j;

	istart=cnet[1];  //  start coord for first link
	iend=cnet[2];//  end coord for first link
	x1=pointx[0];
	y1=pointy[0];
	length=0.;
	xlast=x1;
	ylast=y1;
	usarea=area[0];
	dslast=usarea;
	dsarea=usarea;
	long prt = 0;

	for(j=0; j<np; j++)  //  loop over points
	{
		x=pointx[j];
		y=pointy[j];
		// we have to reverse order of pointx and pointy arrays to finish up with 
		// the point with point index 0 in the shape being the outlet point
		// (which is for backwards compatibility with previous versions of TauDEM)
		int i = np - (j + 1);
		mypointx[i] = x;
		mypointy[i] = y;
		if(obj.getproj()==1){
			double dlon1,dlat1;double dxdyc1[2];
			dlon1=x-xlast;dlat1=y-ylast;
			obj.geotoLength(dlon1,dlat1,y,dxdyc1);
			dl=sqrt(dxdyc1[0]*dxdyc1[0]+dxdyc1[1]*dxdyc1[1]);
		}
		  else if(obj.getproj()==0){
		  dl=sqrt((x-xlast)*(x-xlast)+(y-ylast)*(y-ylast));
		}

       if(dl > 0.)
		{
			length=length+dl;
			xlast=x;  ylast=y;
			dsarea=dslast;   // keeps track of last ds area
			dslast=area[j];
		}
	}
	drop=elev[0]-elev[np-1];
	slope=0.;
	float dsdist=lengthd[np-1];
	float usdist=lengthd[0];
	float middist=(dsdist+usdist)*0.5;
	if(length > 0.)slope=drop/length;

	if(obj.getproj()==1){
           	double dlon2,dlat2;double dxdyc2[2];
			dlon2=x-x1;dlat2=y-y1;
			obj.geotoLength(dlon2,dlat2,y,dxdyc2);
			glength=sqrt(dxdyc2[0]*dxdyc2[0]+dxdyc2[1]*dxdyc2[1]);

		}
		  else if(obj.getproj()==0){
		  glength=sqrt((x-x1)*(x-x1)+(y-y1)*(y-y1));
		}

	// ensure at least two points (assuming have at least 1) by repeating singleton
	if (np < 2) {
		mypointx[1] = mypointx[0];
		mypointy[1] = mypointy[0];
	}

	//SHPObject *shape = SHPCreateSimpleObject(
	//	SHPT_ARC,						// type
	//	nVertices,						// number of vertices
	//	mypointx,						// X values
	//	mypointy,						// Y values
	//	NULL);							// Z values

	hFDefn1 = OGR_L_GetLayerDefn( hLayer1 );
	hFeature1 = OGR_F_Create( hFDefn1 );
	OGR_F_SetFieldInteger( hFeature1, 0, (int)cnet[0]); // set field value 
	OGR_F_SetFieldInteger( hFeature1, 1, (int)cnet[3]);
	OGR_F_SetFieldInteger( hFeature1, 2, (int)cnet[4]);
	OGR_F_SetFieldInteger( hFeature1, 3, (int)cnet[5]);
	OGR_F_SetFieldInteger( hFeature1, 4, (int)cnet[7]);
	OGR_F_SetFieldInteger( hFeature1, 5, (int)cnet[6]);
	OGR_F_SetFieldDouble( hFeature1, 6, length);
	OGR_F_SetFieldInteger( hFeature1, 7, (int)cnet[8]);
	OGR_F_SetFieldDouble( hFeature1, 8,  dsarea);
	OGR_F_SetFieldDouble( hFeature1, 9, drop);
	OGR_F_SetFieldDouble( hFeature1, 10, slope);
	OGR_F_SetFieldDouble( hFeature1, 11, glength);
	OGR_F_SetFieldDouble( hFeature1, 12, usarea);
	OGR_F_SetFieldInteger( hFeature1, 13, (int)cnet[0]);
	OGR_F_SetFieldDouble( hFeature1, 14, dsdist);
	OGR_F_SetFieldDouble( hFeature1, 15, usdist);
	OGR_F_SetFieldDouble( hFeature1, 16, middist);

    //creating geometry using OGR

	line = OGR_G_CreateGeometry( wkbLineString );
    for(j=0; j<np; j++) {
		OGR_G_AddPoint(line, mypointx[j], mypointy[j], 0);
	}
	OGR_F_SetGeometryDirectly(hFeature1,line); // set geometry to feature
    OGR_L_CreateFeature( hLayer1, hFeature1 ); //adding feature 
	
	delete[] mypointx;
    delete[] mypointy;
	
return 0;
}

struct Slink{
	long id;
	short dest;
};

int netsetup(char *pfile,char *srcfile,char *ordfile,char *ad8file,char *elevfile,char *treefile, char *coordfile, 
			 char *outletsds, char *lyrname, int uselayername,int lyrno, char *wfile, char *streamnetsrc, char *streamnetlyr,long useOutlets, long ordert, bool verbose) 
{
	// MPI Init section
	MPI_Init(NULL,NULL);{
		int rank,size;
		MPI_Comm_rank(MCW,&rank);
		MPI_Comm_size(MCW,&size);
		if(rank==0)
			{
				printf("StreamNet version %s\n",TDVERSION);
				fflush(stdout);
		}

		//  Used throughout
		float tempFloat;
		int32_t tempLong;
		short tempShort;
		double tempdxc,tempdyc;
		//  Keep track of time
		double begint = MPI_Wtime();

		//  *** initiate src grid partition from src
		tiffIO srcIO(srcfile, SHORT_TYPE);  
		long TotalX = srcIO.getTotalX();
		long TotalY = srcIO.getTotalY();
		double dxA = srcIO.getdxA();
		double dyA = srcIO.getdyA();

	    hSRSraster=srcIO.getspatialref();
        //double diag=sqrt((dx*dx)+(dy*dy));
		
	

		if(rank==0)
		{
			float timeestimate=(1e-4*TotalX*TotalY/pow((double) size,0.2))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


		//Create partition and read data
		tdpartition *src;
		src = CreateNewPartition(srcIO.getDatatype(), TotalX, TotalY, dxA, dyA, srcIO.getNodata());
		int nx = src->getnx();
		int ny = src->getny();
		int xstart, ystart;  
		src->localToGlobal(0, 0, xstart, ystart);  
		src->savedxdyc(srcIO);
		srcIO.read((long)xstart, (long)ystart, (long)ny, (long)nx, src->getGridPointer());

		//  *** initiate flowdir grid partition from dirfile
		tiffIO dirIO(pfile, SHORT_TYPE);
		if(!dirIO.compareTiff(srcIO)){
			printf("pfile and src files not the same size. Exiting \n");
			MPI_Abort(MCW,4);
		}
		//Create partition and read data
		tdpartition *flowDir;
		flowDir = CreateNewPartition(dirIO.getDatatype(), TotalX, TotalY, dxA, dyA, dirIO.getNodata());
		flowDir->savedxdyc(dirIO);
		dirIO.read((long)xstart, (long)ystart, (long)ny, (long)nx, flowDir->getGridPointer());

		tiffIO ad8IO(ad8file, FLOAT_TYPE);
		if(!ad8IO.compareTiff(srcIO)){
			printf("ad8file and src files not the same size. Exiting \n");
			MPI_Abort(MCW,4);
		}
		//Create partition and read data
		tdpartition *areaD8;
		areaD8 = CreateNewPartition(ad8IO.getDatatype(), TotalX, TotalY, dxA, dyA, ad8IO.getNodata());
		ad8IO.read((long)xstart, (long)ystart, (long)ny, (long)nx, areaD8->getGridPointer());

		tiffIO elevIO(elevfile, FLOAT_TYPE);
		if(!elevIO.compareTiff(srcIO)){
			printf("elevfile and src files not the same size. Exiting \n");
			MPI_Abort(MCW,4);
		}
		//Create partition and read data
		tdpartition *elev;
		elev = CreateNewPartition(elevIO.getDatatype(), TotalX, TotalY, dxA, dyA, elevIO.getNodata());
		elevIO.read((long)xstart, (long)ystart, (long)ny, (long)nx, elev->getGridPointer());


		//Create empty partition to store new ID information
		tdpartition *idGrid;
		idGrid = CreateNewPartition(LONG_TYPE, TotalX, TotalY, dxA, dyA, MISSINGLONG);
		//Create empty partition to store number of contributing neighbors
		tdpartition *contribs;
		contribs = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dxA, dyA, MISSINGSHORT);
		//Create empty partition to stream order
		tdpartition *wsGrid;
		wsGrid = CreateNewPartition(LONG_TYPE, TotalX, TotalY, dxA, dyA, MISSINGLONG);

		makeLinkSet();

		long i,j,inext,jnext;
		int *xOutlets;
		int *yOutlets;
		int numOutlets=0;
		double *x=NULL;
		double *y=NULL;
		int *ids=NULL;
		// Read outlets 
		if( useOutlets == 1) {
			if(rank==0){
				if(readoutlets(outletsds,lyrname,uselayername,lyrno,hSRSraster, &numOutlets, x, y,ids) !=0){
					printf("Exiting \n");
					MPI_Abort(MCW,5);
				}else {
					MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
					MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
					MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
					MPI_Bcast(ids, numOutlets, MPI_INT, 0, MCW);
				}
			}
			else {
				MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);

				//x = (double*) malloc( sizeof( double ) * numOutlets );
				//y = (double*) malloc( sizeof( double ) * numOutlets );
				x = new double[numOutlets];
				y = new double[numOutlets];
				ids = new int[numOutlets];

				MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(ids, numOutlets, MPI_INT, 0, MCW);
			}


			xOutlets = new int[numOutlets];
			yOutlets = new int[numOutlets];
		//  Loop over all points in shapefile.  If they are in the partition (after conversion from 
		//   geographic to global, then global to partition coordinates) set the idGrid to hold ids value
		//   leaving the remainder of idGrid to no data
			for(i = 0; i<numOutlets;i++){
				ad8IO.geoToGlobalXY(x[i], y[i], xOutlets[i], yOutlets[i]);
				int xlocal, ylocal;
				idGrid->globalToLocal(xOutlets[i], yOutlets[i],xlocal,ylocal);
				if(idGrid->isInPartition(xlocal,ylocal)){   //xOutlets[i], yOutlets[i])){
					idGrid->setData(xlocal,ylocal,(int32_t)ids[i]);  //xOutlets[i], yOutlets[i], (long)ids[i]);
				}
			}
			idGrid->share();
		}

		// Timer read time
		double readt = MPI_Wtime();
		if(verbose)  // debug writes
		{
			printf("Files read\n");
			printf("Process: %d, TotalX: %ld, TotalY: %ld\n",rank,TotalX,TotalY);
			printf("Process: %d, nx: %d, ny: %d\n",rank,nx,ny);
			printf("Process: %d, xstart: %d, ystart: %d\n",rank,xstart,ystart);
			printf("Process: %d, numOutlets: %d\n",rank,numOutlets);
			fflush(stdout);
		}

//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//  Block to evaluate distance to outlet

		//Create empty partition to store lengths
		tdpartition *lengths;
		lengths = CreateNewPartition(FLOAT_TYPE, TotalX, TotalY, dxA, dyA, MISSINGFLOAT);

		src->share();
		flowDir->share();
		if(rank==0)
		{
			fprintf(stderr,"Evaluating distances to outlet\n");
			fflush(stderr);
		}


//  Initialize queue and contribs partition	
		queue <node> que;
// Initialize queue for links that will be sent.
		queue <Slink> linkQ;
		long Sup = 0;
		long Sdown = 0;
		Slink temp;
		node t;
		int p;
		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				// If I am on stream and my downslope neighbor is on stream contribs is 1
				// If I am on stream and my downslope neighbor is off stream contribs is 0 because 
				//  I am at the end of a stream
				if(!src->isNodata(i,j) && src->getData(i,j,tempShort)>0 && !flowDir->isNodata(i,j))
				{
					p=flowDir->getData(i,j,tempShort);
					inext=i+d1[p];
					jnext=j+d2[p];
					if(!src->isNodata(inext,jnext) && src->getData(inext,jnext,tempShort)>0 && !flowDir->isNodata(inext,jnext))
						contribs->setData(i,j,(short)1);//addData? What if it is a junction and has more floing into it?
					else
					{
						contribs->setData(i,j,(short)0);
						t.x=i;
						t.y=j;
						que.push(t);
					}
				}
			}
		}
// while loop where each process empties its que, then shares border info, and repeats till everyone is done
		bool finished=false;
		int m;
		//int totalP=0;
		while(!finished){
			contribs->clearBorders();  
			while(!que.empty()){
				t=que.front(); 
				i=t.x; 
				j=t.y;
				que.pop();
				p=flowDir->getData(i,j,tempShort);
				flowDir->getdxdyc(j,tempdxc,tempdyc);
				float llength=0.;
				inext=i+d1[p];
				jnext=j+d2[p];
				if(!lengths->isNodata(inext,jnext))
				{
					llength += lengths->getData(inext,jnext,tempFloat);

					if(p==1||p==5)llength=llength+tempdxc; // make sure that it is correct
					if(p==3||p==7)llength=llength+tempdyc;
					if(p%2==0)llength=llength+sqrt(tempdxc*tempdxc+tempdyc*tempdyc);
				}
				lengths->setData(i,j,llength);
				//totalP++;
				//  Find if neighbor points to me and reduce its dependency by 1
				for(m=1;m<=8;++m){
					inext=i+d1[m];
					jnext=j+d2[m];
					if(pointsToMe(i,j,inext,jnext,flowDir) && !src->isNodata(inext,jnext)  && src->getData(inext,jnext,tempShort)>0)
					{
						contribs->addToData(inext,jnext,(short)(-1));
						if(contribs->isInPartition(inext,jnext) && contribs->getData(inext,jnext,tempShort)==0)
						{
							t.x=inext;
							t.y=jnext;
							que.push(t);
						}
					}
				}
			}
			//Pass information across partitions

			contribs->addBorders();
			lengths->share();

			//If this created a cell with no contributing neighbors, put it on the queue
			for(i=0; i<nx; i++){
				if(contribs->getData(i, -1, tempShort)!=0 && contribs->getData(i, 0, tempShort)==0)
				{
					t.x = i;
					t.y = 0;
					que.push(t);
				}
				if(contribs->getData(i, ny, tempShort)!=0 && contribs->getData(i, ny-1, tempShort)==0)
				{
					t.x = i;
					t.y = ny-1;
					que.push(t); 
				}
			}

			//Check if done
			finished = que.empty();
			finished = contribs->ringTerm(finished);
		}

		// Timer lengtht
		double lengthct = MPI_Wtime();
		if(rank==0)
		{
			fprintf(stderr,"Creating links\n");
			fflush(stderr);
		}

		if(verbose)
			cout << rank << " Starting block to create links"  << endl;

//  Now length partition is evaluated

//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//  Block to create links

//  Reinitialize contribs partition
		delete contribs;
		contribs = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dxA, dyA, MISSINGSHORT);
		long k = 0;

		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				// If I am on stream add 1 for each neighbor on stream that contribs to me
				if(!src->isNodata(i,j) && src->getData(i,j,tempShort)>0 && !flowDir->isNodata(i,j))
				{
					short tempc=0;  
					for(k=1;k<=8;k++)
					{
						inext=i+d1[k];
						jnext=j+d2[k];
						if(pointsToMe(i,j,inext,jnext,flowDir) && !src->isNodata(inext,jnext)  && src->getData(inext,jnext,tempShort)>0)
						{
							tempc=tempc+1;  
						}
					}
					contribs->setData(i,j,tempc);
					if(tempc==0)
					{
						t.x=i;
						t.y=j;
						que.push(t);
					}
				}
			}
		}
		//  Indicate progress by about 10 dots per loop
		long nque=que.size();
		long deltaque=nque/10+10;  // + 10 to not give so many dots for small jobs


		if(verbose)
			cout << rank << "	Starting main link creation loop"  << endl;
//XXXXXXXXXXXX  MAIN WHILE LOOP
// While loop where each process empties its que, then shares border info, and repeats till everyone is done
		finished=false;
		while(!finished){
			bool linksToReceive=true;
			nque=que.size(); //  used for indicating progress
			contribs->clearBorders();
			while(!que.empty()){
				t=que.front();
				i=t.x;
				j=t.y;
				que.pop();
				//if(que.size() < nque)
				//{
				//	nque=nque-deltaque;
				//	fprintf(stderr,".");
				//	fflush(stderr);
				//}
				// begin cell processing
				short nOrder[8];  // neighborOrders
				short inneighbors[8];  // inflowing neighbors
				bool junction; // junction set to true/false in newOrder
				int32_t Id;

				//  Count number of grid cells on stream that point to me.  
				// (There will be no more than 8, usually only 0, 1,2 or 3.  The queue and 
				// processing order ensures that the necessary results have already been 
				//  evaluated for these.  In this case it should be a partially complete link 
				//  object referenced by a value in a link object partition).  Number that flow 
				//  to cell is k.  Record the order and direction to each. 
				k=0;
				for(m=1;m<=8;++m){
					inext=i+d1[m];
					jnext=j+d2[m];
					if(pointsToMe(i,j,inext,jnext,flowDir) && !src->isNodata(inext,jnext) && src->getData(inext,jnext,tempShort)>0 && !idGrid->isNodata(inext,jnext)){//!idGrid->isNodata(inext,jnext)){
						inneighbors[k]=m;  
						//  stream orders are stored in lengths grid to save memory
						nOrder[k]= (short)lengths->getData(inext,jnext,tempFloat); ///  Assign value//  Get the order from the link referenced by the value in idGrid and //   save it in nOrder[m-1]
						k=k+1;  // count inflows
					}
				}

				// Determine downslope neighbor
				long nextx,nexty;
				p=flowDir->getData(i,j,tempShort);
				nextx=t.x+d1[p];
				nexty=t.y+d2[p];

				// Determine if mandatory junction indicated by a value in idGrid
				bool mandatoryJunction=false;
				int32_t shapeID;
				if(!idGrid->isNodata(i,j))
				{
					mandatoryJunction=true;
					shapeID=idGrid->getData(i,j,shapeID);
				}

				//-	determine if terminal link by if downslope neighbor is out of domain
				bool terminal = false;
				if(!src->hasAccess(nextx,nexty)  || src->isNodata(nextx,nexty) 
					|| (src->getData(nextx,nexty,tempShort) != 1))
					terminal = true;

				//  instantiate point
				point* addPoint;
				addPoint = initPoint(elev,areaD8,lengths,i,j);

				//  Case for start of flow path
				if(k==0){
					//  This is the start of a stream - instantiate new link
					streamlink *thisLink;
					thisLink = createLink(-1, -1, -1, addPoint); //,1); //, dx,dy);
					int32_t u1= thisLink->Id;
					idGrid->setData(i,j, u1);
					wsGrid->setData(i,j, u1);
					lengths->setData(i,j,(float)thisLink->order);

					//  Handle mandatory junction
					if(mandatoryJunction){
						appendPoint(u1, addPoint);
						thisLink->shapeId = shapeID;
						if(!terminal){
							streamlink *newLink;
							newLink = createLink(u1,-1,-1, addPoint); //, 1); //, dx,dy);
							newLink->order =  1;	
							setDownLinkId(u1,newLink->Id);		
							newLink->magnitude = 1;
							terminateLink(u1);
							idGrid->setData(i,j,newLink->Id);
							//  Here idGrid and wsGrid intentionally out of sync
							//  idGrid to be used to retrieve upstream link for next cell down
							//  wsGrid used to hold the id for delineation of watersheds which 
							//  is the link that ends at this point
						}
					}else if(terminal){  // handle terminal that is not mandatory junction
						appendPoint(u1, addPoint);
					}
				}

				//  Case for ongoing flow path with single inflow
				else if(k==1){
					int32_t u1=idGrid->getData(i+d1[inneighbors[0]],j+d2[inneighbors[0]],tempLong);
					appendPoint(u1, addPoint);
					streamlink *thisLink;
					//TODO DGT Thinks
					// thisLink=u1;  for efficiency
					thisLink=FindLink(u1);
					//cout << "append done"  << endl;
					idGrid->setData(i,j,u1);
					wsGrid->setData(i,j,u1);
					lengths->setData(i,j,(float)nOrder[0]);

					//  Handle mandatory junction case
					if(mandatoryJunction){
						thisLink->shapeId = shapeID;
						if(!terminal){
							streamlink *newLink;
							newLink = createLink(u1,-1,-1, addPoint); //, 1); //, dx,dy);
							newLink->order =  thisLink->order;	
							setDownLinkId(u1,newLink->Id);		
							newLink->magnitude = thisLink->magnitude;
							terminateLink(u1);
							idGrid->setData(i,j,newLink->Id);
							//  TODO Here need to assign WSNO = -1
						}
					}
				}

				// Case for multiple inflows
				else if(k>1){
					//	rank incoming flow paths from highest to lowest order and process in this order
					//cout << "k>1 junction: "  << k << endl;
					//Dans Code to sort the two largest to the bottom of the list
					short temp;
					for(int a=0;a<2;++a){
						for(int b=0;b<k-1;++b){
							if(nOrder[b]>nOrder[b+1]){
								temp=nOrder[b];
								nOrder[b]=nOrder[b+1];
								nOrder[b+1]=temp;

								temp = inneighbors[b];
								inneighbors[b] = inneighbors[b+1];
								inneighbors[b+1] = temp;//keep track of where they came from. 
							}
						}
					}	
					// Determine order out
					short oOut;//if the last two have the same order, there is a junction, so bump the order
					if(nOrder[k-2]==nOrder[k-1]){
							//junction=true;
						oOut=nOrder[k-1]+1;
					}else{
						oOut=nOrder[k-1];
					}
					int32_t u1, u2;
					u1 = idGrid->getData(i+d1[inneighbors[k-1]],j+d2[inneighbors[k-1]],tempLong);
					u2 = idGrid->getData(i+d1[inneighbors[k-2]],j+d2[inneighbors[k-2]],tempLong);
					appendPoint(u1, addPoint);
					appendPoint(u2, addPoint);
					streamlink *newLink;
					newLink = createLink(u1,u2,-1,addPoint); //,1); //,dx,dy);
					newLink->order = oOut;
					setDownLinkId(u1,newLink->Id);
					setDownLinkId(u2,newLink->Id);
					newLink->magnitude = getMagnitude(u1) + getMagnitude(u2);
					
					//  TODO add WSNO = -1

					terminateLink(u1);
					terminateLink(u2);

					//  Handle remaining incoming flow paths
					for(m=2; m<k; m++)   //  Loop is only entered if k>=3
					{
						u1 = newLink->Id;
						u2 = idGrid->getData(i+d1[inneighbors[k-m-1]],j+d2[inneighbors[k-m-1]],tempLong);

						appendPoint(u1, addPoint);
						appendPoint(u2, addPoint);

						// create a new link
						newLink = createLink(u1,u2,-1,addPoint); //,1);  //,dx,dy);
						//  set order of new link as order out
						newLink->order = oOut;
						//  set u1 to previously created link
						setDownLinkId(u1, newLink->Id);
						setDownLinkId(u2, newLink->Id);
						newLink->magnitude = getMagnitude(u1) + getMagnitude(u2);
						
						//  TODO add WSNO = -1

						terminateLink(u1);
						terminateLink(u2);
					}
					idGrid->setData(i,j,newLink->Id);// Set the idGrid to the new link ID?
					wsGrid->setData(i,j,newLink->Id);
					lengths->setData(i,j,(float)oOut);
					if(mandatoryJunction){
						// create a zero length link to the mandatory junction
						u1 = newLink->Id;
						appendPoint(u1, addPoint);
						newLink->shapeId=shapeID;

						//  TODO assign u1 to WSNO
						if(!terminal){
							newLink = createLink(u1,-1,-1,addPoint); //,1); //,dx,dy);
							//  set order of new link as order out
							newLink->order = oOut;
							//  set u1 to previously created link
							setDownLinkId(u1, newLink->Id);
							newLink->magnitude = getMagnitude(u1);
							idGrid->setData(i,j,newLink->Id);// Set the idGrid to the new link ID?
							//  TODO assign -1 to WNSO
						}
					}else if(terminal){
						u1 = newLink->Id;
						appendPoint(u1, addPoint);
						//  TODO assign u1 to WSNO
					}
				}
				//Check if down neighbor needs to be added to que
				contribs->addToData(nextx,nexty,(short)-1);
				if(contribs->isInPartition(nextx,nexty))
				{
					if(contribs->getData(nextx, nexty, tempShort) == 0 ){
						t.x=nextx;
						t.y=nexty;
						que.push(t);
					}
				}
				//  IF link is to be sent
				//     ID which process to go to (rank - 1 or rank + 1)
				//     Add id to a list for that process
				//     count number for that process
				else 
				{
					//  Package up the link that either began or is in process at cell i,j and send to downstream partition
					//  Link has to be sent
					if(nexty<0 && rank>0)  // up
					{
						temp.id = idGrid->getData(i,j,tempLong);
						temp.dest = rank-1;
						linkQ.push(temp);
						Sup++;
					}
					if(nexty>=ny && rank < size-1)
					{
						temp.id = idGrid->getData(i,j,tempLong);
						temp.dest = rank+1;
						linkQ.push(temp);
						Sdown++;
					}
				}
			}
			if(verbose)
			{
				cout << rank << " End link creation pass - about to share" << endl;
				MPI_Status stat;
        		int messageFlag = false;
        		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &messageFlag, &stat);
       		 	if(messageFlag == true){
                		cout << rank << ": I have a message waiting before I try to pass links!!!" << stat.MPI_TAG << endl;
                		MPI_Abort(MCW,4);
        		}
				MPI_Barrier(MCW);
			}

//  Block to swap links
			if(size > 1)
			{
				long numRecv;
				MPI_Status stat;
				if(rank < size-1){//only send down
					MPI_Send(&Sdown,1,MPI_LONG,rank+1,0,MCW);//send message saying done
					for(int j =0; j<Sdown; j++){
						temp.id = linkQ.front().id;
						temp.dest = linkQ.front().dest;
						linkQ.pop();//all an the q should go to the same dest.
						while(temp.dest != rank+1){
							linkQ.push(temp);
							temp.id = linkQ.front().id;
							temp.dest = linkQ.front().dest;
							linkQ.pop();//all an the q should go to the same dest.
						}
						sendLink(temp.id,temp.dest);
					}
				}
				if(rank >0)
				{
					MPI_Recv(&numRecv,1,MPI_LONG,rank-1,0,MCW,&stat);
					for(int j =0 ;j<numRecv;j++){
						recvLink(rank-1);//recv from up.
					}
				}
				if(rank > 0)
				{
					MPI_Send(&Sup,1,MPI_LONG,rank-1,0,MCW);//send message saying up
					for(int j =0; j<Sup; j++){
						temp.id = linkQ.front().id;
						temp.dest = linkQ.front().dest;
						linkQ.pop();//all an the q should go to the same dest.
						sendLink(temp.id,temp.dest);
					}
				}
				if(rank < size -1)
				{
					MPI_Recv(&numRecv,1,MPI_LONG,rank+1,0,MCW,&stat);
					for(int j = 0;j < numRecv; j++){
						recvLink(rank+1);//recv from below.
					}
				}

				//MPI_Barrier(MCW);
				Sup = 0;
				Sdown = 0;
				
				//MPI_Status stat;
    //            		int messageFlag = false;
    //            		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &messageFlag, &stat);
    //           		 	if(messageFlag == true){
    //                    		cout << rank << ": I have failed to received a message!!!" << stat.MPI_TAG << endl;
    //                    		MPI_Abort(MCW,3);
    //            		}
				//MPI_Barrier(MCW);
				idGrid->share();
				wsGrid->share();
				lengths->share();
				contribs->addBorders();

				for(i=0; i<nx; i++){//todo fix this to add passed to queue
					if(contribs->getData(i, 0, tempShort) == 0 && contribs->getData(i,-1,tempShort)!=0) //may receive flow from more than 1
					{
						t.x = i;
						t.y = 0;
						que.push(t);
					}
					if(contribs->getData(i, ny-1, tempShort) == 0 && contribs->getData(i,ny,tempShort)!=0)//only looks at one direction of flow, out of a partition && idGrid->isNodata(i,ny-1) && src->getData(i,ny-1,tempShort)==1
					{
						t.x = i;
						t.y = ny-1;
						que.push(t); 
					}
				}
				finished = que.empty();
				//MPI_Barrier(MCW);
				int total = 0;
				int AllSum = 0;
				if(!finished)
					total = que.size();
				MPI_Allreduce(&total,&AllSum,1,MPI_INT,MPI_SUM,MCW);
				if(AllSum == 0)
					finished = true;
				else
					finished = false;
				if(verbose)
				{
					cout << rank << " Remaining in partition queue: " << total << endl;
					cout << rank << " Remaining total: " << AllSum << endl;
				}
			}
			else
				finished=true;  // For size == 1 no partition loop
		}	

//  Now all links have been created
		// Timer
		double linkct = MPI_Wtime();
		//if(rank == 0)
		//{
		//	fprintf(stderr,"\nLinks have been created, starting to write links\n");
		//	fflush(stderr);
		//}
//  XXXXXXXXX
//  Block to write link data

		if(verbose)
		{
			cout << rank << " Starting to write links"  << endl;
			MPI_Status stat;
            int messageFlag = false;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &messageFlag, &stat);
            if(messageFlag == true){
                    cout << rank << ": I have failed to received a message!!!" << endl;
                    MPI_Abort(MCW,2);
            }
			MPI_Barrier(MCW);
		}

		long myNumLinks;
		long myNumPoints;
		long totalNumLinks;
		long totalNumPoints;
		long relativePointStart;//if 9 then line 9 is first to be filled by me

		double **PointXY;
		float **PointElevArea;

		long **LinkIdU1U2DMagShapeidCoords;
		double **LinkElevUElevDLength;
		long *buf;
		long *ptr;
		int place;
		int bsize = sizeof(long)+ MPI_BSEND_OVERHEAD;  
		buf = new long[bsize];
		getNumLinksAndPoints(myNumLinks,myNumPoints);
		//MPI_Barrier(MCW);

		i = 0;
		if(myNumLinks > 0){
			LinkIdU1U2DMagShapeidCoords = new long*[myNumLinks];
			for(i=0;i<myNumLinks;i++)
				LinkIdU1U2DMagShapeidCoords[i] = new long[9];

			LinkElevUElevDLength = new double*[myNumLinks];
			for(i = 0;i<myNumLinks;i++)	
				LinkElevUElevDLength[i] = new double[3];

			if(myNumPoints>0)
			{
				PointXY = new double*[myNumPoints];
				for(i=0;i<myNumPoints;i++)
					PointXY[i] = new double[2];

				PointElevArea = new float*[myNumPoints];
				for(i=0;i<myNumPoints;i++)
					PointElevArea[i] = new float[3];

				setLinkInfo(LinkIdU1U2DMagShapeidCoords,LinkElevUElevDLength,PointXY,PointElevArea,elev,&elevIO);
			}
		}

		int ibuf;
		int procNumPoints;
		int totalPoints = 0;
		float pbuf[3];
		double pbuf1[2];
		MPI_Status mystatus;
		
		int numPointsPrinted =0;
		int procNumLinks = 0;
		long treeBuf[9];

		int ilink, ipoint;
		if(rank==0){//open output point file
			FILE *fTreeOut;
			fTreeOut = fopen(treefile,"w");// process 0 writes all of its stuff
			FILE *fout;
			fout = fopen(coordfile,"w");

			//  Open shapefile 
			//need spatial refeence information which is stored in the tiffIO object
			createStreamNetShapefile(streamnetsrc,streamnetlyr,hSRSraster); // need raster spatail information for creating spatial reference in the shapefile
			long ndots=100/size+4;  // number of dots to print per process
			long nextdot=0;
			long dotinc=myNumLinks/ndots;

			for(ilink=0;ilink<myNumLinks;ilink++){//only once per link
				fprintf(fTreeOut,"\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\n",LinkIdU1U2DMagShapeidCoords[ilink][0],LinkIdU1U2DMagShapeidCoords[ilink][1],LinkIdU1U2DMagShapeidCoords[ilink][2],LinkIdU1U2DMagShapeidCoords[ilink][3],LinkIdU1U2DMagShapeidCoords[ilink][4],LinkIdU1U2DMagShapeidCoords[ilink][5],LinkIdU1U2DMagShapeidCoords[ilink][6],LinkIdU1U2DMagShapeidCoords[ilink][7],LinkIdU1U2DMagShapeidCoords[ilink][8]);
				//fflush(fTreeOut);
				long i1=LinkIdU1U2DMagShapeidCoords[ilink][1];
				long i2=LinkIdU1U2DMagShapeidCoords[ilink][2];
				float *lengthd, *elev, *area;
				double *pointx, *pointy;
				lengthd = new float[i2-i1+1];
				elev = new float[i2-i1+1];
				area = new float[i2-i1+1];
				pointx = new double[i2-i1+1];
				pointy = new double[i2-i1+1];
				for(ipoint=i1;ipoint<=i2;ipoint++){
					fprintf(fout,"\t%f\t%f\t%f\t%f\t%f\n",PointXY[ipoint][0],PointXY[ipoint][1],PointElevArea[ipoint][0],PointElevArea[ipoint][1],PointElevArea[ipoint][2]);
					lengthd[ipoint-i1]=PointElevArea[ipoint][0];
					elev[ipoint-i1]=PointElevArea[ipoint][1];
					area[ipoint-i1]=PointElevArea[ipoint][2];
					pointx[ipoint-i1]=PointXY[ipoint][0];
					pointy[ipoint-i1]=PointXY[ipoint][1];
				}
				//  Write shape
				long cnet[9];
				for(int iref=0; iref<9; iref++)
					cnet[iref]=LinkIdU1U2DMagShapeidCoords[ilink][iref];
				reachshape(cnet,lengthd,elev,area,pointx,pointy,i2-i1+1,srcIO);
			
				delete lengthd; // DGT to free memory
				delete elev;
				delete area;
				delete pointx;
				delete pointy;
				//if(ilink > nextdot)  // Indicating progress
				//{
				//	fprintf(stderr,".");
				//	fflush(stderr);
				//	nextdot=nextdot+dotinc;
				//}


			} // process 0 recvs data from other processes, writes to file
			if(myNumLinks == 0)
				numPointsPrinted = 0;
			else
				numPointsPrinted = LinkIdU1U2DMagShapeidCoords[myNumLinks-1][2]+1;
			for(i=1;i<size;i++){// send message to next process to wake it up
				MPI_Send(&ibuf,1,MPI_INT,i,0,MCW);// get numpoints from that process
				MPI_Recv(&procNumLinks,1,MPI_INT,i,0,MCW,&mystatus);//get points one at a time and print them to file
				if(procNumLinks > 0)
				{
					dotinc=procNumLinks/ndots;  // For tracking progress
					nextdot=0;
					for(ilink=0;ilink<procNumLinks;++ilink){
						MPI_Recv(&treeBuf,9,MPI_LONG,i,1,MCW,&mystatus);
						fprintf(fTreeOut,"\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\n",treeBuf[0],treeBuf[1]+numPointsPrinted,treeBuf[2]+numPointsPrinted,treeBuf[3],treeBuf[4],treeBuf[5],treeBuf[6],treeBuf[7],treeBuf[8]);

						MPI_Recv(&procNumPoints,1,MPI_INT,i,0,MCW,&mystatus);//get points one at a time and print them to file
				// Variables for shape
						float *lengthd, *elev, *area;
						double *pointx, *pointy;
						lengthd = new float[procNumPoints];
						elev = new float[procNumPoints];
						area = new float[procNumPoints];
						pointx = new double[procNumPoints];
						pointy = new double[procNumPoints];
		
						for(ipoint=0;ipoint<procNumPoints;++ipoint){
							MPI_Recv(&pbuf1,2,MPI_DOUBLE,i,1,MCW,&mystatus);
							MPI_Recv(&pbuf,3,MPI_FLOAT,i,1,MCW,&mystatus);
							fprintf(fout,"\t%f\t%f\t%f\t%f\t%f\n",pbuf1[0],pbuf1[1],pbuf[0],pbuf[1],pbuf[2]); 

							lengthd[ipoint]=pbuf[0];
							elev[ipoint]=pbuf[1];
							area[ipoint]=pbuf[2];
							pointx[ipoint]=pbuf1[0];
							pointy[ipoint]=pbuf1[1];
						}
						//  Write shape
						reachshape(treeBuf,lengthd,elev,area,pointx,pointy,procNumPoints,srcIO);
						delete lengthd; // DGT to free memory
						delete elev;
						delete area;
						delete pointx;
						delete pointy;	
						//if(ilink > nextdot)  // Indicating progress
						//{
						//	fprintf(stderr,".");
						//	fflush(stderr);
						//	nextdot=nextdot+dotinc;
						//}

					}
				//  DGT moved line below to out of the loop so this only increments on the last link - and only if there is one
				numPointsPrinted += treeBuf[2]+1;//might need adjustmetn JJN
				}
			} 
			fclose(fTreeOut);
			fclose(fout);
			//SHPClose(shp1);
			//DBFClose(dbf1);
		    OGR_DS_Destroy( hDS1 ); // destroy data source
			/*
			shp1.close(streamnetshp);
			*/
		}else{//other processes send their stuff to process 0
			//first, wait to recieve word from process 0
			MPI_Recv(&ibuf,1,MPI_INT,0,0,MCW,&mystatus);//then, send myNumPoints
			MPI_Send(&myNumLinks,1,MPI_INT,0,0,MCW);//pack up each point into a buffer, and send it
			for(ilink=0;ilink<myNumLinks;++ilink){
				treeBuf[0] = LinkIdU1U2DMagShapeidCoords[ilink][0];
				treeBuf[1] = LinkIdU1U2DMagShapeidCoords[ilink][1];
				treeBuf[2] = LinkIdU1U2DMagShapeidCoords[ilink][2];
				treeBuf[3] = LinkIdU1U2DMagShapeidCoords[ilink][3];
				treeBuf[4] = LinkIdU1U2DMagShapeidCoords[ilink][4];
				treeBuf[5] = LinkIdU1U2DMagShapeidCoords[ilink][5];
				treeBuf[6] = LinkIdU1U2DMagShapeidCoords[ilink][6];
				treeBuf[7] = LinkIdU1U2DMagShapeidCoords[ilink][7];
				treeBuf[8] = LinkIdU1U2DMagShapeidCoords[ilink][8];
				MPI_Send(&treeBuf,9,MPI_LONG,0,1,MCW);
				int ptsInLink=treeBuf[2]-treeBuf[1]+1;
				MPI_Send(&ptsInLink,1,MPI_INT,0,0,MCW);//pack up each point into a buffer, and send it
				for(ipoint=treeBuf[1];ipoint<=treeBuf[2];++ipoint){
					pbuf1[0]=PointXY[ipoint][0];
					pbuf1[1]=PointXY[ipoint][1];
					pbuf[0]=PointElevArea[ipoint][0];
					pbuf[1]=PointElevArea[ipoint][1];
					pbuf[2]=PointElevArea[ipoint][2];
					MPI_Send(&pbuf1,2,MPI_DOUBLE,0,1,MCW);
					MPI_Send(&pbuf,3,MPI_FLOAT,0,1,MCW);
			//		MPI_Recv(&ibuf,1,MPI_INT,0,0,MCW,&mystatus);
				}
			}
		}  // just be sure we're all here
		MPI_Barrier(MCW);  //DGT  This seems necessary for cluster version to work, though not sure why.
		
		// Timer - link write time
		double linkwt = MPI_Wtime();
		if(rank==0)  // Indicating progress
		{
			fprintf(stderr,"\n Calculating watersheds\n");
			fflush(stderr);
		}

		if(verbose)
		{
			cout << rank << " Starting to calculate watershed"  << endl;
			int messageFlag;
			MPI_Status stat;
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &messageFlag, &stat);
			if(messageFlag == true){
					cout << rank << ": I have failed to received a message!!!" << endl;
					MPI_Abort(MCW,1);
			}
			MPI_Barrier(MCW);
		}
// XXXXXXXXXXXXXXXXXXXXXXXXX
//  Block to calculate watersheds
		//long tempLongNodata=MISSINGLONG;
		//short tempShortNodata=MISSINGSHORT;

		//tdpartition *temp111;
		//temp111 = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dx, dy, MISSINGSHORT);
		//for(i=0; i<nx; i++)
		//	for(j=0; j<ny; j++)
		//	{
		//		if(!wsGrid->isNodata(i,j))
		//			temp111->setData(i,j,(short)wsGrid->getData(i,j,tempLong));
		////		else
		////			temp111->setToNodata(i,j);
		//	}

//		tiffIO ws1IO("wfile.tif", SHORT_TYPE,&tempShortNodata,ad8IO);
//		ws1IO.write(xstart, ystart, ny, nx, temp111->getGridPointer());
//		tiffIO id1IO("idfile.tif", LONG_TYPE,&tempLongNodata,ad8IO);
//		id1IO.write(xstart, ystart, ny, nx, idGrid->getGridPointer());



		//Calculate watersheds....
		//  Reinitialize partition to delete leftovers from process above
		delete contribs;
		contribs = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dxA, dyA, MISSINGSHORT);
		//MPI_Barrier(MCW);
		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				// If my downslope neighbor has an id, contribs is 0 and I am ready to evaluate so go on q, else contribs is 1
				if(!flowDir->isNodata(i,j))
				{   //  Only put on queue to do if it is off the stream network
					if(src->isNodata(i,j) || src->getData(i,j,tempShort) < 1) //idGrid->isNodata(i,j))  //  Only if it is no data do we have to do anything 
					{
						p=flowDir->getData(i,j,tempShort);
						inext=i+d1[p];
						jnext=j+d2[p];
						if(src->hasAccess(inext,jnext))  //  Only do anything with cells for which we have access to downslope cell
						{
							if(src->isNodata(inext,jnext) || src->getData(inext,jnext,tempShort) < 1){ //idGrid->isNodata(inext,jnext))
								contribs->setData(i,j,(short)1);  //  downslope cell not on stream
								wsGrid->setToNodata(i,j);  // overwrite grid value that may have been inherited from outlet not on stream
							}
							else
							{
								contribs->setData(i,j,(short)0); //  downslope cell on stream
								t.x=i;
								t.y=j;
								que.push(t);
							}
						}
					}//  Here on stream
					else if(ordert==0)  // Ordert = 0 indicates that all watersheds are to be labeled with the number 1
					{
						tempLong=1;  
						wsGrid->setData(i,j,tempLong);
					}
				}
			}
		}

		if(verbose)
		{
			cout << rank << " Watershed initialization complete"  << endl;
		}
		// each process empties its que, then shares border info, and repeats till everyone is done
		finished=false;
		while(!finished){
			contribs->clearBorders();
			while(!que.empty()){
				t=que.front();
				que.pop();
				i = t.x;
				j = t.y;
				p=flowDir->getData(i,j,tempShort);
				inext=i+d1[p];
				jnext=j+d2[p];
				if(ordert > 0){  // Ordert > 0 indicates that all watersheds are to be labeled with different numbers
					wsGrid->setData(i,j,wsGrid->getData(inext,jnext,tempLong));
					//cout << tempLong;
				}else
				{
					tempLong=1;  // Ordert = 0 indicates that all watersheds are to be labeled with the number 1
					wsGrid->setData(i,j,tempLong);
				}
				//  Now find all cells that point to me and decrease contribs and put on queue if necessary
				for(m=1;m<=8;++m){
					inext=i+d1[m];
					jnext=j+d2[m];
					if(pointsToMe(i,j,inext,jnext,flowDir))
					{
						contribs->addToData(inext,jnext,(short)(-1));
						if(contribs->isInPartition(inext,jnext) && contribs->getData(inext,jnext,tempShort)==0)
						{
							t.x=inext;
							t.y=jnext;
							que.push(t);
						}
					}
				}
			}
			//Pass information across partitions
			contribs->addBorders();
			wsGrid->share();
			for(i=0; i<nx; i++){
				if(contribs->getData(i, -1, tempShort)!=0 && contribs->getData(i, 0, tempShort)==0)//only looks at one direction of flow, out of a partition
				{
					t.x = i;
					t.y = 0;
					que.push(t);
				}
				if(contribs->getData(i, ny, tempShort) != 0 && contribs->getData(i, ny-1, tempShort)==0)//only looks at one direction of flow, out of a partition
				{
					t.x = i;
					t.y = ny-1;
					que.push(t); 
				}
			}
			//Check if done
			finished = que.empty();
			finished = contribs->ringTerm(finished);
		}
		// Timer - watershed label time
		double wshedlabt = MPI_Wtime();
		if(verbose)
		{
			cout << rank << " Writing watershed file"  << endl;
		}
	

		int32_t wsGridNodata=MISSINGLONG;
		short ordNodata=MISSINGSHORT;
		tiffIO wsIO(wfile, LONG_TYPE,&wsGridNodata,ad8IO);
		wsIO.write(xstart, ystart, ny, nx, wsGrid->getGridPointer());
		if(verbose)
		{
			cout << rank << " Assigning order array"  << endl;
		}


		//  Use contribs as a short to write out order data that was held in lengths
		tempShort=0;
		for(i=0; i<nx; i++)
			for(j=0; j<ny; j++)
			{
				contribs->setData(i,j,tempShort);
				if(!lengths->isNodata(i,j))
					contribs->setData(i,j,(short)lengths->getData(i,j,tempFloat));
			}
		if(verbose)
		{
			cout << rank << " Writing order file"  << endl;
		}
		tiffIO ordIO(ordfile, SHORT_TYPE,&ordNodata,ad8IO);
		ordIO.write(xstart, ystart, ny, nx, contribs->getGridPointer());
		
		// Timer - write time
		double writet = MPI_Wtime();
        double dataRead, lengthc, linkc, linkw, wshedlab,  write, total,tempd;
        dataRead = readt-begint;
	lengthc = lengthct-readt;
        linkc = linkct-lengthct;
	linkw = linkwt - linkct;
	wshedlab = wshedlabt - linkwt;
        write = writet-wshedlabt;
        total = writet - begint;

        MPI_Allreduce (&dataRead, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = tempd/size;
        MPI_Allreduce (&lengthc, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        lengthc = tempd/size;
        MPI_Allreduce (&linkc, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        linkc = tempd/size;
        MPI_Allreduce (&linkw, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        linkw = tempd/size;
        MPI_Allreduce (&wshedlab, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        wshedlab = tempd/size;
        MPI_Allreduce (&write, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        write = tempd/size;
        MPI_Allreduce (&total, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = tempd/size;

		if( rank == 0)
			printf("Processors: %d\nRead time: %f\nLength compute time: %f\nLink compute time: %f\nLink write time: %f\nWatershed compute time: %f\nWrite time: %f\nTotal time: %f\n", size, dataRead, lengthc, linkc, linkw, wshedlab, write,total);


	}MPI_Finalize();
	return 0;
}

