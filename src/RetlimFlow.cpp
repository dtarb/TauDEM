/*  Retention Limited Flow Accumulation.

  David Tarboton, 
  Utah State University  
  February, 2013
  
*/

/*  Copyright (C) 2014  David Tarboton, Utah State University

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
//#include <math.h>
//#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
//#include "shape/shapefile.h"
#include "initneighbor.h"
#include <stdio.h>
#include "retlimro.h"
using namespace std;

int retlimro(char *angfile, char *wgfile, char * rcfile, char *qrlfile)
{
	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	float p;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if (rank == 0) {
		printf("Retention limited flow accumulation version %s\n", TDVERSION);
		fflush(stdout);
	}
	

	//Create tiff object, read and store header info for angle file
	tiffIO angIO(angfile, FLOAT_TYPE);
	long totalX = angIO.getTotalX();
	long totalY = angIO.getTotalY();
	double dxA = angIO.getdxA();
	double dyA = angIO.getdyA();

	//Create partition and read data
	tdpartition *ang;
	ang = CreateNewPartition(angIO.getDatatype(), totalX, totalY, dxA, dyA, angIO.getNodata());
	int nx = ang->getnx();
	int ny = ang->getny();
	int xstart, ystart;
	ang->localToGlobal(0, 0, xstart, ystart);
	ang->savedxdyc(angIO);  // copy dxc, dyc from io object into partion object
	angIO.read(xstart, ystart, ny, nx, ang->getGridPointer());
	//printf("Rank %d, nx %d, ny %d, xstart %d, ystart %d\n",rank,nx,ny,xstart,ystart);

	// Weight grid, get information from file
	tdpartition *wg;
	tiffIO wgIO(wgfile, FLOAT_TYPE);
	if(!angIO.compareTiff(wgIO)){
		printf("File sizes do not match\n%s\n",wgfile);
		fflush(stdout);
		MPI_Abort(MCW,5);
		return 1;
	}
	wg = CreateNewPartition(wgIO.getDatatype(), totalX, totalY, dxA, dyA, wgIO.getNodata());
	wgIO.read(xstart, ystart, wg->getny(), wg->getnx(), wg->getGridPointer());
	
    //retention capacity (rc) grid, get information from file	
	tdpartition *rcg;
	tiffIO rcgIO(rcfile, FLOAT_TYPE);
	if(!angIO.compareTiff(rcgIO)){
		printf("File sizes do not match\n%s\n",rcfile);
		fflush(stdout);
		MPI_Abort(MCW,5);
		return 1;
	}
	rcg = CreateNewPartition(rcgIO.getDatatype(), totalX, totalY, dxA, dyA, rcgIO.getNodata());
	rcgIO.read(xstart, ystart, rcg->getny(), rcg->getnx(), rcg->getGridPointer());

	tdpartition *qrl;
	
	qrl=CreateNewPartition(FLOAT_TYPE,totalX,totalY,dxA,dyA,MISSINGFLOAT);
	float floatval;
	long jx,iy;

// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	float tempFloat=0;
	short tempShort=0;
	double tempdxc, tempdyc;

	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);
	
	//Share information and set borders to zero
	ang->share();
//	if(usew==1) weightData->share(); //share is not needed, the program does not change this
	rcg->share();  //  Because may access neighbors rc data
	wg->share();

	neighbor->clearBorders();

	node temp;
	queue<node> que;

	int *outletsX=0, *outletsY=0;
	int useOutlets=0;
	int numOutlets=0;
	initNeighborDinfup(neighbor,ang,&que,nx, ny, useOutlets, outletsX, outletsY, numOutlets);
	short neighborNodata = MISSINGSHORT;

	bool con=false, finished=false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()) 
		{
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;

			float qrlval=0.;
			if(!wg->isNodata(i,j) && !rcg->isNodata(i,j))
			{
				for(k=1; k<=8;k++){
					//  Find row and col of neighbors
					in = i+d1[k];
					jn = j+d2[k];
					if(ang->hasAccess(in,jn))
					{
						float angle;
						ang->getData(in,jn, angle);
						ang->getdxdyc(jn, tempdxc, tempdyc);
						p = prop(angle, (k+4)%8, tempdxc, tempdyc);  // Gives the proportion of flow in direction (k+4)%8
						if(p>0.){
							qrlval=qrlval+p*qrl->getData(in,jn,tempFloat);
						}
					}
				}
				qrlval=qrlval+wg->getData(i,j,tempFloat)-rcg->getData(i,j,tempFloat);
				if(qrlval <0.)qrlval=0.;
				qrl->setData(i,j,qrlval);
//  End of expression evaluation
//  Now start machinery of decreasing downslope que
				//  Get the neighbor grid cell and decrease the neighbor partition
				float angle;
				ang->getData(i, j, angle);
				ang->getdxdyc(j, tempdxc, tempdyc);

				// Find the downslope directions
			    for(k=1; k<=8; k++) {			
				    p = prop(angle, k, tempdxc, tempdyc);
				    if(p>0.0) {
						in = i+d1[k];  jn = j+d2[k];
						//Decrement the number of contributing neighbors in neighbor
						neighbor->addToData(in,jn,(short)-1);				
						//Check if neighbor needs to be added to que
						if(ang->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
							temp.x=in;
							temp.y=jn;
							que.push(temp);
						}
					}
				}
			}
		}
		//  Here que is empty
		qrl->share();
		neighbor->addBorders();  // Decreases borders based on answers from the other partition

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
		finished = qrl->ringTerm(finished);
	}


	float qrlNodata = MISSINGFLOAT;
	tiffIO qrlIO(qrlfile,FLOAT_TYPE,qrlNodata,rcgIO);
	qrlIO.write(xstart,ystart,ny,nx,qrl->getGridPointer());

	}MPI_Finalize();
	return 0;
}

