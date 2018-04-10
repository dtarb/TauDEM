/*  CatchHydroGeo function evaluates channel hydraulic properties
  based on HAND(Height Above Nearest Drainage), D-inf slope, 
  and catchment grid inputs, reachid list file, and a stage height text file.

  David Tarboton, Xing Zheng, Yan Liu
  Utah State University, University of Texas at Austin, NCSA/UIUC
  Nov 01, 2016 
  
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

#include <unordered_map>
#include <mpi.h>
#include <math.h>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
using namespace std;

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

int catchhydrogeo(char *handfile, char*catchfile, char*catchlistfile, char *slpfile, char *hfile, char *hpfile)
{
	MPI_Init(NULL, NULL); {

		int rank, size;
		MPI_Comm_rank(MCW, &rank);
		MPI_Comm_size(MCW, &size);
		if (rank == 0)printf("CatchHydroGeo version %s\n", TDVERSION);

		double begin, end;
		//Begin timer
		begin = MPI_Wtime();

		//Create tiff object, read and store header info
		tiffIO hand(handfile, FLOAT_TYPE);
		long totalX = hand.getTotalX();
		long totalY = hand.getTotalY();
		double dxA = hand.getdxA();
		double dyA = hand.getdyA();
		if (rank == 0)
		{
			float timeestimate = (1e-7*totalX*totalY / pow((double)size, 1)) / 60 + 1;  // Time estimate in minutes
			fprintf(stderr, "This run may take on the order of %.0f minutes to complete.\n", timeestimate);
			fprintf(stderr, "This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

		//Create partition and read data
		//Read hand raster
		tdpartition *handData;
		handData = CreateNewPartition(hand.getDatatype(), totalX, totalY, dxA, dyA, hand.getNodata());
		int nx = handData->getnx();
		int ny = handData->getny();
		int xstart, ystart;
		handData->localToGlobal(0, 0, xstart, ystart);
		handData->savedxdyc(hand);  // This saves the local cell dimensions from the IO object
		hand.read(xstart, ystart, ny, nx, handData->getGridPointer());

		//Read catchment raster
		tdpartition *catchData;
		tiffIO catchment(catchfile, LONG_TYPE);
		if (!hand.compareTiff(catchment)) return 1;  //And maybe an unhappy error message
		catchData = CreateNewPartition(catchment.getDatatype(), totalX, totalY, dxA, dyA, catchment.getNodata());
		catchment.read(xstart, ystart, catchData->getny(), catchData->getnx(), catchData->getGridPointer());

		//Read slope raster
		tdpartition *slpData;
		tiffIO slp(slpfile, FLOAT_TYPE);
		if (!hand.compareTiff(slp)) return 1;  //And maybe an unhappy error message
		slpData = CreateNewPartition(slp.getDatatype(), totalX, totalY, dxA, dyA, slp.getNodata());
		slp.read(xstart, ystart, slpData->getny(), slpData->getnx(), slpData->getGridPointer());

		FILE *fp;
		char headers[MAXLN];
		// get catch list and build hash table
		int ncatch; int * catchlist;
		double *slopelist; double *linelenlist; double *areasqkmlist;
		if (rank == 0) {
			ncatch=0;
			fp = fopen(catchlistfile, "r");
			int v1; double v2; double v3; double v4;
            int i=0;
			char line1[80];
			readline(fp, line1);
			sscanf(line1, "%d", &ncatch);
			if (ncatch <= 0) {
				fprintf(stderr, "ERROR: catch list empty!\n");
				exit(1);
			} 
			catchlist = (int *) malloc(sizeof(int) * ncatch);
			slopelist = (double *) malloc(sizeof(double) * ncatch);
			linelenlist = (double *) malloc(sizeof(double) * ncatch);
			areasqkmlist = (double *) malloc(sizeof(double) * ncatch);
			while (fscanf(fp, "%d %lf %lf %lf\n", &v1, &v2, &v3, &v4) != EOF) {
				catchlist[i] = v1;
				slopelist[i] = v2;
				linelenlist[i] = v3;
				areasqkmlist[i] = v4;
				i++;
			}
			fclose(fp);
		}
		MPI_Bcast(&ncatch, 1, MPI_INT, 0, MCW);
		if (rank != 0) catchlist = (int *) malloc(sizeof(int) * ncatch);
		MPI_Bcast(catchlist, ncatch, MPI_INT, 0, MCW);
		unordered_map<int, int> catchhash;
		for (int i=0; i<ncatch; i++) catchhash[catchlist[i]] = i;

		//Read stage table
		int nheight;
		//vector<float> h(nheight);  //DGT this does not work because nheight is still 0 here
		if (rank == 0) {
			nheight = 0;
			fp = fopen(hfile, "r");
			//Count the number of lines 
			while (fgets(headers, sizeof headers, fp) != NULL)
			{
				nheight++;
			}
			fclose(fp);
			nheight = nheight - 1;  // First line is header
		}
		//Read stage into vector
		MPI_Bcast(&nheight, 1, MPI_INT, 0, MCW);
		double *height = (double *) malloc(sizeof(double) * nheight); 
		if (rank == 0) {
			fp = fopen(hfile, "r");
			readline(fp, headers);
			for(int j=0; j<nheight; j++)
			{
				fscanf(fp, "%lf\n", &height[j]);
			} 
			fclose(fp);
		}
		MPI_Bcast(height, nheight, MPI_DOUBLE, 0, MCW);
#ifdef DEBUG
		printf("CatchHydroGeo %d: startx=%d starty=%d nx=%d ny=%d; nheight=%d, [0]=%.6lf; ncatch=%ld\n", rank, xstart, ystart, nx, ny, nheight, height[0], catchhash.size());
#endif

		//Create output vectors
		int *CellCount = new int[nheight * ncatch]; // row - height; column - catchid
		double *SurfaceArea = new double[nheight * ncatch];
		double *BedArea = new double[nheight * ncatch];
		double *Volume = new double[nheight * ncatch];
		for (int i=0; i<nheight; i++) {
			for (int j=0; j<ncatch; j++) {
				CellCount[i*ncatch + j] = 0;
				SurfaceArea[i*ncatch + j] = 0.0;
				BedArea[i*ncatch + j] = 0.0;
				Volume[i*ncatch + j] = 0.0;
			}
		}

		long i, j, k;
		float temphand = 0.0;
		float tempslp = 0.0;
		int32_t tempcatch = 0;

		//Share information and set borders to zero
		handData->share();
		catchData->share();
		slpData->share();

		// Compute hydraulic properties		
		for (j = 0; j < ny; j++) {
			for (i = 0; i < nx; i++) {
				if (!catchData->isNodata(i, j) && !handData->isNodata(i,j) && !slpData->isNodata(i,j)) {  // All 3 inputs have to have data
					handData->getData(i, j, temphand);
					catchData->getData(i, j, tempcatch);
					slpData->getData(i, j, tempslp);
					for (k = 0; k < nheight; k++) {
						if (temphand < height[k] || fabs(temphand - 0.0)<0.000001) {  // DGT prefers strictly less than here.  If the depth is 0, treat it as wet, too 
							CellCount[k*ncatch + catchhash[tempcatch]] += 1;
							double dxc, dyc, cellArea;
							handData->getdxdyc(j, dxc, dyc);  // This function gets latitude dependent dx and dy for each cell, better than averages
							cellArea = dxc*dyc;
							SurfaceArea[k*ncatch + catchhash[tempcatch]] += cellArea;
							BedArea[k*ncatch + catchhash[tempcatch]] += cellArea*sqrt(1 + tempslp*tempslp);
							Volume[k*ncatch + catchhash[tempcatch]] +=  (height[k] - temphand)*cellArea;
						}
					}
				}
			}
		}

		//MPI output reduce
		int *GCellCount = new int[nheight * ncatch]; // row - height; column - catchid
		double *GSurfaceArea = new double[nheight * ncatch];
		double *GBedArea = new double[nheight * ncatch];
		double *GVolume = new double[nheight * ncatch];
		// TODO may be able to do Reduce as a vector operation without loops
		MPI_Reduce(CellCount, GCellCount, nheight * ncatch, MPI_INT, MPI_SUM, 0, MCW);
		MPI_Reduce(SurfaceArea, GSurfaceArea, nheight * ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);
		MPI_Reduce(BedArea, GBedArea, nheight * ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);
		MPI_Reduce(Volume, GVolume, nheight * ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);
		//Write results
		if (rank == 0) {
			FILE *fp;
			fp = fopen(hpfile, "w");
			fprintf(fp, "CatchId, Stage, Number of Cells, SurfaceArea (m2), BedArea (m2), Volume (m3), SLOPE, LENGTHKM, AREASQKM\n");
			int i, j;
			for (i = 0; i < ncatch; i++) {
				for (j = 0; j < nheight; j++) {
					fprintf(fp, "%d, ", catchlist[i]);
					fprintf(fp, "%.6lf, ", height[j]);
					fprintf(fp, "%d, ", GCellCount[j * ncatch + i]);
					fprintf(fp, "%.6lf, ", GSurfaceArea[j * ncatch + i]);
					fprintf(fp, "%.6lf, ", GBedArea[j * ncatch + i]);
					fprintf(fp, "%.6lf, ", GVolume[j * ncatch + i]);
					fprintf(fp, "%.10lf, ", slopelist[i]);
					fprintf(fp, "%.6lf, ", linelenlist[i]);
					fprintf(fp, "%.6lf\n", areasqkmlist[i]);
				}
			}
		}
        delete[] CellCount;
        delete[] SurfaceArea;
        delete[] BedArea;
        delete[] Volume;
        delete[] GCellCount;
        delete[] GSurfaceArea;
        delete[] GBedArea;
        delete[] GVolume;
		free(catchlist);
		free(slopelist);
		free(linelenlist);
		free(areasqkmlist);
		free(height);

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
