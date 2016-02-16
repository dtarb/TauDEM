/*  Taudem AreaD8 functions 

  David Tarboton, Dan Watson
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
#include <iostream>
#include "initneighbor.h"
using namespace std;


int aread8( char* pfile, char* afile, char *datasrc, char *lyrname,int uselyrname,int lyrno, char *wfile, int useOutlets, int usew, int contcheck) {

	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("AreaD8 version %s\n",TDVERSION);
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
	if( useOutlets == 1) {
		if(rank==0){
			if(readoutlets(datasrc,lyrname,uselyrname,lyrno,hSRSRaster, &numOutlets, x, y)==0){
				usingShapeFile=true;
				MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
				MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
			}
			else {
				printf("Error opening shapefile. Exiting \n");
				MPI_Abort(MCW,5);
			}
		}
		else {
			MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
			x = new double[numOutlets];
			y = new double[numOutlets];
			MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
			MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
			usingShapeFile=true;
		}
	}

	//Create tiff object, read and store header info



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
	p.read(xstart, ystart, ny, nx, flowData->getGridPointer());

	//if using weightData, get information from file
	tdpartition *weightData;
	if( usew == 1){
		tiffIO w(wfile,FLOAT_TYPE);
		if(!p.compareTiff(w)){
			printf("File sizes do not match\n%s\n",wfile);
			MPI_Abort(MCW,5);
			return 1;  
		} 
		weightData = CreateNewPartition(w.getDatatype(), totalX, totalY, dxA, dyA, w.getNodata()); 
		w.read(xstart, ystart, weightData->getny(), weightData->getnx(), weightData->getGridPointer());
	}

	//Begin timer
	double readt = MPI_Wtime();
	

	//Convert geo coords to grid coords
	int *outletsX; int *outletsY;
	if(usingShapeFile) {
		outletsX = new int[numOutlets];
		outletsY = new int[numOutlets];
	
		for( int i=0; i<numOutlets; i++){
	 
			p.geoToGlobalXY(x[i], y[i], outletsX[i], outletsY[i]);
		 
		}
	}

	//Create empty partition to store new information
	tdpartition *aread8;
	aread8 = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, -1.0f); // modified by Nazmus

	// con is used to check for contamination at the edges
	long i,j;
	short k;
	long in,jn;
	float area;
	bool con=false, finished;
	float tempFloat=0;
	short tempShort=0;

	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT); //modified by Nazmus
	
	//Share information and set borders to zero
	flowData->share();
	if(usew==1) weightData->share();
	aread8->clearBorders();
	neighbor->clearBorders();

	node temp;
	queue<node> que;

	initNeighborD8up(neighbor,flowData,&que,nx, ny, useOutlets, outletsX, outletsY, numOutlets);
	
	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()){
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;
			
			if(flowData->isInPartition(i,j)){
				//  FLOW ALGEBRA Evaluation
				//   Initialize
				if( usew==1) 
				{
					if(!weightData->isNodata(i,j))
						aread8->setData(i,j, weightData->getData(i,j,tempFloat));
				}
				else aread8->setData(i,j,(float)1);
				con=false;  //  Initially not contaminated
				for(k=1; k<=8; k++) {
					in = i+d1[k];
					jn = j+d2[k];
					if(!flowData->hasAccess(in,jn) || flowData->isNodata(in,jn))
						con=true;
					else
					{
						flowData->getData(in,jn,tempShort);
						if(tempShort-k == 4 || tempShort-k == -4)
						{
							if(aread8->isNodata(in,jn))con=true;
							else
							{
								aread8->addToData(i,j,aread8->getData(in,jn,tempFloat));
							}
						}
					}
				}
				if(con && contcheck == 1)aread8->setToNodata(i,j);
			}
			//  END FLOW ALGEBRA EXPRESSION
			// Decrement neighbor dependence of downslope cell
				
			flowData->getData(i,j,k);
			if(k>=1 && k <=8){
				//continue;
				in = i+d1[k];
				jn = j+d2[k];
				neighbor->addToData(in,jn,(short)-1);
				//Check if neighbor needs to be added to que
				if(flowData->isInPartition(in,jn) && neighbor->getData(in, jn, tempShort) == 0 ){
					temp.x=in;
					temp.y=jn;
					que.push(temp);
				}
			}else{
				if(flowData->isNodata(i,j))
					cout << "Warning: evaluating at location (most likely specified outlet) where flow direction is undefined. i = " << i << ", j = " << j <<  ", rank = " << rank << endl;
				else
					cout << "Warning: Invalid flow direction = " << k << " encountered at i = " << i << ", j = " << j <<  ", rank = " << rank << endl;
			}
		}

		//Pass information
		aread8->share();
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
		finished = aread8->ringTerm(finished);
	}

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float aNodata = -1.0f;
	tiffIO a(afile, FLOAT_TYPE, &aNodata, p);
	a.write(xstart, ystart, ny, nx, aread8->getGridPointer());
	double writet = MPI_Wtime();
	if( rank == 0) 
		printf("Number of Processes: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
		  size,readt-begint, computet-readt, writet-computet,writet-begint);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();


	return 0;
}
