/*  Gridnet function to compute grid network order and upstream 
    lengths based on D8 model  
  
  David Tarboton
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

#include <mpi.h>
#include <math.h>
#include <iostream>
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include "ogrsf_frmts.h"
using namespace std;




int gagewatershed( char *pfile,char *wfile, char* datasrc,char* lyrname,int uselyrname,int lyrno, char *idfile,int writeid, int writeupid,char *upidfile) 
{//1

	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);//returns the rank of the calling processes in a communicator
	MPI_Comm_size(MCW,&size);//returns the number of processes in a communicator
	if(rank==0)printf("Gage Watershed version %s\n",TDVERSION);
		
	double *x=NULL, *y=NULL;
	int numOutlets=0;
	int *ids=NULL;
	int *dsids=NULL;
	int idnodata;
	FILE *fidout1;
	if (writeupid == 1) {		
		fidout1 = fopen(upidfile, "a");
	}
	
	double begint = MPI_Wtime();
	tiffIO p(pfile,SHORT_TYPE);
	long totalX = p.getTotalX();
	long totalY = p.getTotalY();
    double dxA = p.getdxA();
	double dyA = p.getdyA();
	OGRSpatialReferenceH hSRSRaster;
    hSRSRaster=p.getspatialref();
	if(rank==0){//4
		if(readoutlets(datasrc,lyrname,uselyrname,lyrno,hSRSRaster, &numOutlets, x, y,ids)==0){
			MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
			MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
			MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
			MPI_Bcast(ids, numOutlets, MPI_INT, 0, MCW);
			//  Find the minimum id to use it - 1 for no data 
			idnodata=ids[0];
			for(int i=1; i<numOutlets; i++)
			{
				if(ids[i]<idnodata)idnodata=ids[i];
			}
			idnodata=idnodata-1;
			MPI_Bcast(&idnodata,1,MPI_INT,0,MCW);
		
			//printf("after bcast\n"); fflush(stdout);
		}//5
		else {
			printf("Error opening shapefile. Exiting \n");
			MPI_Abort(MCW,5);
		}
	}//4
	else {
		//int countPts;
		//MPI_Bcast(&countPts, 1, MPI_INT, 0, MCW);
		MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);

		//x = (double*) malloc( sizeof( double ) * numOutlets );
		//y = (double*) malloc( sizeof( double ) * numOutlets );
		x = new double[numOutlets];
		y = new double[numOutlets];
		ids = new int[numOutlets];

		MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
		MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
		MPI_Bcast(ids, numOutlets, MPI_INT, 0, MCW);
		MPI_Bcast(&idnodata,1,MPI_INT,0,MCW);
	}
	dsids=new int[numOutlets];

	//printf("Rank: %d, Numoutlets: %d\n",rank,numOutlets); fflush(stdout);
//	for(int i=0; i< numOutlets; i++)
//		printf("rank: %d, X: %lf, Y: %lf\n",rank,x[i],y[i]);

	//Create tiff object, read and store header info

	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//printf("After header read %d\n",rank);   fflush(stdout);

	//Create partition and read data
	tdpartition *flowData;
	flowData = CreateNewPartition(p.getDatatype(), totalX, totalY, dxA, dyA, p.getNodata());
	int nx = flowData->getnx();
	int ny = flowData->getny();
	int xstart, ystart;
	flowData->localToGlobal(0, 0, xstart, ystart);
	p.read(xstart, ystart, ny, nx, flowData->getGridPointer());
	//printf("Pfile read");  fflush(stdout);

	//Begin timer
	double readt = MPI_Wtime();
	//printf("Read time %lf\n",readt);
	//fflush(stdout);
	 // writing upstream coordinates
	
	//Create empty partition to store new information
	tdpartition *wshed;
	wshed = CreateNewPartition(LONG_TYPE, totalX, totalY, dxA, dyA, MISSINGLONG);

	//Convert geo coords to grid coords
	int *outletsX, *outletsY; short outletpointswgrid; long tempSubwgrid; 
	outletsX = new int[numOutlets];
	outletsY = new int[numOutlets];

	node temp;
	queue<node> que;

	for( int i=0; i<numOutlets; i++)
	{
		p.geoToGlobalXY(x[i], y[i], outletsX[i], outletsY[i]);
		int xlocal, ylocal;
		wshed->globalToLocal(outletsX[i], outletsY[i],xlocal,ylocal);
		if(wshed->isInPartition(xlocal,ylocal)){   //xOutlets[i], yOutlets[i])){
			wshed->setData(xlocal,ylocal,(int32_t)ids[i]);  //xOutlets[i], yOutlets[i], (long)ids[i]);
							//Push outlet cells on to que
						temp.x = xlocal;
						temp.y = ylocal;
						que.push(temp);	
		}
		dsids[i]= idnodata;  
	}


	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	bool con=false, finished;
	float tempFloat=0.0;
	short tempShort=0;
	int32_t tempLong=0;
	int myid=0;
	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);

	//  Initialize neighbors to 1 if flow direction exists
		for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++) {
				if(!flowData->isNodata(i,j)) {
					//Set contributing neighbors to 0 
					neighbor->setData(i,j,(short)1);
				}
			}
		} 

	
	//Share information and set borders to zero
	flowData->share();
	neighbor->clearBorders();
	wshed->share();
	//flowData->getData(500,600,tempShort);
	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()){
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;	
			//  if not labeled, label with downstream result
			if(wshed->isNodata(i,j))
			{
				flowData->getData(i,j,k);
				in=i+d1[k];
				jn=j+d2[k];
				wshed->setData(i,j,wshed->getData(in,jn,tempLong));
			}
			//  if upstream is not labeled, put upstream on queue
			for(k=1; k<=8; k++)
			{  
				in=i+d1[k];
				jn=j+d2[k];
				/* test if neighbor drains towards cell excluding boundaries */
				short sdir = flowData->getData(in,jn,tempShort);
		       
				if(flowData->isNodata(in,jn) && writeupid == 1)	{ 
					 double mx,my;
					int gx,gy;  //  Global x and y (col and row) coordinates
						flowData->localToGlobal(in,jn,gx,gy);
						p.globalXYToGeo(gx,gy,mx,my);
						fprintf(fidout1, "%f, %f\n", mx, my);
						fflush(fidout1);						
						
						//myfile<<mx<<","<<my<<endl;
				}
				if(sdir > 0) 
				{
					if((sdir-k==4 || sdir-k==-4))
					{
						//  add to Q
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn) && wshed->isNodata(in,jn)){
							//Decrement the number of contributing neighbors in neighbor
							neighbor->addToData(in,jn,(short)-1);

							//Check if neighbor needs to be added to que
							if(flowData->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
								temp.x=in;
								temp.y=jn;
								que.push(temp);
							}
						}
						if(!wshed->isNodata(in,jn))
						{
							int32_t idup=wshed->getData(in,jn,tempLong);
							int32_t idme=wshed->getData(i,j,tempLong);
							//  Find the array index for idup
							int iidex;
							for(iidex=0; iidex<numOutlets; iidex++)
							{
								if(ids[iidex]==idup)break;
							}
							dsids[iidex]=idme;
						}
					}
				}
			}
		}
		//Pass information
		neighbor->addBorders();
		wshed->share();
		
		//If this created a cell with no contributing neighbors, put it on the queue
		for(i=0; i<nx; i++){
			if(neighbor->getData(i, -1, tempShort)!=0 && neighbor->getData(i, 0, tempShort)==0){
				temp.x = i;
				temp.y = 0;
				que.push(temp);
			}
			if(neighbor->getData(i, ny, tempShort)!=0 && neighbor->getData(i, ny-1, tempShort)==0){
				temp.x = i;
				temp.y = ny-1;
				que.push(temp); 
			}
		}
		//Clear out borders
		neighbor->clearBorders();
	   //
		
		//Check if done
		finished = que.empty();
		finished = wshed->ringTerm(finished);
		
	}

	

	//  Reduce all values to the 0 process
	int *dsidsr;	
    dsidsr=new int[numOutlets];
	MPI_Reduce(dsids,dsidsr,numOutlets,MPI_INT, MPI_MAX,0, MCW);

	//Stop timer
	double computet = MPI_Wtime();

//  Write id.txt with from and to links
	if(writeid == 1)
	{
		if(rank == 0)
		{
				FILE *fidout;
				fidout = fopen(idfile,"w");// process 0 writes 
				fprintf(fidout,"id iddown\n");
				for(i=0; i<numOutlets; i++)
				{

					fprintf(fidout,"%d %d\n",ids[i],dsidsr[i]);
				}
		}
	}
	
	
	//Create and write TIFF file
	long lNodata = MISSINGLONG;
	tiffIO wshedIO(wfile, LONG_TYPE, &lNodata, p);
	wshedIO.write(xstart, ystart, ny, nx, wshed->getGridPointer());

	double writet = MPI_Wtime();
	if( rank == 0) 
		printf("Size: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
		  size,readt-begint, computet-readt, writet-computet,writet-begint);

	

	
	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}



