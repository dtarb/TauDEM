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

		int ncatch; int *catchlist;
		double *slopelist; double *linelenlist; double *manningsnlist;
		int has_manningsn = 0;
		if (rank == 0) {
			ncatch = 0;
			fp = fopen(catchlistfile, "r");
			if (!fp) {
				fprintf(stderr, "ERROR: Cannot open catch list file!\n");
				exit(1);
			}
			char line[256];
			// Read header
			if (!fgets(line, sizeof(line), fp)) {
				fprintf(stderr, "ERROR: Empty catch list file!\n");
				exit(1);
			}
			// Count lines for ncatch
			long pos = ftell(fp);
			while (fgets(line, sizeof(line), fp)) {
				if (line[0] != '\n' && line[0] != '\0') ncatch++;
			}
			if (ncatch <= 0) {
				fprintf(stderr, "ERROR: catch list empty!\n");
				exit(1);
			}
			fseek(fp, pos, SEEK_SET); // rewind to first data line

			catchlist = (int *)malloc(sizeof(int) * ncatch);
			slopelist = (double *)malloc(sizeof(double) * ncatch);
			linelenlist = (double *)malloc(sizeof(double) * ncatch);
			manningsnlist = (double *)malloc(sizeof(double) * ncatch);

			int ncols = 3;
			// Peek at first data line to determine columns
			if (fgets(line, sizeof(line), fp)) {
				char *tok;
				int colcount = 0;
				char tmp[256];
				strcpy(tmp, line);
				tok = strtok(tmp, ",");
				while (tok) {
					colcount++;
					tok = strtok(NULL, ",");
				}
				ncols = colcount;
				fseek(fp, pos, SEEK_SET);
			}
			if (ncols < 3) {
				fprintf(stderr, "ERROR: Catchment list file must have at least 3 columns (id, slope, length).\n");
				exit(1);
			}
			has_manningsn = (ncols == 4);
			if (!has_manningsn) {
				fprintf(stderr, "INFO: Mannings n not found in catchment list file. Using default value of 0.05 for all catchments.\n");
			}

			int i = 0;
			while (fgets(line, sizeof(line), fp) && i < ncatch) {
				char *tok;
				tok = strtok(line, ",");
				if (!tok) continue;
				catchlist[i] = atoi(tok);

				tok = strtok(NULL, ",");
				if (!tok) {
					fprintf(stderr, "ERROR: Missing slope value for catchment id %d in catchment list file.\n", catchlist[i]);
					exit(1);
				}
				slopelist[i] = atof(tok);

				tok = strtok(NULL, ",");
				if (!tok) {
					fprintf(stderr, "ERROR: Missing length value for catchment id %d in catchment list file.\n", catchlist[i]);
					exit(1);
				}
				linelenlist[i] = atof(tok);

				if (has_manningsn) {
					tok = strtok(NULL, ",");
					if (tok) manningsnlist[i] = atof(tok);
					else manningsnlist[i] = 0.05;
				} else {
					manningsnlist[i] = 0.05;
				}
				i++;
			}
			fclose(fp);
		}
		MPI_Bcast(&ncatch, 1, MPI_INT, 0, MCW);
		if (rank != 0) {
			catchlist = (int *)malloc(sizeof(int) * ncatch);
			slopelist = (double *)malloc(sizeof(double) * ncatch);
			linelenlist = (double *)malloc(sizeof(double) * ncatch);
			manningsnlist = (double *)malloc(sizeof(double) * ncatch);
		}
		MPI_Bcast(catchlist, ncatch, MPI_INT, 0, MCW);
		MPI_Bcast(slopelist, ncatch, MPI_DOUBLE, 0, MCW);
		MPI_Bcast(linelenlist, ncatch, MPI_DOUBLE, 0, MCW);
		MPI_Bcast(manningsnlist, ncatch, MPI_DOUBLE, 0, MCW);
		unordered_map<int, int> catchhash;
		for (int i = 0; i < ncatch; i++) catchhash[catchlist[i]] = i;

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
		int *CellCount = new int[nheight * ncatch](); // row - height; column - catchid
		double *SurfaceArea = new double[nheight * ncatch]();
		double *BedArea = new double[nheight * ncatch]();
		double *Volume = new double[nheight * ncatch]();
		// Per-catchment total area (planimetric) accumulated across all cells (not stage-dependent)
		double *CatchArea = new double[ncatch]();
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
				if (!catchData->isNodata(i, j)) {
					double dxc, dyc, cellArea;
					catchData->getData(i, j, tempcatch);
					handData->getdxdyc(j, dxc, dyc); // This function gets latitude dependent dx and dy for each cell
					cellArea = dxc * dyc;
					auto it = catchhash.find(tempcatch);
					if (it != catchhash.end()) {
						int catchIndex = it->second;
						CatchArea[catchIndex] += cellArea;
						if (!handData->isNodata(i,j) && !slpData->isNodata(i,j)) {  // All 2 inputs have to have data
							handData->getData(i, j, temphand);
							slpData->getData(i, j, tempslp);
							for (k = 0; k < nheight; k++) {
								if (temphand < height[k] || fabs(temphand - 0.0)<0.000001) {  // DGT prefers strictly less than here. If the hand is 0, on stream treat it as wet, too 
									CellCount[k*ncatch + catchIndex] += 1;
									SurfaceArea[k*ncatch + catchIndex] += cellArea;
									BedArea[k*ncatch + catchIndex] += cellArea * sqrt(1 + tempslp*tempslp);
									Volume[k*ncatch + catchIndex] += (height[k] - temphand) * cellArea;
								}
							}
						}
					}
				}
			}
		}

		//MPI output reduce
		int *GCellCount = new int[nheight * ncatch](); // row - height; column - catchid
		double *GSurfaceArea = new double[nheight * ncatch]();
		double *GBedArea = new double[nheight * ncatch]();
		double *GVolume = new double[nheight * ncatch]();
		double *GArea = new double[nheight * ncatch]();
		double *GCrossSectionalArea = new double[nheight * ncatch]();
		double *GWetPerimeter = new double[nheight * ncatch]();
		double *GHydRadius = new double[nheight * ncatch]();
		double *GFlow = new double[nheight * ncatch]();
		// TODO may be able to do Reduce as a vector operation without loops
		MPI_Reduce(CellCount, GCellCount, nheight * ncatch, MPI_INT, MPI_SUM, 0, MCW);
		MPI_Reduce(SurfaceArea, GSurfaceArea, nheight * ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);
		MPI_Reduce(BedArea, GBedArea, nheight * ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);
		MPI_Reduce(Volume, GVolume, nheight * ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);
		// Reduce total catchment area per catchment
		double *GCatchArea = new double[ncatch]();
		MPI_Reduce(CatchArea, GCatchArea, ncatch, MPI_DOUBLE, MPI_SUM, 0, MCW);

		//Write results
		if (rank == 0) {
			// Post-process to calculate derived hydraulic properties
			for (int i = 0; i < ncatch; i++) {
				for (int j = 0; j < nheight; j++) {
					int idx = j * ncatch + i;
					GArea[idx] = GCatchArea[i];
					if (GVolume[idx] > 0) {
						if (linelenlist[i] > 0) {
							GCrossSectionalArea[idx] = GVolume[idx] / linelenlist[i];
							GWetPerimeter[idx] = GBedArea[idx] / linelenlist[i];
						}
						if (GWetPerimeter[idx] > 0) {
							GHydRadius[idx] = GCrossSectionalArea[idx] / GWetPerimeter[idx];
							GFlow[idx] = (GCrossSectionalArea[idx] * pow(GHydRadius[idx], 2.0 / 3.0) * sqrt(slopelist[i])) / manningsnlist[i];
						}
					}
				}
			}

			FILE *fp;
			fp = fopen(hpfile, "w");
			fprintf(fp, "Id, Stage_m, Number of Cells, ReachWetArea_m2, ReachBedArea_m2, ReachVolume_m3, ReachSlope, ReachLength_m, CatchArea_m2, CrossSectionalArea_m2, WetPerimeter_m, HydRadius_m, Manning_n, Flow_m3s\n");
			for (int i = 0; i < ncatch; i++) {
				for (int j = 0; j < nheight; j++) {
					int idx = j * ncatch + i;
					/* Write each column with a separate fprintf to make output formatting clearer and easier to modify. */
					fprintf(fp, "%d,", catchlist[i]);
					fprintf(fp, "%.6lf,", height[j]);
					fprintf(fp, "%d,", GCellCount[idx]);
					fprintf(fp, "%.6lf,", GSurfaceArea[idx]);
					fprintf(fp, "%.6lf,", GBedArea[idx]);
					fprintf(fp, "%.6lf,", GVolume[idx]);
					fprintf(fp, "%.10lf,", slopelist[i]);
					fprintf(fp, "%.6lf,", linelenlist[i]);
					fprintf(fp, "%.6lf,", GArea[idx]);
					fprintf(fp, "%.6lf,", GCrossSectionalArea[idx]);
					fprintf(fp, "%.6lf,", GWetPerimeter[idx]);
					fprintf(fp, "%.6lf,", GHydRadius[idx]);
					fprintf(fp, "%.6lf,", manningsnlist[i]);
					fprintf(fp, "%.6lf\n", GFlow[idx]);
				}
			}
			fclose(fp);
		}
		delete[] CellCount;
		delete[] SurfaceArea;
		delete[] BedArea;
		delete[] Volume;
		delete[] GCatchArea;
		delete[] CatchArea;
		delete[] GCellCount;
		delete[] GSurfaceArea;
		delete[] GBedArea;
		delete[] GVolume;
		delete[] GArea;
		delete[] GCrossSectionalArea;
		delete[] GWetPerimeter;
		delete[] GHydRadius;
		delete[] GFlow;
		free(catchlist);
		free(slopelist);
		free(linelenlist);
		free(manningsnlist);
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
