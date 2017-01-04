/*  InunMap function takes HAND raster, gets inun depth info
 *  from the COMID mask raster and inundation forecast netcdf4 input,
 *  then creates the inundation map raster.

  Yan Liu, David Tarboton, Xing Zheng
  NCSA/UIUC, Utah State University, University of Texas at Austin
  Jan 03, 2017 
  
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
#include <netcdf.h> // using C API; C++ API is not as updaed

int inunmap(char *handfile, char*catchfile, char*fcfile, char *mapfile)
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

		// get forecast from netcdf and build hash table
		long nfc; // num of COMIDs with forecast
        long *catchlist; double *hlist;
		if (rank == 0) {
			int ncid, varidCatch, varidH;
			if (nc_open(fcfile, NC_NOWRITE, &ncid)) {
				fprintf(stderr, "NetCDF: error open file %s\n", fcfile); exit(1);
			}
			size_t n;
			if (nc_inq_dimlen(ncid, 0, &n)) {
				fprintf(stderr, "NetCDF: error get dim size \n"); exit(1);
			}
			nfc = (long) n;
			char t[512];
			if (nc_get_att_text(ncid, NC_GLOBAL, "Timestamp", t)) {
				fprintf(stderr, "NetCDF: error get timestamp attr \n"); exit(1);
			}

			catchlist = (long *) malloc(sizeof(long) * nfc);
			hlist = (double *) malloc(sizeof(double) * nfc);
			
			if (nc_inq_varid(ncid, "COMID", &varidCatch) || nc_inq_varid(ncid, "H", &varidH)) {
				fprintf(stderr, "NetCDF: error get varid for comid and H\n"); exit(1);
			}
			if (nc_get_var_long(ncid, varidCatch, catchlist)) {
				fprintf(stderr, "NetCDF: error read comid list\n"); exit(1);
			}
			if (nc_get_var_double(ncid, varidH, hlist)) {
				fprintf(stderr, "NetCDF: error read h list\n"); exit(1);
			}
			printf("Forecast input: %s\n", fcfile);
			printf("\tnum_COMIDs: %ld\n", nfc);
			printf("\tTimestamp: %s\n", t);
			int l = (nfc>5)?5:nfc;
			printf("COMID:\t");for (int i=0; i<l; i++) printf("%ld ", catchlist[i]);printf("\n");
			printf("H:\t");for (int i=0; i<l; i++) printf("%lf ", hlist[i]);printf("\n");
			nc_close(ncid);
		}
		MPI_Bcast(&nfc, 1, MPI_LONG, 0, MCW);
		if (rank != 0) {
			catchlist = (long*) malloc(sizeof(long) * nfc);
			hlist = (double*) malloc(sizeof(double) * nfc);
		}
		MPI_Bcast(catchlist, nfc, MPI_INT, 0, MCW);
		MPI_Bcast(hlist, nfc, MPI_DOUBLE, 0, MCW);
		unordered_map<int, double> catchhash;
		for (int i=0; i<nfc; i++) catchhash[(int)catchlist[i]] = hlist[i];

		long i, j, k;

		//Share information and set borders to zero
		//handData->share();
		//catchData->share();

		// create inun map partition
		tdpartition *inunp;
		float felNodata = -3.0e38;
		inunp = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, felNodata);

		// Compute inundation map cell values
		for (j = 0; j < ny; j++) {
			for (i = 0; i < nx; i++) {
				if (!catchData->isNodata(i, j) && !handData->isNodata(i,j)) {  // All 2 input rasters have to have data
					int32_t comid = 0;
					double hfc = 0.0;
					catchData->getData(i, j, comid);
					hfc =  catchhash[comid];
					if (hfc < 0.0) continue;
					float handv = 0.0;
					handData->getData(i, j, handv);
					if (hfc >= handv) {
						inunp->setData(i,j, (float)(hfc - (double)handv));
					}// else {
						//inunp->setToNodata(i,j);
					//}
				}
			}
		}
		free(catchlist);
		free(hlist);

		//Create and write TIFF file
		tiffIO inunmapraster(mapfile, FLOAT_TYPE, &felNodata, hand);
		inunmapraster.write(xstart, ystart, ny, nx, inunp->getGridPointer());

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
