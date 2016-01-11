/*  D8FlowPathExtremeUp 

  Function that evaluates the extreme (either maximum or minimum) upslope value 
  from an input grid based on the D8 flow directions.  It is similar to the minimum upslope function 
  used Tarolli and Tarboton (2006).  This is the parallel version using D8 flow directions (Tarolli and Tarboton used Dinfinity).  This is 
  intended initially for use in stream raster generation to identify a threshold of slope x area 
  product that results in an optimum (according to drop analysis) stream network.  If an outlets 
  shapefile is provided the function should only output results for the area upslope of the outlets.

  David G Tarboton, Teklu K Tesfa
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
#include "initneighbor.h"
using namespace std;

int d8flowpathextremeup(char *pfile, char*safile, char *ssafile, int usemax, char* datasrc,char* lyrname,int uselyrname,int lyrno,int useOutlets, int contcheck)
{
MPI_Init(NULL,NULL);
{  //  All code within braces so that objects go out of context and destruct before MPI is closed

	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("D8FlowPathExtremeUp version %s\n",TDVERSION);

	double *x, *y;
	int numOutlets=0;
	tiffIO p(pfile,SHORT_TYPE);
	long totalX = p.getTotalX();
	long totalY = p.getTotalY();
    double dxA = p.getdxA();
	double dyA = p.getdyA();
	OGRSpatialReferenceH hSRSRaster;
    hSRSRaster=p.getspatialref();
//  Begin timer
    double begint = MPI_Wtime();
	if( useOutlets == 1) {
		if(rank==0){
			if(readoutlets(datasrc,lyrname,uselyrname,lyrno, hSRSRaster,&numOutlets, x, y) !=0){
				printf("Exiting \n");
				MPI_Abort(MCW,5);
			}else {
				MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
				MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
			}
		}
		else {
			MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);

			x = (double*) malloc( sizeof( double ) * numOutlets );
			y = (double*) malloc( sizeof( double ) * numOutlets );
			
			MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
			MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
		}
	}
 // printf("%d %d\n",rank,numOutlets);
  //for(int i=0; i<numOutlets; i++)printf("%g, %g\n",x[i],y[i]);
	//Read Flow Direction header using tiffIO

	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Read flow direction data into partition
	tdpartition *flowData;
	flowData = CreateNewPartition(p.getDatatype(), totalX, totalY, dxA, dyA, p.getNodata());
	int nx = flowData->getnx();
	int ny = flowData->getny();
	int xstart, ystart;
	flowData->localToGlobal(0, 0, xstart, ystart);
	p.read(xstart, ystart, ny, nx, flowData->getGridPointer());

  	//Read input grid for which extreme upslope values are required (ss grid)
	tdpartition *saData;
	tiffIO sa(safile,FLOAT_TYPE);
	if(!p.compareTiff(sa)) {
		printf("File sizes do not match\n%s\n",safile);
		MPI_Abort(MCW,5);
		return 1;  
	}
	saData = CreateNewPartition(sa.getDatatype(), totalX, totalY, dxA, dyA, sa.getNodata());
	sa.read(xstart, ystart, ny, nx, saData->getGridPointer());

	//Record time reading files
	double readt = MPI_Wtime();

	//Convert geo coords to grid coords
	int *outletsX=0, *outletsY=0;
	if(useOutlets==1) {
		outletsX = new int[numOutlets];
		outletsY = new int[numOutlets];
		for( int i=0; i<numOutlets; i++)
			p.geoToGlobalXY(x[i], y[i], outletsX[i], outletsY[i]);
	}

	//Create empty partition to store new information
	tdpartition *ssa;
	ssa = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	int con=0;
	bool finished;
	float tempFloat=0;
	short tempShort=0;

	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);
	
	//Share information and set borders to zero
	flowData->share();
	saData->share();
	ssa->clearBorders();
	neighbor->clearBorders();

	node temp;
	queue<node> que;
	initNeighborD8up(neighbor,flowData,&que,nx,ny,useOutlets,outletsX,outletsY,numOutlets);
	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()){
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;
			//  FLOW ALGEBRA EXPRESSION EVALUATION		
			if(flowData->isInPartition(i,j)){
				//  Initial value is the value of saData at the location
				ssa->setData(i,j,saData->getData(i,j,tempFloat));
				con=0;   //  So far not edge contaminated
				// Now examine neighbors
				for(k=1; k<=8; k++) {
					in = i+d1[k];
					jn = j+d2[k];
			//test if neighbor drains towards cell excluding boundaries 
					if(!flowData->isNodata(in,jn))
					{
						flowData->getData(in,jn,tempShort);
						if(tempShort-k == 4 || tempShort-k == -4)
						{
                            if(ssa->isNodata(in,jn))con = -1;
							else 
							{
								float nFloat;  // Variable to hold neighbor value
								if(usemax == 1)
								{
									if(ssa->getData(in,jn,nFloat) > ssa->getData(i,j,tempFloat))ssa->setData(i,j,nFloat);
								}
								else
								{
									if(ssa->getData(in,jn,nFloat) < ssa->getData(i,j,tempFloat))ssa->setData(i,j,nFloat);
								}
							}
						}
					}
					else con = -1;
				}
				if(con == -1 && contcheck == 1)ssa->setToNodata(i,j);
			}
			else ssa->setToNodata(i,j);
			//  END FLOW ALGEBRA EXPRESSION EVALUATION
			// decrease neighbor dependence of downslope cell
			flowData->getData(i,j,k);
			in = i+d1[k];
			jn = j+d2[k];
			//Decrement the number of contributing neighbors in neighbor
			neighbor->addToData(in,jn,(short)-1);
			//Check if neighbor needs to be added to que
			if(flowData->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
				temp.x=in;
				temp.y=jn;
				que.push(temp);
			}
		}
		//  Here the queue is empty
		//Pass information
		ssa->share();
		neighbor->addBorders();

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
		finished = ssa->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float aNodata = MISSINGFLOAT;
	tiffIO a(ssafile, FLOAT_TYPE, &aNodata, p);
	a.write(xstart, ystart, ny, nx, ssa->getGridPointer());
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
return(0);
}
