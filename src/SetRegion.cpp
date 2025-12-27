/*  Set Region function to set grid cell values for a grid processing region as part of RWD.

David Tarboton
Utah State University
Jan 15, 2017

This function takes as input a file defining regions (gwfile) and a flow direction grid
The output is a new region grid that adds downstream grid cells with flow direction nodata that the region drains into.
The logic is
   if  gwData is nodata
     if pData is notata
	   if neighbor drains to cell set region to region of neighbor
   else
     use existing gwdata

*/

/*  Copyright (C) 2017  David Tarboton, Utah State University

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
#include <stack>
using namespace std;

/*********************************************************************/
int readline(FILE *fp, char *fline)
{
	int i = 0, ch;

	for (i = 0; i< MAXLN; i++)
	{
		ch = getc(fp);

		if (ch == EOF) { *(fline + i) = '\0'; return(EOF); }
		else          *(fline + i) = (char)ch;

		if ((char)ch == '\n') { *(fline + i) = '\0'; return(0); }

	}
	return(1);
}


int setregion(char *fdrfile, char *regiongwfile, char *newfile, int32_t regionID)
{
	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("SetRegion version %s\n",TDVERSION);

	double begin,end;
	//Begin timer
	begin = MPI_Wtime();

	//Create tiff object, read and store header info
	tiffIO fdrIO(fdrfile , SHORT_TYPE);
	long totalX = fdrIO.getTotalX();
	long totalY = fdrIO.getTotalY();
	double dxA = fdrIO.getdxA();
	double dyA = fdrIO.getdyA();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			//fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"Time estimate not available.\n");
			fflush(stderr);
		}

	//Create partition and read fdr data
	tdpartition *pData;
	pData = CreateNewPartition(fdrIO.getDatatype(), totalX, totalY, dxA, dyA, fdrIO.getNodata());
	int nx = pData->getnx();
	int ny = pData->getny();
	int xstart, ystart;
	pData->localToGlobal(0, 0, xstart, ystart);
	fdrIO.read(xstart, ystart, ny, nx, pData->getGridPointer());



	//Create partition and read gw data
	tdpartition *gwData;
	tiffIO gwIO(regiongwfile, LONG_TYPE);
	if (!fdrIO.compareTiff(gwIO)) {
			printf("File sizes do not match\n%s\n", regiongwfile);
			fflush(stdout);
			MPI_Abort(MCW, 5);
			return 1;
	}
	gwData = CreateNewPartition(gwIO.getDatatype(), totalX, totalY, dxA, dyA, gwIO.getNodata());
	gwIO.read(xstart, ystart, ny, nx, gwData->getGridPointer());


	//Create empty partition to store new information
	tdpartition *newvals;
	newvals = CreateNewPartition(LONG_TYPE, totalX, totalY, dxA, dyA, MISSINGLONG);

	//Share information across partitions
	gwData->share();
	pData->share();


	// replicate		
	for (long j = 0; j < ny; j++) {
		for (long i = 0; i < nx; i++) {
			newvals->setToNodata(i, j);
			if (gwData->isNodata(i, j)) {
				if (pData->isNodata(i, j)) {
					// Check neighbors
					for (int k = 1; k <= 8; k++) {
						long in = i + d1[k];
						long jn = j + d2[k];
						if (pData->hasAccess(in, jn) && !pData->isNodata(in, jn)) {
							short dirnb;
							pData->getData(in, jn, dirnb);
							if (dirnb > 0 && abs(dirnb - k) == 4) { // The neighbor drains to the cell so it is in the region
								newvals->setData(i, j, regionID);
								break;
							}
						}
					}
				}
				else
					newvals->setData(i, j, regionID);
			}
			else {
				newvals->setData(i, j,regionID);
			}
		}
	}


	//Create and write TIFF file
	long aNodata = MISSINGLONG;
	tiffIO newIO(newfile, LONG_TYPE, aNodata, gwIO);
	newIO.write(xstart, ystart, ny, nx, newvals->getGridPointer());


	//Stop timer
	end = MPI_Wtime();
	double compute, temp;
	compute = end - begin;

	MPI_Allreduce(&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	compute = temp / size;


	if (rank == 0)
		printf("Compute time: %f\n", compute);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}

