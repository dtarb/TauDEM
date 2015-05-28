/*  SlopeArea function that evaluates S^m A^n based on slope and specific catchment area 
  grid inputs, and parameters m and n.  This is intended for use with the slope-area stream 
  raster delineation method.
     
  David Tarboton, Teklu K. Tesfa
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


int slopearea(char *slopefile, char*scafile, char *safile, float *p)
{
	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("SlopeArea version %s\n",TDVERSION);

	double begin,end;

	//Create tiff object, read and store header info
	tiffIO slp(slopefile, FLOAT_TYPE);
	long totalX = slp.getTotalX();
	long totalY = slp.getTotalY();
	double dxA = slp.getdxA();
	double dyA = slp.getdyA();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


	//Create partition and read data
	tdpartition *slpData;
	slpData = CreateNewPartition(slp.getDatatype(), totalX, totalY, dxA, dyA, slp.getNodata());
	int nx = slpData->getnx();
	int ny = slpData->getny();
	int xstart, ystart;
	slpData->localToGlobal(0, 0, xstart, ystart);
	slp.read(xstart, ystart, ny, nx, slpData->getGridPointer());

	tdpartition *scaData;
	tiffIO sca(scafile, FLOAT_TYPE);
	if(!slp.compareTiff(sca)) return 1;  //And maybe an unhappy error message
	scaData = CreateNewPartition(sca.getDatatype(), totalX, totalY, dxA, dyA, sca.getNodata());
	sca.read(xstart, ystart, scaData->getny(), scaData->getnx(), scaData->getGridPointer());
	
	//Begin timer
	begin = MPI_Wtime();

	//Create empty partition to store new information
	tdpartition *sa;
	sa = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, -1.0f);

	// con is used to check for contamination at the edges
	long i,j;
	float tempslp=0;
	float tempsca=0;
	float tempsa=0;
	
	//Share information and set borders to zero
	slpData->share();
	scaData->share();
	sa->clearBorders();

// Compute sa		
	for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++ ) {
				slpData->getData(i,j,tempslp);
				scaData->getData(i,j,tempsca);
				if(tempslp >= 0. && tempsca >= 0.){
					tempsa=pow(tempslp,p[0]) * pow(tempsca,p[1]);
					sa->setData(i,j,tempsa);
				}
				else
					sa->setToNodata(i,j);
			}
		}

		//Pass information
		sa->addBorders();		

		//Clear out borders
		sa->clearBorders();

	//Stop timer
	end = MPI_Wtime();
 	double compute, temp;
        compute = end-begin;

        MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = temp/size;


        if( rank == 0)
                printf("Compute time: %f\n",compute);

	//Create and write TIFF file
	float aNodata = -1.0f;
	tiffIO saa(safile, FLOAT_TYPE, &aNodata, slp);
	saa.write(xstart, ystart, ny, nx, sa->getGridPointer());

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}
