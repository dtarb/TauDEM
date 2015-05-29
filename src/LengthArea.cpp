/*  LengthAreamn function that evaluates A >= M L^y ? 1:0 based on upslope path length 
  and D8 contributing area grid inputs, and parameters M and y.  This is intended for use 
  with the length-area stream raster delineation method.  

  David Tarboton, Teklu Tesfa
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

int lengtharea(char *plenfile, char*ad8file, char *ssfile, float *p)
{
	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("LengthArea version %s\n",TDVERSION);

	double begin,end;

	//Create tiff object, read and store header info
	tiffIO plen(plenfile, FLOAT_TYPE);
	long totalX = plen.getTotalX();
	long totalY = plen.getTotalY();
	double dxA = plen.getdxA();
	double dyA = plen.getdyA();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


	//Create partition and read data
	tdpartition *plenData;
	plenData = CreateNewPartition(plen.getDatatype(), totalX, totalY, dxA, dyA, plen.getNodata());
	int nx = plenData->getnx();
	int ny = plenData->getny();
	int xstart, ystart;
	plenData->localToGlobal(0, 0, xstart, ystart);
	plen.read(xstart, ystart, ny, nx, plenData->getGridPointer());

	tdpartition *ad8Data;
	tiffIO ad8(ad8file, LONG_TYPE);
	if(!plen.compareTiff(ad8)) return 1;  //And maybe an unhappy error message
	ad8Data = CreateNewPartition(ad8.getDatatype(), totalX, totalY, dxA, dyA, ad8.getNodata());
	ad8.read(xstart, ystart, ad8Data->getny(), ad8Data->getnx(), ad8Data->getGridPointer());
	
	//Begin timer
	begin = MPI_Wtime();

	//Create empty partition to store new information
	tdpartition *ss;
	ss = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);

	long i,j;
	float tempplen=0.0;
	int32_t tempad8=0;
	short tempss=0;
	
	//Share information and set borders to zero
	plenData->share();
	ad8Data->share();
	ss->clearBorders();

// Compute sa		
	for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++ ) {
				plenData->getData(i,j,tempplen);
				ad8Data->getData(i,j,tempad8);				
				if(tempplen >= 0.0){					
					tempss = (tempad8 >= (p[0]* pow(tempplen,p[1]))) ? 1: 0;
					ss->setData(i,j,tempss);
				}
				
			}
 		}

		//Pass information
		ss->addBorders();		

		//Clear out borders
		ss->clearBorders();

	//Stop timer
	end = MPI_Wtime();
 	double compute, temp;
        compute = end-begin;

        MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = temp/size;


	if( rank == 0) 
		printf("Compute time: %f\n",compute);

	//Create and write TIFF file
	short aNodata = -32768;
	tiffIO sss(ssfile, SHORT_TYPE, &aNodata, ad8);
	sss.write(xstart, ystart, ny, nx, ss->getGridPointer());

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}

