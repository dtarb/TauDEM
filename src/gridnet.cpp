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
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
using namespace std;




int gridnet( char *pfile, char *plenfile, char *tlenfile, char *gordfile, char *maskfile,
		char* datasrc,char* lyrname,int uselyrname,int lyrno, int useMask, int useOutlets, int thresh) 
{//1

	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);//returns the rank of the calling processes in a communicator
	MPI_Comm_size(MCW,&size);//returns the number of processes in a communicator
	if(rank==0)printf("GridNet version %s\n",TDVERSION);
		
	double *x, *y;
	int numOutlets=0;
	bool usingShapeFile=false;

	double begint = MPI_Wtime();
	tiffIO p(pfile,SHORT_TYPE);
	long totalX = p.getTotalX();
	long totalY = p.getTotalY();
	double dxA = p.getdxA();
	double dyA = p.getdyA();
	OGRSpatialReferenceH hSRSRaster;
    hSRSRaster=p.getspatialref();

	if( useOutlets == 1) {//3
		if(rank==0){//4
			if(readoutlets(datasrc,lyrname,uselyrname,lyrno, hSRSRaster,&numOutlets, x, y)==0){
//				for(int i=0; i< numOutlets; i++)
//					printf("rank: %d, X: %lf, Y: %lf\n",rank,x[i],y[i]);
				usingShapeFile=true;
			//	printf("Rank: %d, numOutlets: %d\n",rank,numOutlets);
				MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
				MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
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

			MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
			MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
			usingShapeFile=true;
			//printf("Rank: %d, numOutlets: %d\n",rank,numOutlets);
		}
	}//3
	//printf("Rank: %d, Numoutlets: %d\n",rank,numOutlets); fflush(stdout);
//	for(int i=0; i< numOutlets; i++)
//		printf("rank: %d, X: %lf, Y: %lf\n",rank,x[i],y[i]);

	//Create tiff object, read and store header info
	//tiffIO p(pfile,SHORT_TYPE);
	//long totalX = p.getTotalX();
	//long totalY = p.getTotalY();
	//double dxA = p.getdxA();
	//double dyA = p.getdyA();
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
	flowData->savedxdyc(p);
	p.read(xstart, ystart, ny, nx, flowData->getGridPointer());
	//printf("Pfile read");  fflush(stdout);

	//if using Mask, create partion and read it
	tdpartition *maskData;
	if( useMask == 1){
		tiffIO mask(maskfile,LONG_TYPE);
		if(!p.compareTiff(mask)) {
			printf("File sizes do not match\n%s\n",maskfile);
			MPI_Abort(MCW,5);
			return 1;  
		}
		maskData = CreateNewPartition(mask.getDatatype(), totalX, totalY, dxA, dyA, mask.getNodata());
		mask.read(xstart, ystart, maskData->getny(), maskData->getnx(), maskData->getGridPointer());
	}
	else
	{
		maskData = CreateNewPartition(LONG_TYPE, totalX, totalY, dxA, dyA, 1);
		thresh=0;  //  Here we have a partition filled with ones and a 0 threshold so mask condition is always satisfied
	}
	//Begin timer
	double readt = MPI_Wtime();
	//printf("Read time %lf\n",readt);
	//fflush(stdout);

	//Convert geo coords to grid coords
	int *outletsX, *outletsY;
	if(usingShapeFile) {
		outletsX = new int[numOutlets];
		outletsY = new int[numOutlets];
		for( int i=0; i<numOutlets; i++)
			p.geoToGlobalXY(x[i], y[i], outletsX[i], outletsY[i]);
	}

	//Create empty partition to store new information
	tdpartition *plen;
	tdpartition *tlen;
	tdpartition *gord;
	plen = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, -1.0f);
	tlen = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, -1.0f);
	gord = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -1);

	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	float area;
	bool con=false, finished;
	float tempFloat=0.0;
	short tempShort=0;
	int32_t tempLong=0; 

		/*  Calculate Distances  */
	float **dist;
	//for(i=1; i<=8; i++){
		//dist[i]=sqrt(d1[i]*d1[i]*dy*dy+d2[i]*d2[i]*dx*dx);
	//}
	double tempdxc,tempdyc;
	dist = new float*[ny];
    for(int m = 0; m <ny; m++)
    dist[m] = new float[9];
	for (int m=0; m<ny;m++){
		flowData->getdxdyc(m,tempdxc,tempdyc);
		for(int kk=1; kk<=8; kk++)
      {
	    dist[m][kk]=sqrt(tempdxc*tempdxc*d1[kk]*d1[kk]+tempdyc*tempdyc*d2[kk]*d2[kk]);
	}

	}

	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);
	
	//Share information and set borders to zero
	flowData->share();
	maskData->share();
	plen->clearBorders();
	tlen->clearBorders();
	gord->clearBorders();
	neighbor->clearBorders();

	node temp;
	queue<node> que;
	

	if(!usingShapeFile) {
		//Treat gord like area in aread8.  Initialize to 1
		for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++ ) {
				if(!flowData->isNodata(i,j) && maskData->getData(i,j,tempLong)>= thresh)
					gord->setData(i,j,(short)1);
			}
		}
	
		//Count the contributing neighbors and put on queue
		for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++) {

		

				//Initialize neighbor count to no data, but then 0 if flow direction is defined
				neighbor->setToNodata(i,j);
				if(!flowData->isNodata(i,j)) {
					//Set contributing neighbors to 0 
					neighbor->setData(i,j,(short)0);
					//Count number of contributing neighbors
					for(k=1; k<=8; k++){
						in = i+d1[k];
						jn = j+d2[k];
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
							flowData->getData(in,jn,tempShort);
							if(tempShort-k == 4 || tempShort-k == -4)
								neighbor->addToData(i,j,(short)1);
						}
					}
					if(neighbor->getData(i, j, tempShort) == 0){
						//Push nodes with no contributing neighbors on queue
						temp.x = i;
						temp.y = j;
						que.push(temp);
					}
				}
			}
		} 
	}
	// If Outlets are specified
	else {
		//Set area to 0 for all points not upstream of outlets
		for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++ ) {
				if(!flowData->isNodata(i,j))
					gord->setData(i,j,(short)0);
			}
		}
		//Put outlets on queue to be evalutated
		queue<node> toBeEvaled;
		//printf("Num outlets: %d\n",numOutlets);
		for( i=0; i<numOutlets; i++) {
			flowData->globalToLocal(outletsX[i], outletsY[i], temp.x, temp.y);
			if(flowData->isInPartition(temp.x, temp.y))
				toBeEvaled.push(temp);
		}

		//TODO - this is 100% linear partition dependent.
		//Create a packet for message passing
		int *bufferAbove = new int[nx];
		int *bufferBelow = new int[nx];
		int countA, countB;

		//TODO - consider copying this statement into other memory allocations
		if( bufferAbove == NULL || bufferBelow == NULL ) {
			printf("Error allocating memory\n");
			MPI_Abort(MCW,5);
		}
		
		MPI_Status status;
		int rank;
		MPI_Comm_rank(MCW,&rank);

		finished = false;
		while(!finished) {
			countA = 0;
			countB = 0;
			while(!toBeEvaled.empty()) {
				temp = toBeEvaled.front();
				toBeEvaled.pop();
				i = temp.x;
				j = temp.y;
				// Only evaluate if cell hasn't been evaled yet
				if(neighbor->isNodata(i,j)){
					//Set area of cell
					gord->setData(i,j,(short)1);
					//Set contributing neighbors to 0
					neighbor->setData(i,j,(short)0);
					//Count number of contributing neighbors
					for( k=1; k<=8; k++){
						in = i+d1[k];
						jn = j+d2[k];
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)) {
							flowData->getData(in,jn,tempShort);
							if(tempShort-k == 4 || tempShort-k == -4){
								if( jn == -1 ) {
									bufferAbove[countA] = in;
									countA +=1;
								}
								else if( jn == ny ) {
									bufferBelow[countB] = in;
									countB += 1;
								}
								else {
									temp.x = in;
									temp.y = jn;
									toBeEvaled.push(temp);
								}
								neighbor->addToData(i,j,(short)1);
							}
						}					
					}

					if(neighbor->getData(i,j, tempShort) == 0){
						//Push nodes with no contributing neighbors on queue
						temp.x = i;
						temp.y = j;
						que.push(temp);
					}
					
				}	
			}
			finished = true;
			
			neighbor->transferPack( &countA, bufferAbove, &countB, bufferBelow );

			if( countA > 0 || countB > 0 )
				finished = false;

			if( rank < size-1 ) {
				for( k=0; k<countA; k++ ) {
					temp.x = bufferAbove[k];
					temp.y = ny-1;
					toBeEvaled.push(temp);
				}
			}
			if( rank > 0 ) {
				for( k=0; k<countB; k++ ) {
					temp.x = bufferBelow[k];
					temp.y = 0;
					toBeEvaled.push(temp);
				}
			}
			finished = neighbor->ringTerm( finished );
		}

	}
	
	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()){
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;			
	
			if(flowData->isInPartition(i,j)){   // DGT thinks this is redundant - nothing should be on queue that is not in partition - but does no harm
				//  Here is where the flow algebra is evaluated
				short a1,a2;
				float ld;
				if(maskData->getData(i,j,tempLong)>=thresh)
				{
					tempFloat=0.0f;  //  Initialize to 0
					tlen->setData(i,j,tempFloat);  
					plen->setData(i,j,tempFloat);
					a1=0;
					a2=0;	
					for(k=1; k<=8; k++)
					{  
						in=i+d1[k];
						jn=j+d2[k];
					   /* test if neighbor drains towards cell excluding boundaries */
						short sdir = flowData->getData(in,jn,tempShort);
						if(sdir > 0) 
						{
							if( maskData->getData(in,jn,tempLong)>=thresh && (sdir-k==4 || sdir-k==-4))
							{
								//  Implement Strahler ordering 
								if(gord->getData(in,jn,tempShort) >= a1)
								{
									a2=a1;
									a1=gord->getData(in,jn,tempShort);
								}
								else if ( gord->getData(in,jn,tempShort) > a2 )
									a2=gord->getData(in,jn,tempShort);
								//  Length calculations
								ld= plen->getData(in,jn,tempFloat) + dist[j][sdir];
								tlen->addToData(i,j,(float)(tlen->getData(in,jn,tempFloat)+dist[j][sdir]));
								if( ld > plen->getData(i,j,tempFloat))
									plen->setData(i,j,ld);
							}
						}
					}
					if(a2+1 > a1) gord->setData(i,j,(short)(a2+1));
					else gord->setData(i,j,(short)a1);
				}
			}

			//  End of evaluation of flow algebra
			// Drain cell into surrounding neighbors
			flowData->getData(i,j,k);
			in = i+d1[k];
			jn = j+d2[k];
			//TODO - streamlining here
			if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
				//Decrement the number of contributing neighbors in neighbor
				neighbor->addToData(in,jn,(short)-1);
		
				//Check if neighbor needs to be added to que
				if(flowData->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
					temp.x=in;
					temp.y=jn;
					que.push(temp);
				}
			}
		}
		//Pass information
		neighbor->addBorders();
		gord->share();
		plen->share();
		tlen->share();

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
	
		//Check if done
		finished = que.empty();
		finished = gord->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();
	
	//Create and write TIFF file
	short sNodata = -1;
	float fNodata = -1.0f;
	tiffIO gordIO(gordfile, SHORT_TYPE, &sNodata, p);
	gordIO.write(xstart, ystart, ny, nx, gord->getGridPointer());
	tiffIO plenIO(plenfile, FLOAT_TYPE, &fNodata, p);
	plenIO.write(xstart, ystart, ny, nx, plen->getGridPointer());
	tiffIO tlenIO(tlenfile, FLOAT_TYPE, &fNodata, p);
	tlenIO.write(xstart, ystart, ny, nx, tlen->getGridPointer());

	double writet = MPI_Wtime();
	double dataRead, compute, write, total,tempd;
        dataRead = readt-begint;
        compute = computet-readt;
        write = writet-computet;
        total = writet - begint;

        MPI_Allreduce (&dataRead, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = tempd/size;
        MPI_Allreduce (&compute, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = tempd/size;
        MPI_Allreduce (&write, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        write = tempd/size;
        MPI_Allreduce (&total, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = tempd/size;

        if( rank == 0)
                printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                  size,dataRead, compute, write,total);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}




