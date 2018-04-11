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

int inunmap(char *handfile, char*catchfile, char *maskfile, char*fcfile, int maskpits, char*hpfile, char *mapfile)
{
	MPI_Init(NULL, NULL); {

		int rank, size;
		MPI_Comm_rank(MCW, &rank);
		MPI_Comm_size(MCW, &size);
		if (rank == 0)printf("InunMap version %s\n", TDVERSION);

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

		//Read mask raster (waterbody)
		tdpartition *maskData = NULL;
		if (maskfile != NULL) {
			tiffIO mask(maskfile, SHORT_TYPE);
			if (!hand.compareTiff(mask)) return 1;  //And maybe an unhappy error message
			maskData = CreateNewPartition(mask.getDatatype(), totalX, totalY, dxA, dyA, mask.getNodata());
			mask.read(xstart, ystart, maskData->getny(), maskData->getnx(), maskData->getGridPointer());
		}

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
		MPI_Bcast(catchlist, nfc, MPI_LONG, 0, MCW);
		MPI_Bcast(hlist, nfc, MPI_DOUBLE, 0, MCW);
		unordered_map<int, double> catchhash;
		for (int i=0; i<nfc; i++) catchhash[(int)catchlist[i]] = hlist[i];

		// get hydroprop table from netcdf and build comid-areasqkm hash table
		long nhp; // num of COMIDs in the hydroproperty table
		long hpcount = 0; // num of COMIDs that need to be hashed
        long *hydrocatchlist; float *inunratiolist;
		unordered_map<int, float> inunratiohash;
		if (maskpits) {
			if (rank == 0) {
				int ncid, varidCatch, varidH, varidA, varidS;
				if (nc_open(hpfile, NC_NOWRITE, &ncid)) {
					fprintf(stderr, "hpNetCDF: error open file %s\n", hpfile); exit(1);
				}
				size_t n;
				if (nc_inq_dimlen(ncid, 0, &n)) {
					fprintf(stderr, "hpNetCDF: error get dim size \n"); exit(1);
				}
				if (nc_inq_varid(ncid, "CatchId", &varidCatch) || nc_inq_varid(ncid, "Stage", &varidH) || nc_inq_varid(ncid, "SurfaceArea", &varidS) || nc_inq_varid(ncid, "AREASQKM", &varidA)) {
					fprintf(stderr, "hpNetCDF: error get varid for comid and H\n"); exit(1);
				}
				long nrows = (long) n;
				int nstages = 83; //TODO: get the val from netcdf itself
				nhp = nrows / nstages; // must be divisible by nstages
				// read variables into memory: only stage=1ft
				hydrocatchlist = (long *) malloc(sizeof(long) * nhp); // output
				inunratiolist = (float*) malloc(sizeof(float) * nhp); // output
				long *hcatchlist = (long *) malloc(sizeof(long) * nhp); 
				double *surfacearealist = (double*) malloc(sizeof(double) * nhp);
				double *areasqkmlist = (double*) malloc(sizeof(double) * nhp);
				size_t nhp2 = nhp;
				size_t index = 1; // 1ft is the 2nd stage on stage table. TODO: get it from a config
				ptrdiff_t interval = 83; //TODO: get it from a config
				if (nc_get_vars_long(ncid, varidCatch, &index, &nhp2, &interval, hcatchlist)) {
					fprintf(stderr, "hpNetCDF: error read comid list\n"); exit(1);
				}
				if (nc_get_vars_double(ncid, varidS, &index, &nhp2, &interval, surfacearealist)) {
					fprintf(stderr, "hpNetCDF: error read surfacearea list\n"); exit(1);
				}
				if (nc_get_vars_double(ncid, varidA, &index, &nhp2, &interval, areasqkmlist)) {
					fprintf(stderr, "hpNetCDF: error read areasqkm list\n"); exit(1);
				}

				for (int i=0; i<nhp; i++) {
					float inunratio = (float)(surfacearealist[i] / (areasqkmlist[i] * 1000000.0));
					if (inunratio < 0.1) continue; // if ratio > 10%, we should check if to show it or not
					hydrocatchlist[hpcount] = hcatchlist[i];	
					inunratiolist[hpcount] = inunratio;	
					hpcount ++;
				}
				int ll = (nhp>5)?5:nhp;
				printf("Check: input hydrotable\n");
				printf("COMID:\t");for (int i=0; i<ll; i++) printf("%ld\t", hcatchlist[i]);printf("\n");
				printf("S:\t");for (int i=0; i<ll; i++) printf("%.5f\t", surfacearealist[i]);printf("\n");
				printf("A:\t");for (int i=0; i<ll; i++) printf("%.5f\t", areasqkmlist[i]);printf("\n");
				free(hcatchlist); free(surfacearealist); free(areasqkmlist);
/* DEPRECATED: the following reads nc row by row, very slow. 
				long comid; double areqsqkm, surfacearea, h;
			
				if (nc_inq_varid(ncid, "CatchId", &varidCatch) || nc_inq_varid(ncid, "Stage", &varidH) || nc_inq_varid(ncid, "SurfaceArea", &varidS) || nc_inq_varid(ncid, "AREASQKM", &varidA)) {
					fprintf(stderr, "hpNetCDF: error get varid for comid and H\n"); exit(1);
				}
				hydrocatchlist = (long *) malloc(sizeof(long) * nhp);
				memset(hydrocatchlist, 0, nhp *sizeof(long));
				inunratiolist = (float*) malloc(sizeof(float) * nhp);
				memset(inunratiolist, 0, nhp *sizeof(float));
				for (size_t i=0; i<nrows; i++) { // scan hydroprop table to find areasqkm at Stage=1ft
					if (nc_get_var1_double(ncid, varidH, &i, &h)) {
						fprintf(stderr, "hpNetCDF: error read Stage\n"); exit(1);
					}
					if (h<0.3 || h>0.4) continue; //TODO: the interval needs to be computed
					// stage=1ft row
					if (nc_get_var1_double(ncid, varidS, &i, &surfacearea)) {
						fprintf(stderr, "hpNetCDF: error read SurfaceArea\n"); exit(1);
					}
					if (nc_get_var1_double(ncid, varidA, &i, &areqsqkm)) {
						fprintf(stderr, "hpNetCDF: error read AREASQKM\n"); exit(1);
					}
					float inunratio = (float)(surfacearea / (areqsqkm * 1000000.0));
					if (inunratio < 0.1) continue; // if ratio > 10%, we should check if to show it or not
					if (nc_get_var1_long(ncid, varidCatch, &i, &comid)) {
						fprintf(stderr, "hpNetCDF: error read comid\n"); exit(1);
					}
					hydrocatchlist[hpcount] = comid;
					inunratiolist[hpcount] = inunratio;
					hpcount ++;
				}
*/
				printf("Hydroproperty input: %s\n", hpfile);
				printf("\tnum_COMIDs: %ld\n", nhp);
				printf("\tnum_COMIDs to mask: %ld\n", hpcount);
				int l = (hpcount>5)?5:hpcount;
				printf("COMID:\t");for (int i=0; i<l; i++) printf("%ld ", hydrocatchlist[i]);printf("\n");
				printf("RATIO:\t");for (int i=0; i<l; i++) printf("%.5f ", inunratiolist[i]);printf("\n");
				nc_close(ncid);
			}
			MPI_Bcast(&hpcount, 1, MPI_LONG, 0, MCW);
			if (rank != 0) {
				hydrocatchlist = (long*) malloc(sizeof(long) * hpcount);
				inunratiolist = (float*) malloc(sizeof(float) * hpcount);
			}
			MPI_Bcast(hydrocatchlist, hpcount, MPI_LONG, 0, MCW);
			MPI_Bcast(inunratiolist, hpcount, MPI_FLOAT, 0, MCW);
			for (int i=0; i<hpcount; i++) { // build comid-inunratio hash
				int comid = (int)(hydrocatchlist[i]);
				if (catchhash.count(comid) > 0) // comid also exists in forecast
					inunratiohash[comid] = inunratiolist[i];
			}
		}

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
				if (!catchData->isNodata(i, j) && !handData->isNodata(i,j) && (!maskData || (maskData && maskData->isNodata(i,j)))) {  // All 2 input rasters have to have data, and not masked (nodata is 0)
					int32_t comid = 0;
					double hfc = 0.0;
					catchData->getData(i, j, comid);
					hfc =  catchhash[comid];
					if (hfc < 0.0) continue;
					if (maskpits && inunratiohash.count(comid) > 0) continue; // ignore catchments with small pit-removed waterbodies. they are in the hash if inunratio > 10%
					float handv = 0.0;
					handData->getData(i, j, handv);
					if (hfc > handv + 0.001) { // need to have diff>1mm
						inunp->setData(i,j, (float)(hfc - (double)handv));
					}// else {
						//inunp->setToNodata(i,j);
					//}
				}
			}
		}
		free(catchlist);
		free(hlist);
		if (maskpits) {
			free(hydrocatchlist);
			free(inunratiolist);
		}

		//Create and write TIFF file
		tiffIO inunmapraster(mapfile, FLOAT_TYPE, felNodata, hand);
		inunmapraster.write(xstart, ystart, ny, nx, inunp->getGridPointer());

		//Stop timer
		end = MPI_Wtime();
		double compute, temp;
		compute = end - begin;

		MPI_Allreduce(&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
		compute = temp / size;


		if (rank == 0)
			printf("InunMap Compute time: %f\n", compute);

			//Brackets force MPI-dependent objects to go out of scope before Finalize is called
		}MPI_Finalize();

		return 0;
	}
