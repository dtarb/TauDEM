/*  Edit Raster function to edit grid cell values and make minor changes where necessary.

David Tarboton
Utah State University
November 24 2016

*/

/*  Copyright (C) 2016  David Tarboton, Utah State University

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


int editraster(char *rasterfile, char *newfile, char *changefile)
{
	MPI_Init(NULL,NULL);{

	//Only used for timing
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("Editraster version %s\n",TDVERSION);

	double begin,end;

	//Create tiff object, read and store header info
	tiffIO rasterIO(rasterfile , SHORT_TYPE);
	long totalX = rasterIO.getTotalX();
	long totalY = rasterIO.getTotalY();
	double dxA = rasterIO.getdxA();
	double dyA = rasterIO.getdyA();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			//fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"Time estimate not available.\n");
			fflush(stderr);
		}

	//Create partition and read data
	tdpartition *rasterData;
	rasterData = CreateNewPartition(rasterIO.getDatatype(), totalX, totalY, dxA, dyA, rasterIO.getNodata());
	int nx = rasterData->getnx();
	int ny = rasterData->getny();
	int xstart, ystart;
	rasterData->localToGlobal(0, 0, xstart, ystart);
	rasterIO.read(xstart, ystart, ny, nx, rasterData->getGridPointer());

	//Begin timer
	begin = MPI_Wtime();

	//Create empty partition to store new information
	tdpartition *newvals;
	newvals = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, (int16_t)-32768);

	// replicate		
	for (long j = 0; j < ny; j++) {
		for (long i = 0; i < nx; i++) {
			if (rasterData->isNodata(i, j)) newvals->setToNodata(i, j);
			else {
				short tempshort;
				newvals->setData(i, j, rasterData->getData(i, j, tempshort));
			}
		}
	}


	// read values to change
	FILE *fp;
	fp = fopen(changefile, "r");
	char headers[MAXLN];
	readline(fp, headers);
	stack<float> xs, ys;
	stack<short> vals;
	double x, y;
	short val;
	int n = 0;
	while (fscanf(fp, "%lf,%lf,%d", &x, &y, &val) == 3)
	{
		int32_t gx, gy, tx, ty;
		rasterIO.geoToGlobalXY(x,y, gx, gy);
		rasterData->globalToLocal(gx, gy, tx, ty);
		if (rasterData->isInPartition(tx, ty))
		{
			printf("Process: %d changing %lf, %lf, %d\n",rank,x,y,val);
			fflush(stdout);
			newvals->setData(tx, ty, val);
		}
	}

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
	tiffIO newIO(newfile, SHORT_TYPE, aNodata, rasterIO);
	newIO.write(xstart, ystart, ny, nx, newvals->getGridPointer());

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}

