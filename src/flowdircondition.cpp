/*  TauDEM D8FlowDir function to compute flow direction based on d8 flow model.
     
  David G Tarboton, Dan Watson, Jeremy Neff
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
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
using namespace std;




//Open files, Initialize grid memory, makes function calls to set flowDir, slope, and resolvflats, writes files
int flowcondition( char* demfile, char *flowfile, int useflowfile,char* zsbfile) {

	MPI_Init(NULL,NULL);{

	//Only needed to output time
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)
	{
		printf("D8FlowDir version %s\n",TDVERSION);
		fflush(stdout);
	}
		double begin,end;
	

	//Read DEM from file
	tiffIO dem(demfile, FLOAT_TYPE);
	long totalX = dem.getTotalX();
	long totalY = dem.getTotalY();
	double dxA = dem.getdxA();
	double dyA = dem.getdyA();
	

	tdpartition *elevDEM;
	elevDEM = CreateNewPartition(dem.getDatatype(), totalX, totalY, dxA, dyA, dem.getNodata());
	int xstart, ystart;
	int nx = elevDEM->getnx();
	int ny = elevDEM->getny();
	elevDEM->localToGlobal(0, 0, xstart, ystart);
	elevDEM->savedxdyc(dem);


	tiffIO p(flowfile,SHORT_TYPE);
	tdpartition *flowData;
	flowData = CreateNewPartition(p.getDatatype(), totalX, totalY, dxA, dyA, p.getNodata()); 
	//int nx = flowData->getnx();
	//int ny = flowData->getny();
	//int xstart, ystart;
	flowData->localToGlobal(0, 0, xstart, ystart);
	p.read(xstart, ystart, ny, nx, flowData->getGridPointer());
	//double headert = MPI_Wtime();

	if(rank==0)
	{
		float timeestimate=(2.8e-9*pow((double)(totalX*totalY),1.55)/pow((double) size,0.65))/60+1;  // Time estimate in minutes
		//fprintf(stderr,"%d %d %d\n",totalX,totalY,size);
		fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
		fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
		fflush(stderr);
	}

	begin = MPI_Wtime();
	tdpartition *sar;
	sar = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, -1.0f);

	//long i,j;

	float tempsar=0;

	dem.read(xstart, ystart, ny, nx, elevDEM->getGridPointer());
	elevDEM->share();


	
	//Creates empty partition to store new flow direction 
	//tdpartition *flowDir;
	//short flowDirNodata = -32768;
	//flowDir = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, flowDirNodata);

//	flowDir = new linearpart<short>;

//	flowDir->init(totalX, totalY, dx, dy, MPI_SHORT, short(-32768));

	//If using flowfile is enabled, read it in
	//tdpartition *imposedflow, *area;
	//area = new linearpart<int32_t>;
	//area->init(totalX, totalY, dxA, dyA, MPI_LONG, int32_t(-1));

	//if( useflowfile == 1) {
		//tiffIO flow(flowfile,SHORT_TYPE);
		//imposedflow = CreateNewPartition(flow.getDatatype(), flow.getTotalX(), flow.getTotalY(), flow.getdxA(), flow.getdyA(), flow.getNodata());

		//if(!dem.compareTiff(flow)) {
		//	printf("Error using imposed flow file.\n");	
		//	return 1;
		//}
		
	flowData->share();
	elevDEM->share();
	sar->clearBorders();

		//Compute sar	
	int i,j;short tempslp;float elev1;
	for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++ ) {
				flowData->getData(i,j,tempslp);
				elevDEM->getData(i,j,elev1);
				if(!flowData->isNodata(i,j)){
					if(tempslp>0){
					   tempsar=elev1;
					   sar->setData(i,j,tempsar);
					}
					else
					   sar->setToNodata(i,j);				
				}
				else
					sar->setToNodata(i,j);
			}
		}
		//delete imposedflow;
		//TODO - why is this here?
		//darea( &flowDir, &area, NULL, NULL, 0, 1, NULL, 0, 0 );
	
//Pass information
		sar->addBorders();		

		//Clear out borders
		sar->clearBorders();

		//Stop timer
	end = MPI_Wtime();
	double compute, temp;
        compute = end-begin;

        MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = temp/size;


        if( rank == 0)
                printf("Compute time: %f\n",compute);

	//Create and write TIFF file
	//Create and write TIFF file
	float aNodata = -1.0f;
	char prefix[5] = "zsb";
	tiffIO sarr(zsbfile, FLOAT_TYPE, &aNodata,p);
	sarr.write(xstart, ystart, ny, nx, sar->getGridPointer());

	
	//if( rank == 0)
		//  These times are only for  process 0 - not averaged across processors.  This may be an approximation - but probably do not want to hold processes up to synchronize just so as to get more accurate timing
		//printf("Processors: %d\nHeader read time: %f\nData read time: %f\nCompute Slope time: %f\nWrite Slope time: %f\nResolve Flat time: %f\nWrite Flat time: %f\nTotal time: %f\n",
		//  size,headerRead,dataRead, computeSlope, writeSlope,computeFlat,writeFlat,total);
	}
	MPI_Finalize();
	return 0;
}

