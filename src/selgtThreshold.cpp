/*  Threshold function to evaluate grid cells >= input threshold value.
     
  David Tarboton
  Utah State University  
  Sept 3, 2010 

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


int selgtthreshold(char *demfile, char *outfile, float thresh, int prow, int pcol)
{
	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("SELGTThreshold version %s\n",TDVERSION);

	double begin,end;

	//Create tiff object, read and store header info
	tiffIO dem(demfile, FLOAT_TYPE);
	long totalX = dem.getTotalX();
	long totalY = dem.getTotalY();
	double dx = dem.getdx();
	double dy = dem.getdy();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Create partition and read data
	tdpartition *demData;
	demData = CreateNewPartition(dem.getDatatype(), totalX, totalY, dx, dy, dem.getNodata());
	int nx = demData->getnx();
	int ny = demData->getny();
	int xstart, ystart;
	demData->localToGlobal(0, 0, xstart, ystart);
	dem.read(xstart, ystart, ny, nx, demData->getGridPointer());

	//Begin timer
	begin = MPI_Wtime();

// Compute new vales		
	for(long j=0; j<ny; j++) 
		for(long i=0; i<nx; i++ ) {		
			if(!demData->isNodata(i,j)) 
			{
				float tempFloat;
				if(demData->getData(i,j,tempFloat)<=thresh)demData->setToNodata(i,j);
			}
		}
	//Stop timer
	end = MPI_Wtime();
	if( rank == 0) 
		printf("Compute time: %f\n",end-begin);

	//Create and write TIFF file
	float aNodata = *(float*)dem.getNodata();
	tiffIO out(outfile, FLOAT_TYPE, &aNodata, dem);
	out.write(xstart, ystart, ny, nx, demData->getGridPointer(),"sgt",prow,pcol);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}

