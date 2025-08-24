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
#include <gdal.h>
#include <gdal_priv.h>

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
			GDALAllRegister();
            GDALDataset *poDS = (GDALDataset *) GDALOpen(fcfile, GA_ReadOnly);
            if (poDS == NULL) {
                fprintf(stderr, "GDAL: error opening file %s\n", fcfile);
                exit(1);
            }

            // Read COMID and H variables
			// DGT 8/24/25 Modify here to read text file that has Id, flow, n 
            GDALRasterBand *poBandCOMID = poDS->GetRasterBand(1); // Adjust band number as needed
            GDALRasterBand *poBandH = poDS->GetRasterBand(2);     // Adjust band number as needed

			// DGT 8/24/25 nfc to be number of records in flowtab.csv
            nfc = poBandCOMID->GetXSize(); // Assuming 1D array
            catchlist = (long *) malloc(sizeof(long) * nfc);
            hlist = (double *) malloc(sizeof(double) * nfc);

            // Read data
            if (poBandCOMID->RasterIO(GF_Read, 0, 0, nfc, 1, catchlist, nfc, 1, GDT_Int32, 0, 0) != CE_None ||
                poBandH->RasterIO(GF_Read, 0, 0, nfc, 1, hlist, nfc, 1, GDT_Float64, 0, 0) != CE_None) {
                fprintf(stderr, "GDAL: error reading data\n");
                exit(1);
            }

            // Get timestamp from metadata
            char **metadata = poDS->GetMetadata();
            printf("Forecast input: %s\n", fcfile);
            printf("\tnum_COMIDs: %ld\n", nfc);
            if (metadata != NULL) {
                printf("\tTimestamp: %s\n", CSLFetchNameValue(metadata, "TIMESTAMP"));
            }

            GDALClose(poDS);
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
				// DGT 8/24/25 Modify here to read hydroprop text file 
				
				GDALDataset *poDS = (GDALDataset *) GDALOpen(hpfile, GA_ReadOnly);
				if (poDS == NULL) {
					fprintf(stderr, "GDAL: error opening hydroprop file %s\n", hpfile);
					exit(1);
				}

				// Read variables using GDAL bands
				// Adjust band numbers based on your NetCDF structure
				GDALRasterBand *poBandCatchId = poDS->GetRasterBand(1);
				GDALRasterBand *poBandStage = poDS->GetRasterBand(2);
				GDALRasterBand *poBandSurfArea = poDS->GetRasterBand(3);
				GDALRasterBand *poBandAreaSqKm = poDS->GetRasterBand(4);

				// Get the number of records
				nhp = poBandCatchId->GetXSize(); // Assuming 1D array
				
				// Allocate memory for temporary arrays
				long *catchids = (long *) malloc(sizeof(long) * nhp);
				float *stages = (float *) malloc(sizeof(float) * nhp);
				float *surfareas = (float *) malloc(sizeof(float) * nhp);
				float *areasqkms = (float *) malloc(sizeof(float) * nhp);

				// Read data from bands
				if (poBandCatchId->RasterIO(GF_Read, 0, 0, nhp, 1, catchids, nhp, 1, GDT_Int32, 0, 0) != CE_None ||
                    poBandStage->RasterIO(GF_Read, 0, 0, nhp, 1, stages, nhp, 1, GDT_Float32, 0, 0) != CE_None ||
                    poBandSurfArea->RasterIO(GF_Read, 0, 0, nhp, 1, surfareas, nhp, 1, GDT_Float32, 0, 0) != CE_None ||
                    poBandAreaSqKm->RasterIO(GF_Read, 0, 0, nhp, 1, areasqkms, nhp, 1, GDT_Float32, 0, 0) != CE_None) {
                    fprintf(stderr, "GDAL: error reading hydroprop data\n");
                    exit(1);
                }
				// Allocate arrays for comid and inunratio
				hydrocatchlist = (long *) malloc(sizeof(long) * nhp);
				inunratiolist = (float *) malloc(sizeof(float) * nhp);

                // Process the data to calculate inundation ratios
                for (int idx = 0; idx < nhp; idx++) {
                    if (areasqkms[idx] > 0) {
                        float inunratio = surfareas[idx] / areasqkms[idx];
                        if (inunratio > 0.1) { // 10% threshold
                            hydrocatchlist[hpcount] = catchids[idx];
                            inunratiolist[hpcount] = inunratio;
                            hpcount++;
                        }
                    }
                }

                // Reallocate arrays to actual size needed
                hydrocatchlist = (long *) realloc(hydrocatchlist, sizeof(long) * hpcount);
                inunratiolist = (float *) realloc(inunratiolist, sizeof(float) * hpcount);

                // Clean up temporary arrays
                free(catchids);
                free(stages);
                free(surfareas);
                free(areasqkms);

                printf("Hydroprop input: %s\n", hpfile);
                printf("\tnum_COMIDs processed: %ld\n", hpcount);

                GDALClose(poDS);
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
