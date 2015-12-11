/*  D8HDistToStrm

  This function computes the distance from each grid cell moving downstream until a stream 
  grid cell as defined by the Stream Raster grid is encountered.  The optional threshold 
  input is to specify a threshold to be applied to the Stream Raster grid (src).  
  Stream grid cells are defined as having src value >= the threshold, or >=1 if a 
  threshold is not specified.

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

int distgrid(char *pfile, char *srcfile, char *distfile, int thresh)
{
MPI_Init(NULL,NULL);
{  //  All code within braces so that objects go out of context and destruct before MPI is closed
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("D8HDistToStrm version %s\n",TDVERSION);
	int i,j,in,jn;
	float tempFloat; double tempdxc,tempdyc;
	short tempShort,k;
	int32_t tempLong;
	bool finished;

 //  Begin timer
    double begint = MPI_Wtime();

	//Read Flow Direction header using tiffIO
	tiffIO pf(pfile,SHORT_TYPE);
	long totalX = pf.getTotalX();
	long totalY = pf.getTotalY();
	double dxA = pf.getdxA();
	double dyA = pf.getdyA();


	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Read flow direction data into partition
	tdpartition *p;
	p = CreateNewPartition(pf.getDatatype(), totalX, totalY, dxA, dyA, pf.getNodata());
	int nx = p->getnx();
	int ny = p->getny();
	int xstart, ystart;
	p->localToGlobal(0, 0, xstart, ystart);
	p->savedxdyc(pf);
	pf.read(xstart, ystart, ny, nx, p->getGridPointer());

 	//Read src file
	tdpartition *src;
	tiffIO srcf(srcfile,LONG_TYPE);
	if(!pf.compareTiff(srcf)) {
		printf("File sizes do not match\n%s\n",srcfile);
		MPI_Abort(MCW,5);
		return 1;  //And maybe an unhappy error message
	}
	src = CreateNewPartition(srcf.getDatatype(), totalX, totalY, dxA, dyA, srcf.getNodata());
	srcf.read(xstart, ystart, ny, nx, src->getGridPointer());

	//Record time reading files
	double readt = MPI_Wtime();
   
 	//Create empty partition to store distance information
	tdpartition *fdarr;
	fdarr = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	/*  Calculate Distances  */
	//float dist[9];
	float** dist = new float*[ny];
    for(int j = 0; j< ny; j++)
    dist[j] = new float[9];

    for (int m=0;m<ny;m++){
	p->getdxdyc(m,tempdxc,tempdyc);
	for(int kk=1; kk<=8; kk++){
		 dist[m][kk]=sqrt(d1[kk]*d1[kk]*tempdxc*tempdxc+d2[kk]*d2[kk]*tempdyc*tempdyc);
		 }
	}

	//  Set neighbor partition to 1 because all grid cells drain to one other grid cell in D8
	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);
    
	node temp;
	queue<node> que;
	for(j=0; j<ny; j++) 
		for(i=0; i<nx; i++) {
			if(!p->isNodata(i,j)) {
				//Set contributing neighbors to 1 
				neighbor->setData(i,j,(short)1);
			}
			if(!src->isNodata(i,j) && src->getData(i,j,tempLong) >=thresh){
				neighbor->setData(i,j,(short)0);
				temp.x = i;
				temp.y = j;
				que.push(temp);
			}
	}

	//Share information and set borders to zero
	p->share();
	src->share();
	fdarr->share();
	neighbor->clearBorders();

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
			//  If on stream
			if(!src->isNodata(i,j) && src->getData(i,j,tempLong) >=thresh){
				fdarr->setData(i,j,(float)0.0);
			}
			else
			{
				p->getData(i,j,k);  //  Get neighbor downstream
				in = i+d1[k];
				jn = j+d2[k];
				if(fdarr->isNodata(in,jn))fdarr->setToNodata(i,j);
				else
					fdarr->setData(i,j,(float)(dist[j][k]+fdarr->getData(in,jn,tempFloat)));
			}
			//  Now find upslope cells and reduce dependencies
			for(k=1; k<=8; k++) {
				in = i+d1[k];
				jn = j+d2[k];
			//test if neighbor drains towards cell excluding boundaries 
				if(!p->isNodata(in,jn))
				{
					p->getData(in,jn,tempShort);
					if(tempShort-k == 4 || tempShort-k == -4){
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
			}
		}
		//  Here the queue is empty
		//Pass information
		fdarr->share();
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
		finished = fdarr->ringTerm(finished);
	}
	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float aNodata = MISSINGFLOAT;
	tiffIO a(distfile, FLOAT_TYPE, &aNodata, pf);
	a.write(xstart, ystart, ny, nx, fdarr->getGridPointer());
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
                  size, dataRead, compute, write,total);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();
return(0);
}

