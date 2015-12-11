/*  DinfRevAccum function to compute D-infinity reverse accumulation.
  
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


int dsaccum(char *angfile,char *wgfile, char *raccfile, char *dmaxfile)
{

	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("DinfRevAccum version %s\n",TDVERSION);

	float dmaxx,angle,raccc,apooll;
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

	//if using weightData, get information from file
	tdpartition *wgData;	
	tiffIO wg(wgfile, FLOAT_TYPE);
	if(!ang.compareTiff(wg)) {
		printf("File sizes do not match\n%s\n",wgfile);
		MPI_Abort(MCW,5);
		return 1;  
	}
	wgData = CreateNewPartition(wg.getDatatype(), totalX, totalY, dxA, dyA, wg.getNodata());
	wg.read(xstart, ystart, wgData->getny(), wgData->getnx(), wgData->getGridPointer());
	
	//Begin timer
	double readt = MPI_Wtime();	

	//Create empty partition to store new information
	tdpartition *racc;
	racc = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);
	tdpartition *dmax;
	dmax = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	bool con=false, finished;
	float tempFloat=0;
	short tempShort=0;

	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);
	
	//Share information and set borders to zero
	flowData->share();
	wgData->share();
//	racc->clearBorders();
//	dmax->clearBorders();
	neighbor->clearBorders();

	node temp;
	queue<node> que;
	
	//Count the contributing neighbors and put on queue.  The logic for what goes on que must strictly
	// not use any data other than flow field (and outlet points)
	for(j=0; j<ny; j++) {
		for(i=0; i<nx; i++) {
			if(!flowData->isNodata(i,j)) {
				//Set receiving neighbors to 0 
				neighbor->setData(i,j,(short)0);
				//Count number of neighbors that receive flow from the target cell 
				for(k=1; k<=8; k++){
					in = i+d1[k];
					jn = j+d2[k];
					flowData->getData(i, j, angle);
					flowData->getdxdyc(j,tempdxc,tempdyc);
					p = prop(angle, k,tempdxc,tempdyc);
					if(p>0. && flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn))
						neighbor->addToData(i,j,(short)1);
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

	//Debug code to write neighbor file
	/* short smv = MISSINGSHORT;
	char nfile[50];
	sprintf(nfile,"neighbor.tif");
	tiffIO ntio(nfile, SHORT_TYPE, &smv, ang);
	ntio.write(xstart, ystart, ny, nx, neighbor->getGridPointer()); */

	
	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()) 
		{
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;
			// EVALUATE DOWN FLOW ALGEBRA EXPRESSION
			if(wgData->isNodata(i,j))
			{
				racc->setToNodata(i,j);
				dmax->setToNodata(i,j);
			}
			else
			{
				racc->setData(i,j,wgData->getData(i,j,tempFloat));
				dmax->setData(i,j,tempFloat);
				for(k=1; k<=8; k++){
					in = i+d1[k];
					jn = j+d2[k];
					flowData->getData(i, j, angle);
					flowData->getdxdyc(j,tempdxc,tempdyc);
					p = prop(angle, k,tempdxc,tempdyc);
					if(p>0. && flowData->hasAccess(in,jn) && !racc->isNodata(in, jn))
					{
						float valn;
						valn=p*racc->getData(in,jn,tempFloat);
						racc->addToData(i,j,valn);
						dmax->getData(in,jn,valn);
						if(valn > dmax->getData(i,j,tempFloat))dmax->setData(i,j,valn);
					}
				}
			}
			//  END DOWN FLOW ALGEBRA EXPRESSION EVALUATION	
			//  Decrement the number of contributing neighbors in neighbor for cells that drain to i,j
			for(k=1; k<=8; k++) {
				in = i+d1[k];
				jn = j+d2[k];
				if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
					flowData->getData(in,jn, angle);
					flowData->getdxdyc(jn,tempdxc,tempdyc);
					p = prop(angle, (k+4)%8,tempdxc,tempdyc);
					if(p>0.)
					{
						neighbor->addToData(in,jn,(short)-1);
						//Check if neighbor needs to be added to que
						if(flowData->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
							temp.x=in;
							temp.y=jn;
							que.push(temp);
						}
					}
				}
			}
		}
		//Pass information
		racc->share();
		dmax->share();
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
		finished = racc->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float raccNodata = MISSINGFLOAT;
	tiffIO rrac(raccfile, FLOAT_TYPE, &raccNodata, ang);
	rrac.write(xstart, ystart, ny, nx, racc->getGridPointer());

	tiffIO ddmax(dmaxfile, FLOAT_TYPE, &raccNodata, ang);
	ddmax.write(xstart, ystart, ny, nx, dmax->getGridPointer());

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
