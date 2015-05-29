/*  DinfAvalanche function to compute avalanche runout zones

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


// The old program was written in column major order.
// d1 and d2 were used to translate flow data (1-8 taken from the indeces)
// to direction in the 2d array.  We switched d1 and d2 to create it in row major order
// This may be wrong and need to be changed
//const short d1[9] = { 0,0,-1,-1,-1, 0, 1,1,1};
//const short d2[9] = { 0,1, 1, 0,-1,-1,-1,0,1};
// moved to commonlib.h

//float dist[9];
float **dist;
int avalancherunoutgrd(char *angfile, char *felfile, char *assfile, char *rzfile, char *dmfile, float thresh, 
					   float alpha, int path)
{

	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("DinfAvalanche version %s\n",TDVERSION);

	float area,angle,asss,dmm,rzz,zmm;
	int32_t imm,jmm;
	double p,tempdxc,tempdyc;

	//  Keep track of time
	double begint = MPI_Wtime();

	//Create tiff object, read and store header info
	tiffIO ang(angfile, FLOAT_TYPE);
	long totalX = ang.getTotalX();
	long totalY = ang.getTotalY();
	double dxA = ang.getdxA();
	double dyA = ang.getdyA();
    double xllcentr = ang.getXllcenter();
	double yllcentr = ang.getYllcenter();
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

	//using elevation, get information from file
	tdpartition *felData;
	tiffIO fel(felfile, FLOAT_TYPE);
	if(!ang.compareTiff(fel)) {
		printf("File sizes do not match\n%s\n",felfile);
		MPI_Abort(MCW,5);
		return 1; 
	}
	felData = CreateNewPartition(fel.getDatatype(), totalX, totalY, dxA, dyA, fel.getNodata());
	fel.read(xstart, ystart, felData->getny(), felData->getnx(), felData->getGridPointer());

	tdpartition *assData;
	tiffIO ass(assfile, SHORT_TYPE);
	if(!ang.compareTiff(ass)) {
		printf("File sizes do not match\n%s\n",assfile);
		MPI_Abort(MCW,5);
		return 1; 
	}
	assData = CreateNewPartition(ass.getDatatype(), totalX, totalY, dxA, dyA, ass.getNodata());
	ass.read(xstart, ystart, assData->getny(), assData->getnx(), assData->getGridPointer());

	//  Calculate distances in each direction
	int kk;
	 dist = new float*[ny];
    for(int m= 0; m < ny;m++)
    dist[m] = new float[9];
	for ( int m=0;m<ny;m++){
		 flowData->getdxdyc(m,tempdxc,tempdyc);
	     for(kk=1; kk<=8; kk++){
		dist[m][kk]=sqrt(tempdxc*tempdxc*d1[kk]*d1[kk]+tempdyc*tempdyc*d2[kk]*d2[kk]);
	                           }
	}
	//Begin timer
	double readt = MPI_Wtime();

	//Convert geo coords to grid coords
	
	//Create empty partition to store new information
	tdpartition *rz;
	rz = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);
	tdpartition *dm;
	dm = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	tdpartition *im;
	im = CreateNewPartition(LONG_TYPE, totalX, totalY, dxA, dyA, MISSINGLONG);
	tdpartition *jm;
	jm = CreateNewPartition(LONG_TYPE, totalX, totalY, dxA, dyA, MISSINGLONG);

	tdpartition *zm;
	zm = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	double bndbox[4];
	float *xcoord;
	float *ycoord;	
	xcoord = new float[totalX];
	ycoord = new float[totalY];
	
	bndbox[0] = xllcentr - (ang.getdlon()/2);
	bndbox[1] = yllcentr - (ang.getdlat()/2);
    bndbox[2] = bndbox[0] + (ang.getdlon()*totalX);
	bndbox[3] = bndbox[1] + (ang.getdlat()*totalY);
	


	/*dem->head.bndbox[0]=xllcenter-(*dx/2);
	dem->head.bndbox[1]=yllcenter-(*dy/2);
	dem->head.bndbox[2]=dem->head.bndbox[0] + *dx * (*nx);
	dem->head.bndbox[3]=dem->head.bndbox[1] + *dy * (*ny);*/
	int ii,jj;
	for(int ii=0; ii<totalY; ii++){
		ycoord[ii]=bndbox[3] - (ii*ang.getdlat());}
	for(int jj=0; jj<totalX; jj++)
	    {xcoord[jj]=bndbox[0] + (jj*ang.getdlon());}

	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	bool con=false, finished;
	float tempFloat=0;
	short tempShort=0;

	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);
	
	//Share information and set borders to zero
	flowData->share();
	felData->share();
	assData->share();
	neighbor->clearBorders();

	node temp;
	queue<node> que;

		//Count the flow receiving neighbors and put on queue
	int useOutlets=0;
	long numOutlets=0;
	int *outletsX=0, *outletsY=0;
	initNeighborDinfup(neighbor,flowData,&que,nx, ny, useOutlets, outletsX, outletsY, numOutlets);

	
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
			//  FLOW ALGEBRA EVALUATION
			if(!felData->isNodata(i,j)){
				if(!assData->isNodata(i,j) && assData->getData(i,j,tempShort)>0)
				{
					rz->setData(i,j,alpha);
					int ig,jg;
					flowData->localToGlobal((int)i,(int)j,ig,jg);
					im->setData(i,j,(int32_t)ig);  //  record global row and column so that lookup across partitions works
					jm->setData(i,j,(int32_t)jg);
					zm->setData(i,j,felData->getData(i,j,tempFloat));  //  Record max elevation in partition as global lookup later does not work across partitions
					dm->setData(i,j,(float)0.0);
				}
				float rzzij;
				rz->getData(i,j,rzzij);
				for(k=1; k<=8; k++) {
					in = i+d1[k];
					jn = j+d2[k];
					if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
						flowData->getData(in,jn, angle);
						flowData->getdxdyc(jn,tempdxc,tempdyc);
						p = prop(angle, (k+4)%8,tempdxc,tempdyc);
						if(p>0.0 && p >= thresh) {
							rz->getData(in,jn,rzz);
							im->getData(in,jn,imm);
							jm->getData(in,jn,jmm);
							zm->getData(in,jn,zmm);
							if(rzz >= alpha){
								float d;
								if(path==1){
									dm->getData(in,jn,dmm);
									d=dmm + dist[j][k];
								}
								else
								{		
									int ig,jg;
									flowData->localToGlobal((int)i,(int)j,ig,jg);
									float dxx=xcoord[ig]-xcoord[imm];//was jmm; imm and jmm are swiched because in the serial code they are used as j,i
									float dyy=ycoord[jg]-ycoord[jmm];//was imm

									if(ang.getproj()==1){
										double dxdyc[2];
		                                ang.geotoLength(dxx,dyy,ycoord[jg],dxdyc);
										d=sqrt(dxdyc[0]*dxdyc[0]+dxdyc[1]*dxdyc[1]);


									}
									else if(ang.getproj()==0) {
									d=sqrt(dxx*dxx+dyy*dyy);

									}


								}
								float felin,felij;
								//felData->getData(imm,jmm,felin);
								felin=zmm;
								felData->getData(i,j,felij);
								float zd=felin - felij;
								float beta=atan(zd/d)*180/PI;

//								float rzzij;
//								rz->getData(i,j,rzzij);  // Moved outside loop

								if(beta >= alpha && beta > rzzij)
								{
									rzzij=beta;
									rz->setData(i,j,rzzij);
									im->setData(i,j,imm);
									jm->setData(i,j,jmm);
									zm->setData(i,j,zmm);
									dm->setData(i,j,d);
								}
							}
						}
					}
				}
			}
			//  END FLOW ALGEBRA EXPRESSION EVALUATION
			//  Decrement neighbor dependence of downslope cell
			flowData->getData(i, j, angle);
			flowData->getdxdyc(j,tempdxc,tempdyc);
			for(k=1; k<=8; k++) {			
				p = prop(angle, k,tempdxc,tempdyc);
				if(p>0.0) {
					in = i+d1[k];  jn = j+d2[k];
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
		}
		//Pass information

		rz->share();
		im->share();
		jm->share();
		dm->share();
		zm->share();
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
		finished = rz->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float scaNodata = MISSINGFLOAT;
	tiffIO rrz(rzfile, FLOAT_TYPE, &scaNodata, ang);
	rrz.write(xstart, ystart, ny, nx, rz->getGridPointer());

	tiffIO ddm(dmfile, FLOAT_TYPE, &scaNodata, ang);
	ddm.write(xstart, ystart, ny, nx, dm->getGridPointer());

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


	return 0;
}
