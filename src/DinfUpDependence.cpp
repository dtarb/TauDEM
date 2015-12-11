/*  DinfUpDependence function to compute D-infinity upslope dependence.
  
  David Tarboton,Teklu K Tesfa
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


int depgrd(char* angfile, char* dgfile, char* depfile)
{

	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("DinfUpDependence version %s\n",TDVERSION);

	float angle,depp;
	int32_t dgg;
	double p,tempdxc,tempdyc;

	//  Keep track of time
	double begint = MPI_Wtime();

	//Create tiff object, read and store header info
	tiffIO ang(angfile, FLOAT_TYPE);
	long totalX = ang.getTotalX();
	long totalY = ang.getTotalY();
	double dxA = ang.getdxA();
	double dyA = ang.getdyA();

	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Create partition and read data
	tdpartition *flowData;
	flowData = CreateNewPartition(ang.getDatatype(), totalX, totalY, dxA, dyA, ang.getNodata());
	int nx = flowData->getnx();
	int ny = flowData->getny();
	int xstart, ystart;
	flowData->localToGlobal(0, 0, xstart, ystart);
	flowData->savedxdyc(ang);
	ang.read(xstart, ystart, ny, nx, flowData->getGridPointer());

	// Read DG 
	tdpartition *dgData;	
	tiffIO dg(dgfile, LONG_TYPE);
	if(!ang.compareTiff(dg)) return 1;  //And maybe an unhappy error message
	dgData = CreateNewPartition(dg.getDatatype(), totalX, totalY, dxA, dyA, dg.getNodata());
	dg.read(xstart, ystart, dgData->getny(), dgData->getnx(), dgData->getGridPointer());
	
	//Begin timer
	double readt = MPI_Wtime();	

	//Create  partition to store dependence result
	tdpartition *dep;
	float depNodata = -1.0f;
	dep = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, depNodata);

	//  Create neighbor partition
	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);
	
	//Share information and set borders to zero
	flowData->share();
	dgData->share();
	dep->share();  //  To fill borders with nodata
	neighbor->clearBorders();  //  To fill borders with 0
	long x,y;
	short k;
	long xn,yn;
	bool finished;
	float tempFloat=0;
	short tempShort=0;

	node temp;
	queue<node> que;

	//Count the contributing neighbors and put on queue
	for(y=0; y<ny; y++) {
		for(x=0; x<nx; x++) {
			// Neighbor was initialized to no data by createNewPartition.  
			// In this loop initialize to 0 then count number of downslope 
			//  neighbors on which it depends		
			if(!flowData->isNodata(x,y)) {
				//Set receiving neighbors to 0 
				neighbor->setData(x,y,(short)0);
				//Count number of neighbors that receive flow from the target cell 
				for(k=1; k<=8; k++){
					xn = x+d1[k];  yn = y+d2[k];
					flowData->getData(x,y, angle);
					flowData->getdxdyc(y,tempdxc,tempdyc);
					p = prop(angle, k,tempdxc,tempdyc);
					if(p>0.0 && flowData->hasAccess(xn,yn) && !flowData->isNodata(xn,yn))
						neighbor->addToData(x,y,(short)1);
				}
				if(neighbor->getData(x, y, tempShort) == 0 ){
					temp.x=x;
					temp.y=y;
					que.push(temp);
				}
			}
		}
	} 
	
	finished = false;
	
//Ring terminating while loop
	while(!finished) {

//  SHARE Quantities evaluated in FLOW ALGEBRA
		dep->share();

//  Reconcile neighbor dependencies across partitions
		neighbor->addBorders();
		finished=true;  // so that if nothing happens we quit
		//If this created a cell with no contributing neighbors, put it on the queue
		for(x=0; x<nx; x++){
			//  Only add neighbors to que that were changed by border exchange
			if(neighbor->getData(x, -1, tempShort)!=0 && neighbor->getData(x, 0, tempShort)==0){
				temp.x = x;
				temp.y = 0;
				que.push(temp);
			}
			if(neighbor->getData(x, ny, tempShort)!=0 && neighbor->getData(x, ny-1, tempShort)==0){
				temp.x = x;
				temp.y = ny-1;
				que.push(temp); 
			}
		}
		neighbor->clearBorders();  //  zero for next time round

//  Processing within a partition
		while(!que.empty()) 
		{
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			x = temp.x;
			y = temp.y;
			finished=false;  // because here we are evaluating 
			//  BEGIN GENERIC DOWN FLOW ALGEBRA EVALUATION BLOCK
			//fdepg.d[j][i] = fdepg.d[j][i] + p * fdepg.d[jn][in] ;
			angle=flowData->getData(x, y, angle);
			flowData->getdxdyc(y,tempdxc,tempdyc);
			if(dgData->getData(x,y,dgg)>=1)
			{
				dep->setData(x,y,(float)1.0);
			}
			else
			{
				dep->setData(x,y,(float)0.0);
				for(k=1; k<=8; k++) {
					p=prop(angle,k,tempdxc,tempdyc);  // 
					if(p>0.0)
					{
						xn = x+d1[k];  yn = y+d2[k];
						if(flowData->hasAccess(xn,yn) && !flowData->isNodata(xn,yn)){
							dep->getData(xn,yn,depp);
							dep->addToData(x,y,(float)(depp*p));
						}
					}
				}
			}
			//  END OF EXPRESSION EVALUATION BLOCK
			// Now decrease neighbor dependencies of inflowing neighbors
			for(k=1; k<=8; k++) {
				xn = x+d1[k];  yn = y+d2[k];
				if(flowData->hasAccess(xn,yn) && !flowData->isNodata(xn,yn)){
					flowData->getData(xn, yn, angle);
					flowData->getdxdyc(yn,tempdxc,tempdyc);
					p = prop(angle, (k+4)%8,tempdxc,tempdyc);
					if(p>0.0) {
						//Decrement the number of contributing neighbors in neighbor
						neighbor->addToData(xn,yn,(short)-1);	
						//Check if neighbor needs to be added to que
						if(flowData->isInPartition(xn,yn) && neighbor->getData(xn, yn, tempShort) == 0 ){
							temp.x=xn;
							temp.y=yn;
							que.push(temp);
						}
					}
				}
			}
		}
		finished = dep->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	tiffIO ddep(depfile, FLOAT_TYPE, &depNodata, ang);
	ddep.write(xstart, ystart, ny, nx, dep->getGridPointer());

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
                  size , dataRead, compute, write,total);


	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();


	return 0;
}
