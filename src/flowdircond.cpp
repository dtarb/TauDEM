/*  Flow direction conditioning function to condition DEM based on input flow directions
      
  David Tarboton, Nazmus Sazib
  Utah State University  
  May 10, 2016 
  
*/

/*  Copyright (C) 2016  David Tarboton, Utah State University

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




int flowdircond( char *pfile, char *zfile, char *zfdcfile) 
{

	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);//returns the rank of the calling processes in a communicator
	MPI_Comm_size(MCW,&size);//returns the number of processes in a communicator
	if(rank==0)printf("FlowDirCond version %s\n",TDVERSION);
		
	double begint = MPI_Wtime();
	tiffIO p(pfile,SHORT_TYPE);
	long totalX = p.getTotalX();
	long totalY = p.getTotalY();
	double dxA = p.getdxA();
	double dyA = p.getdyA();
	OGRSpatialReferenceH hSRSRaster;
    hSRSRaster=p.getspatialref();

	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

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

	//Read the DEM
	tdpartition *zData;
	tiffIO zIO(zfile,FLOAT_TYPE);
	if(!p.compareTiff(zIO)) {
		printf("File sizes do not match\n%s\n",zfile);
		fflush(stdout);
		MPI_Abort(MCW,5);
		return 1;  
	}
	zData = CreateNewPartition(zIO.getDatatype(), totalX, totalY, dxA, dyA, zIO.getNodata());
	zIO.read(xstart, ystart, zData->getny(), zData->getnx(), zData->getGridPointer());
	//Begin timer
	double readt = MPI_Wtime();

	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	float area;
	bool con=false, finished;
	float tempFloat=0.0;
	short tempShort=0;
	int32_t tempLong=0; 
	int numOutlets=0;
	int *outletsX; int *outletsY;
	int useOutlets=0;
	outletsX = new int[numOutlets];
		outletsY = new int[numOutlets];
	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);
	
	//Share information and set borders to zero
	flowData->share();
	zData->share();
	neighbor->clearBorders();

	node temp;
	queue<node> que;

	initNeighborD8up(neighbor,flowData,&que,nx, ny, useOutlets, outletsX, outletsY, numOutlets);
//TODO  Assumption - need to check that this handles no data flow directions OK for how we are using it	
	
	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()){
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;	
			//  Start of flow algebra evaluation
			if (!zData->isNodata(i,j))
			{
				float zval=zData->getData(i,j,tempFloat);
				for(k=1; k<=8; k++)
				{  
					in=i+d1[k];
					jn=j+d2[k];
					/* test if neighbor drains towards cell excluding boundaries */
					short sdir = flowData->getData(in,jn,tempShort);
					if(sdir > 0)
					{
						if(!zData->isNodata(in,jn))
						{
							if(zData->getData(in,jn,tempFloat)<zval && (sdir-k==4 || sdir-k==-4))
							{
								zval=tempFloat;
								zData->setData(i,j,zval);
							}
						}
					}
				}
			}
			//  End of evaluation of flow algebra
			// Drain cell into surrounding neighbors
			flowData->getData(i,j,k);
			in = i+d1[k];
			jn = j+d2[k];
			if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn))
			{
				//Decrement the number of contributing neighbors in neighbor
				neighbor->addToData(in,jn,(short)-1);		
				//Check if neighbor needs to be added to que
				if(flowData->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 )
				{
					temp.x=in;
					temp.y=jn;
					que.push(temp);
				}
			}
		}
		//Pass information
		neighbor->addBorders();
		zData->share();

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
		finished = zData->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();
	
	//Create and write TIFF file
	float fNodata =-1.0f;
	tiffIO zfdcIO(zfdcfile, FLOAT_TYPE, &fNodata, p);
	zfdcIO.write(xstart, ystart, ny, nx, zData->getGridPointer());

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




