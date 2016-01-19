/*  DinfTransLimAccum function to compute D-infinity transport limited accumulation.
  
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
#include "initneighbor.h"
using namespace std;


// The old program was written in column major order.
// d1 and d2 were used to translate flow data (1-8 taken from the indeces)
// to direction in the 2d array.  We switched d1 and d2 to create it in row major order
// This may be wrong and need to be changed
//const short d1[9] = { 0,0,-1,-1,-1, 0, 1,1,1};
//const short d2[9] = { 0,1, 1, 0,-1,-1,-1,0,1};
// moved to commonlib.h

//Transport limited accumulation funciton
int tlaccum(char *angfile, char *tsupfile, char *tcfile, char *tlafile, char *depfile, 
			char *cinfile, char *coutfile, char* datasrc,char* lyrname,int uselyrname,int lyrno, int useOutlets, int usec, 
			int contcheck)
{

	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("DinfTransLimAccum version %s\n",TDVERSION);

	//  Read shapfile 
	double *x, *y;
	int numOutlets=0;
	tiffIO ang(angfile, FLOAT_TYPE);
	long totalX = ang.getTotalX();
	long totalY = ang.getTotalY();
	double dxA = ang.getdxA();
	double dyA = ang.getdyA();
	OGRSpatialReferenceH hSRSRaster;
	hSRSRaster=ang.getspatialref();
	if( useOutlets == 1) {
		if(rank==0){
			if(readoutlets(datasrc,lyrname,uselyrname,lyrno,hSRSRaster, &numOutlets, x, y) !=0){
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

	float loadin,loadout,transin,transout,tsupp,tcc,angle;
	double p,tempdxc,tempdyc;

	//  Keep track of time
	double begint = MPI_Wtime();

	//Create tiff object, read and store header info
	/*tiffIO ang(angfile, FLOAT_TYPE);
	long totalX = ang.getTotalX();
	long totalY = ang.getTotalY();
	double dxA = ang.getdxA();
	double dyA = ang.getdyA();*/

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
	
	//Transport supply grid, get information from file
	tdpartition *tsupData;
	tiffIO tsup(tsupfile, FLOAT_TYPE);
	if(!ang.compareTiff(tsup)) {
		printf("File sizes do not match\n%s\n",tsupfile);
		MPI_Abort(MCW,5);
		return 1;  
	}
	tsupData = CreateNewPartition(tsup.getDatatype(), totalX, totalY, dxA, dyA, tsup.getNodata());
	tsup.read(xstart, ystart, tsupData->getny(), tsupData->getnx(), tsupData->getGridPointer());
	
	//Transport capacity grid, get information from file
	tdpartition *tcData;
	tiffIO tc(tcfile, FLOAT_TYPE);
	if(!ang.compareTiff(tc)){
		printf("File sizes do not match\n%s\n",tcfile);
		MPI_Abort(MCW,5);
		return 1;  
	} 
	tcData = CreateNewPartition(tc.getDatatype(), totalX, totalY, dxA, dyA, tc.getNodata());
	tc.read(xstart, ystart, tcData->getny(), tcData->getnx(), tcData->getGridPointer());

	//if using concentration grid, get information from file	
	tdpartition *cinData;
	if( usec == 1){		
		tiffIO cin(cinfile, FLOAT_TYPE);
		if(!ang.compareTiff(cin)) {
			printf("File sizes do not match\n%s\n",cinfile);
			MPI_Abort(MCW,5);
			return 1;  
		}
		cinData = CreateNewPartition(cin.getDatatype(), totalX, totalY, dxA, dyA, cin.getNodata());
		cin.read(xstart, ystart, cinData->getny(), cinData->getnx(), cinData->getGridPointer());
	}

	//Begin timer
	double readt = MPI_Wtime();

	//Convert geo coords to grid coords
	int *outletsX=0, *outletsY=0;
	if(useOutlets==1) {
		outletsX = new int[numOutlets];
		outletsY = new int[numOutlets];
		for( int i=0; i<numOutlets; i++)
			ang.geoToGlobalXY(x[i], y[i], outletsX[i], outletsY[i]);
	}

	//Create empty partition to store new information
	tdpartition *tla;
	tla = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	tdpartition *dep;
	dep = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA,  MISSINGFLOAT);
	
	tdpartition *csout;
	if(usec==1){			
			csout = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA,  MISSINGFLOAT);
	}

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
	if(usec==1) cinData->share(); //share is not needed, the program does not change this
	tsupData->share();
	tcData->share();
	neighbor->clearBorders();

	node temp;
	queue<node> que;

	initNeighborDinfup(neighbor,flowData,&que,nx, ny, useOutlets, outletsX, outletsY, numOutlets);
	finished = false;
	FILE *fp;
	
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()) 
		{
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;
			//  FLOW ALGEBRA EXPRESSION EVALUATION		
		
			if(flowData->isInPartition(i,j)  && (!tsupData->isNodata(i,j)) && (!tcData->isNodata(i,j))){
			  if(usec==0 || !cinData->isNodata(i,j)){
				//  Initialize the result
				transin=0.;
				loadin=0. ;
				con=false;  // not contaminated so far
				for(k=1; k<=8; k++) {
					in = i+d1[k];
					jn = j+d2[k];
					if(!flowData->hasAccess(in,jn) || flowData->isNodata(in,jn))
						con=true;
					else{
						flowData->getData(in,jn, angle);
						flowData->getdxdyc(jn,tempdxc,tempdyc);
						p = prop(angle, (k+4)%8,tempdxc,tempdyc);
						if(p>0.){
							float neighbor_transport=0.0;
							if(tla->isNodata(in,jn))con=true;
							else transin=transin+p*tla->getData(in,jn,neighbor_transport);
							if(usec==1)
							{
								if(csout->isNodata(in,jn))con=true;
								
								else {  // Here load of contaminant coming from the neighbor is sediment transport from neighbor * concentration of neighbor
								loadin=loadin+p*neighbor_transport*csout->getData(in,jn,tempFloat);
								}
							}
						}
					}
				}
				//  Local inputs
				tsupData->getData(i,j,tsupp);
				tcData->getData(i,j,tcc);
				float depp;
				if((transin+tsupp) > tcc)
				{
					transout=tcc;
					depp=transin+tsupp-transout;
				}
				else
				{
					transout=transin+tsupp;
					depp=0.;
				}
				tla->setData(i,j,transout);
				dep->setData(i,j,depp);
				if(usec==1)
				{
					
					if(transout < transin) // no erosion from cell
					{
						if(transin > 0)loadout=loadin*transout/transin;
						else loadout=0;
					}
					else
						loadout=loadin+cinData->getData(i,j,tempFloat)*(transout-transin);
					if(transout > 0.)
						csout->setData(i,j,(float)(loadout/transout));
					else
						csout->setData(i,j,(float)(0.0));
				}
				if(con && contcheck == 1)
				{
					dep->setToNodata(i,j);
					tla->setToNodata(i,j);
					if(usec==1)csout->setToNodata(i,j);
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
		tla->share();
		//  dep->share();  Not needed as not used in calculations
		if(usec==1) csout->share();
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
		// Clear out borders
		neighbor->clearBorders();

		//Check if done
		finished = que.empty();
		finished = tla->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float scaNodata = MISSINGFLOAT;
	tiffIO ttla(tlafile, FLOAT_TYPE, &scaNodata, ang);
	ttla.write(xstart, ystart, ny, nx, tla->getGridPointer());

	tiffIO ddep(depfile, FLOAT_TYPE, &scaNodata, ang);
	ddep.write(xstart, ystart, ny, nx, dep->getGridPointer());

	if(usec == 1){
		tiffIO ccsout(coutfile, FLOAT_TYPE, &scaNodata, ang);
		ccsout.write(xstart, ystart, ny, nx, csout->getGridPointer());
	}

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
