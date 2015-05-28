/*  SlopeAveDown function to compute slope averaged over down stream distance.
     
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
#include "initneighbor.h"

using namespace std;



//float dist[9];
float **dist;

// Slope D
///////////////////////////////////////////////////////////////////////
int sloped(char *pfile,char* felfile,char* slpdfile, double dn)
{
	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("SlopeAveDown version %s\n",TDVERSION);

	double begint = MPI_Wtime();

	// Read elevation data
	tiffIO dem(felfile,FLOAT_TYPE);
	long totalX = dem.getTotalX();
	long totalY = dem.getTotalY();
	double dxA = dem.getdxA();
	double dyA = dem.getdyA();


	if(rank==0)
		{
			float timeestimate=(1e-6*totalX*totalY*(dn/dxA)/pow((double) size,1))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


	//Create partition and read data
	tdpartition* z;
	z = CreateNewPartition(dem.getDatatype(), totalX, totalY, dxA, dyA, dem.getNodata());
	int nx = z->getnx();
	int ny = z->getny();
	int xstart, ystart;
	z->localToGlobal(0, 0, xstart, ystart);
	z->savedxdyc(dem);
	dem.read(xstart, ystart, ny, nx, z->getGridPointer());	

	//Read flow directions 
	tiffIO pIO(pfile,SHORT_TYPE);
	if(!dem.compareTiff(pIO)) {
		printf("File sizes do not match\n%s\n",pfile);
		MPI_Abort(MCW,5);
		return 1; 
	}
	//Create partition and read data
	tdpartition *p;
	p = CreateNewPartition(pIO.getDatatype(), totalX, totalY, dxA, dyA, pIO.getNodata());
	pIO.read(xstart, ystart, ny, nx, p->getGridPointer());

// begin timer
	double readt = MPI_Wtime();

	//  Calculate distances in each direction
	//int kk;
	//for(kk=1; kk<=8; kk++)
	//{
		//dist[kk]=sqrt(dx*dx*d2[kk]*d2[kk]+dy*dy*d1[kk]*d1[kk]);
	//}
	double tempdxc,tempdyc;
	dist = new float*[ny]; 
    for(int m = 0; m < ny; m++)
    dist[m] = new float[9];
    for (int m=0;m<ny;m++){
		z->getdxdyc(m,tempdxc,tempdyc);
	for(int kk=1; kk<=8; kk++){
		dist[m][kk]=sqrt(d1[kk]*d1[kk]*tempdxc*tempdxc+d2[kk]*d2[kk]*tempdyc*tempdyc);
	}
	}


	
	//Create partitions to work with
	tdpartition *ed;  //  Elevation from downslope
	ed = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	tdpartition *dd;  //  Distances from downslope
	dd = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	tdpartition *sd;  //  Slope down
	sd = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

//  Working variables
	float tempFloat,ddi,zi,slp;
	short tempShort;
	long i,j,in,jn;
	short k;

	//  Initialize
	//  ed to original elevations
	//  dd to 0
	//  add points to process to que1

	for(i=0;i<nx;i++)
	{
		for(j=0;j<ny;j++)
		{
			if(!z->isNodata(i,j) && !p->isNodata(i,j))
			{
				ed->setData(i,j,z->getData(i,j,tempFloat));
				dd->setData(i,j,(float)0.0);
			}
		}
	}
	//  Share borders
	z->share();
	ed->share();
	dd->share();
	p->share();
	//  Loop
	bool finished=false;

	int niter=dn/min(dxA,dyA)+1;
	int iter;
	//  Now need to repeat as many times as necessary
	//  until down elevations have been pulled up far enough to evaluate distance down.  Calculate
	//  the number of iterations apriori as keeping track of not changing is not reliable
	//  as there are unresolvable grid cells at the edge
	for(iter=0; iter<niter; iter++)
	{	
		//  Set up neighbor partition inside this loop to reinforce its destruction and refreshing for each iteration
		tdpartition *neighbor;
		neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);
		neighbor->clearBorders();

		//  Set up dependence que
		node temp;
		queue<node> que;
		int useOutlets=0;
		long numOutlets=0;
		int *outletsX=0, *outletsY=0;
		
		initNeighborD8up(neighbor,p,&que,nx, ny, useOutlets, outletsX, outletsY, numOutlets);

		finished=false;
		//  Internal ring terminationg loop like D8 flow algebra
		while(!finished) {
			while(!que.empty()){
				//Takes next node with no contributing neighbors
				temp = que.front();
				que.pop();
				i = temp.x;
				j = temp.y;	
				//  HERE EVALUATE SLOPEDOWN AND SHIFT ed up
				p->getData(i,j,k);
				if(k >= 1 && k <= 8)  //  Can only work with valid k
				{
					in=i+d1[k];
					jn=j+d2[k];
					if(ed->hasAccess(in,jn) && !ed->isNodata(in,jn))
					{
						ddi=dist[j][k]+dd->getData(in,jn,tempFloat);
						zi=ed->getData(in,jn,tempFloat);
						if(sd->isNodata(i,j))
						{
							if(ddi > dn)  // ready to calculate slope
							{
								slp=(z->getData(i,j,tempFloat)-zi)/ddi;
								sd->setData(i,j,slp);
							}
						}
						//  save ed and dd for next pass
						ed->setData(i,j,zi);  
						dd->setData(i,j,ddi);
					}
					//  DONE WITH EVALUATION
					if(p->hasAccess(in,jn) && !p->isNodata(in,jn)){
						//Decrement the number of contributing neighbors in neighbor
						neighbor->addToData(in,jn,(short)-1);	
						//Check if neighbor needs to be added to que
						if(p->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
							temp.x=in;
							temp.y=jn;
							que.push(temp);
						}
					}
				}
				else
					printf("Warning:  Attempted to use invalid flow direction to access downslope grid cell\n");
			}
			//  Now swap border information among partitions
			neighbor->addBorders();
			ed->share();
			dd->share();
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
			finished = sd->ringTerm(finished);
		}
		//  This ends the internal loop for one iteration.  
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float slpnd = MISSINGFLOAT;
	tiffIO slpIO(slpdfile, FLOAT_TYPE, &slpnd, pIO);
	slpIO.write(xstart, ystart, ny, nx, sd->getGridPointer());

	double writet = MPI_Wtime();
        double dataRead, compute, write, total,temp;
        dataRead = readt-begint;
        compute = computet-readt;
        write = writet-computet;
        total = writet - begint;

        MPI_Allreduce (&dataRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = temp/size;
        MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = temp/size;
        MPI_Allreduce (&write, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        write = temp/size;
        MPI_Allreduce (&total, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = temp/size;

        if( rank == 0)
                printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                  size , dataRead, compute, write,total);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}
