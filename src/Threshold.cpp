/*  Threshold function to evaluate grid cells >= input threshold value.
     
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

int threshold(char *ssafile,char *srcfile,char *maskfile, float thresh, int usemask)
{
	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("Threshold version %s\n",TDVERSION);

	double begin,end;

	//Create tiff object, read and store header info
	tiffIO ssa(ssafile, FLOAT_TYPE);
	long totalX = ssa.getTotalX();
	long totalY = ssa.getTotalY();
	double dxA = ssa.getdxA();
	double dyA = ssa.getdyA();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Create partition and read data
	tdpartition *ssaData;
	ssaData = CreateNewPartition(ssa.getDatatype(), totalX, totalY, dxA, dyA, ssa.getNodata());
	int nx = ssaData->getnx();
	int ny = ssaData->getny();
	int xstart, ystart;
	ssaData->localToGlobal(0, 0, xstart, ystart);
	ssa.read(xstart, ystart, ny, nx, ssaData->getGridPointer());

	//Mask 
	tdpartition *maskData;
	if( usemask == 1){
		tiffIO mask(maskfile, FLOAT_TYPE);
		if(!ssa.compareTiff(mask)) return 1;  //And maybe an unhappy error message
		maskData = CreateNewPartition(mask.getDatatype(), totalX, totalY, dxA, dyA, mask.getNodata());
		mask.read(xstart, ystart, maskData->getny(), maskData->getnx(), maskData->getGridPointer());
	}
	
	//Begin timer
	begin = MPI_Wtime();

	//Create empty partition to store new information
	tdpartition *src;
	src = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, -32768);

	long i,j;
	float tempssa=0;
	float tempmask=0;
	short tempsrc=0;
	
	//Share information and set borders to zero
	ssaData->share();
	if(usemask == 1) maskData->share();
	src->clearBorders();

// Compute sa		
	for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++ ) {
				
				if(usemask == 1){					
					if(ssaData->isNodata(i,j)) src->setToNodata(i,j);
					else{
						maskData->getData(i,j,tempmask);
						ssaData->getData(i,j,tempssa);
						tempsrc=((tempssa >= thresh) & (tempmask >= 0))?1:0;
						src->setData(i,j,tempsrc);
					}				
				}
				else{
					if(ssaData->isNodata(i,j)) src->setToNodata(i,j);
					else{
						ssaData->getData(i,j,tempssa);
						tempsrc=(tempssa >= thresh)?1:0;
						src->setData(i,j,tempsrc);
					}
				}				
			}
	}
		//Pass information
		src->addBorders();		

		//Clear out borders
		src->clearBorders();

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
	tiffIO srcc(srcfile, SHORT_TYPE, &aNodata, ssa);
	srcc.write(xstart, ystart, ny, nx, src->getGridPointer());

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}

