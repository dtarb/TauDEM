/*  InunMap function takes HAND raster, gets inun depth info
 *  from the COMID mask raster and inundation forecast CSV input,
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


int inunmap(char *handfile, char*catchfile, char *maskfile, char*fcfile, char*hpfile, char *mapfile, char *depthfile)
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

		// get forecast from csv file (fcfile) and build hash table
		long nfc; // num of COMIDs with forecast
        long *catchlist; double *flowlist;
		if (rank == 0) {
			FILE *fp;
			char headers[MAXLN];

			// Open CSV file
			fp = fopen(fcfile, "r");
			if (fp == NULL) {
				fprintf(stderr, "Error: Cannot open forecast file %s\n", fcfile);
				exit(1);
			}

			// Read header line
			if (!fgets(headers, sizeof(headers), fp)) {
				fprintf(stderr, "Error: Empty forecast file %s\n", fcfile);
				fclose(fp);
				exit(1);
			}

			// Count number of data lines
			nfc = 0;
			long pos = ftell(fp);
			char line[256];
			while (fgets(line, sizeof(line), fp)) {
				if (line[0] != '\n' && line[0] != '\0') nfc++;
			}

			if (nfc <= 0) {
				fprintf(stderr, "Error: No data found in forecast file %s\n", fcfile);
				fclose(fp);
				exit(1);
			}

			// Allocate memory
			catchlist = (long *) malloc(sizeof(long) * nfc);
			flowlist = (double *) malloc(sizeof(double) * nfc);

			// Reset file pointer to start of data
			fseek(fp, pos, SEEK_SET);

			// Read data from CSV file (id, flow format)
			for (long i = 0; i < nfc; i++) {
				if (fscanf(fp, "%ld,%lf", &catchlist[i], &flowlist[i]) != 2) {
					fprintf(stderr, "Error: Failed to read data at line %ld in file %s\n", i+2, fcfile);
					fclose(fp);
					exit(1);
				}
			}

			printf("Forecast input: %s\n", fcfile);
			printf("\tnum_COMIDs: %ld\n", nfc);

			fclose(fp);
		}
		MPI_Bcast(&nfc, 1, MPI_LONG, 0, MCW);
		if (rank != 0) {
			catchlist = (long*) malloc(sizeof(long) * nfc);
			flowlist = (double*) malloc(sizeof(double) * nfc);
		}
		MPI_Bcast(catchlist, nfc, MPI_LONG, 0, MCW);
		MPI_Bcast(flowlist, nfc, MPI_DOUBLE, 0, MCW);
		unordered_map<int, float> catchhash;
		// get hydroprop table from CSV file and build comid-areasqkm hash table
		long nhp = 0; // num of COMIDs in the hydroproperty table
		long hpcount = 0; // num of COMIDs that need to be hashed
		long *hydrocatchlist;
		float *surfareas; // TODO: This can be removed if we don't need inunratiohash
		float *areasqkms;
        float *inunratiolist;
		// Hash maps for depth file additional columns
		unordered_map<int, float> catchareahash; // catchment_id -> catchment area
		unordered_map<int, float> inuncatchareahash;   // catchment_id -> inundation area
		
		// compute inundation depth from forecasted flow
		// For each catchment ID and flow from fcfile, interpolate depth from hpfile
		if(rank == 0) {
			FILE *fp;
			char headers[MAXLN];

			// Open hydroprop CSV file
			fp = fopen(hpfile, "r");
			if (fp == NULL) {
				fprintf(stderr, "Error: Cannot open hydroprop file %s\n", hpfile);
				exit(1);
			}

			// Read header line
			if (!fgets(headers, sizeof(headers), fp)) {
				fprintf(stderr, "Error: Empty hydroprop file %s\n", hpfile);
				fclose(fp);
				exit(1);
			}

			// Count number of data lines
			nhp = 0;
			long pos = ftell(fp);
			char line[256];
			while (fgets(line, sizeof(line), fp)) {
				if (line[0] != '\n' && line[0] != '\0') nhp++;
			}

			if (nhp <= 0) {
				fprintf(stderr, "Error: No data found in hydroprop file %s\n", hpfile);
				fclose(fp);
				exit(1);
			}

			// Allocate memory for temporary arrays
			long *catchids = (long *) malloc(sizeof(long) * nhp);
			float *stages = (float *) malloc(sizeof(float) * nhp);
			float *surfareas = (float *) malloc(sizeof(float) * nhp);
			float *areasqkms = (float *) malloc(sizeof(float) * nhp);
			float *flows = (float *) malloc(sizeof(float) * nhp);
			float *interpolated_depths = (float *) malloc(sizeof(float) * nfc);

			// Reset file pointer to start of data
			fseek(fp, pos, SEEK_SET);

			// Read data from CSV file (hpfile) using line-by-line parsing
			// Expected format: Id, Stage_m, Number of Cells, ReachWetArea_m2, ReachBedArea_m2, ReachVolume_m3, ReachSlope, ReachLength_m, CatchArea_m2, CrossSectionalArea_m2, WetPerimeter_m, HydRadius_m, Manning_n, Flow_m3s
			char line2[256];
			for (long hp_idx = 0; hp_idx < nhp; hp_idx++) {
				if (!fgets(line2, sizeof(line2), fp)) {
					fprintf(stderr, "Error: Failed to read line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}

				// Parse the line using strtok
				char *token = strtok(line2, ",");
				if (!token) {
					fprintf(stderr, "Error: Failed to parse ID at line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}
				catchids[hp_idx] = atol(token);

				token = strtok(NULL, ",");
				if (!token) {
					fprintf(stderr, "Error: Failed to parse Stage at line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}
				stages[hp_idx] = atof(token);

				// Skip columns 3 (Number of Cells)
				token = strtok(NULL, ",");
				if (!token) {
					fprintf(stderr, "Error: Failed to parse column 3 at line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}

				// Column 4: ReachWetArea_m2 (surfareas)
				token = strtok(NULL, ",");
				if (!token) {
					fprintf(stderr, "Error: Failed to parse ReachWetArea at line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}
				surfareas[hp_idx] = atof(token);

				// Skip columns 5-8 (ReachBedArea_m2, ReachVolume_m3, ReachSlope, ReachLength_m)
				for (int skip = 0; skip < 4; skip++) {
					token = strtok(NULL, ",");
					if (!token) {
						fprintf(stderr, "Error: Failed to parse column %d at line %ld in file %s\n", 5+skip, hp_idx+2, hpfile);
						fclose(fp);
						exit(1);
					}
				}

				// Column 9: CatchArea_m2 (areasqkms)
				token = strtok(NULL, ",");
				if (!token) {
					fprintf(stderr, "Error: Failed to parse CatchArea at line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}
				areasqkms[hp_idx] = atof(token);

				// Skip columns 10-13 (CrossSectionalArea_m2, WetPerimeter_m, HydRadius_m, Manning_n)
				for (int skip = 0; skip < 4; skip++) {
					token = strtok(NULL, ",");
					if (!token) {
						fprintf(stderr, "Error: Failed to parse column %d at line %ld in file %s\n", 10+skip, hp_idx+2, hpfile);
						fclose(fp);
						exit(1);
					}
				}

				// Column 14: Flow_m3s (flows)
				token = strtok(NULL, ",\n\r");  // Include newline chars in delimiter
				if (!token) {
					fprintf(stderr, "Error: Failed to parse Flow at line %ld in file %s\n", hp_idx+2, hpfile);
					fclose(fp);
					exit(1);
				}
				flows[hp_idx] = atof(token);
			}
			for (long fc_idx = 0; fc_idx < nfc; fc_idx++) {
				long target_id = catchlist[fc_idx];
				double target_flow = flowlist[fc_idx];
				double interpolated_depth = -9999.0; // Default to no data value

				// Find matching records in hydroprop data for this ID
				// We need to find Q1 <= target_flow <= Q2 for the same ID
				double Q1 = -1, Q2 = -1, h1 = -1, h2 = -1;
				bool found_lower = false, found_upper = false;

				// Search through already loaded hydroprop data
				// Since flows are in ascending order for each ID, we can optimize:
				for (long hp_idx = 0; hp_idx < nhp; hp_idx++) {
					if (catchids[hp_idx] == target_id) {
						float hp_flow = flows[hp_idx];
						float hp_stage = stages[hp_idx];

						// Check if this flow value can serve as lower bound
						if (hp_flow <= target_flow) {
							Q1 = hp_flow;
							h1 = hp_stage;
							found_lower = true;
						}
						// Check if this flow value can serve as upper bound
						if (hp_flow >= target_flow) {
							Q2 = hp_flow;
							h2 = hp_stage;
							found_upper = true;
							// Since flows are sorted, once we find upper bound,
							// we either have lower bound or none exists
							break;
						}
					}
				}

				// Perform interpolation if we have valid bounds
				if (found_lower && found_upper && Q2 > Q1) {
					// Linear interpolation: h = (Q-Q1)/(Q2-Q1)*(h2-h1) + h1
					interpolated_depth = (target_flow - Q1) / (Q2 - Q1) * (h2 - h1) + h1;
				} else if (found_lower && Q1 == target_flow) { // TODO: we probaly don't need this type of interpolation
					// Exact match with lower bound
					interpolated_depth = h1;
				} else if (found_upper && Q2 == target_flow) { // TODO: we probaly don't need this type of interpolation
					// Exact match with upper bound
					interpolated_depth = h2;
				}
				interpolated_depths[fc_idx] = interpolated_depth;
			}
			fclose(fp);

			MPI_Bcast(&nhp, 1, MPI_LONG, 0, MCW);
			if (rank != 0) {
				hydrocatchlist = (long*) malloc(sizeof(long) * nhp);
				inunratiolist = (float*) malloc(sizeof(float) * nhp);
				catchids = (long*) malloc(sizeof(long) * nhp);
				stages = (float*) malloc(sizeof(float) * nhp);
				flows = (float*) malloc(sizeof(float) * nhp);
				surfareas = (float*) malloc(sizeof(float) * nhp);
				areasqkms = (float*) malloc(sizeof(float) * nhp);
				interpolated_depths = (float*) malloc(sizeof(float) * nfc);
			}
			MPI_Bcast(catchids, nhp, MPI_LONG, 0, MCW);
			MPI_Bcast(stages, nhp, MPI_FLOAT, 0, MCW);
			MPI_Bcast(flows, nhp, MPI_FLOAT, 0, MCW);
			MPI_Bcast(surfareas, nhp, MPI_FLOAT, 0, MCW);
			MPI_Bcast(areasqkms, nhp, MPI_FLOAT, 0, MCW);
			MPI_Bcast(interpolated_depths, nfc, MPI_FLOAT, 0, MCW);
			for (int i=0; i<nfc; i++) catchhash[(int)catchlist[i]] = interpolated_depths[i];
			// Allocate arrays for comid and inunratio
			hydrocatchlist = (long *) malloc(sizeof(long) * nhp);
			inunratiolist = (float *) malloc(sizeof(float) * nhp);
			
			// Process the data to calculate inundation ratios and populate hash maps
			for (int idx = 0; idx < nhp; idx++) {
				int catchment_id = (int)catchids[idx];

				// Populate hash maps for depth file generation
				catchareahash[catchment_id] = areasqkms[idx];

				if (areasqkms[idx] > 0) {
					float inunratio = surfareas[idx] / areasqkms[idx];
					hydrocatchlist[hpcount] = catchids[idx];
					inunratiolist[hpcount] = inunratio;
					hpcount++;
				}
			}
			// Clean up arrays we don't need anymore
			if (catchids) free(catchids);
			if (stages) free(stages);
			if (flows) free(flows);
			if (surfareas) free(surfareas);
			if (areasqkms) free(areasqkms);
			if (interpolated_depths) free(interpolated_depths);

			// Reallocate arrays to actual size needed
			hydrocatchlist = (long *) realloc(hydrocatchlist, sizeof(long) * hpcount);
			inunratiolist = (float *) realloc(inunratiolist, sizeof(float) * hpcount);

			printf("Hydroprop input: %s\n", hpfile);
			printf("\tnum_COMIDs processed: %ld\n", hpcount);

			// compute cell area for each catchment using catchData and handData
			float temphand = 0.0;
			int32_t tempcatch = 0;
			for (long j = 0; j < ny; j++) {
				for (long i = 0; i < nx; i++) {
					if (!catchData->isNodata(i, j)) {
						catchData->getData(i, j, tempcatch);
						auto it = catchhash.find(tempcatch);
						if (it != catchhash.end()) {
							float inunDepth = it->second;
							double dxc, dyc, cellArea;
							handData->getdxdyc(j, dxc, dyc); // This function gets latitude dependent dx and dy for each cell
							cellArea = dxc * dyc;
							if(!handData->isNodata(i,j)) {
								handData->getData(i, j, temphand);
								if ((inunDepth - temphand) > 0.0) {
									inuncatchareahash[tempcatch] += cellArea;
								}
							}
						}
					}
				}
			}
		}

		// TODO: The following 2 lines can be removed if we don't need inunratiohash
		long *catchids = NULL; float *stages = NULL; float *flows = NULL;
		unordered_map<int, float> inunratiohash;

		// Read hydroprop file if mask file is provided OR if depth file is requested
		// TODO: This whole if block can be removed if we don't need inunratiohash
		if (maskfile != NULL || depthfile != NULL) {
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
		
		//generate depth file
		if (depthfile != NULL && rank == 0) {
			FILE *depthfp = fopen(depthfile, "w");
			if (depthfp == NULL) {
				fprintf(stderr, "Error: Cannot create depth file %s\n", depthfile);
			}
			else {
				// Write header
				fprintf(depthfp, "id,flow,depth,InunArea_m2,CatchArea_m2,InunRatio\n");
				float nodata = -9999.0;
				for (long fc_idx = 0; fc_idx < nfc; fc_idx++) {
					long target_id = catchlist[fc_idx];
					double target_flow = flowlist[fc_idx];
					double interpolated_depth = catchhash[target_id];

					// Get additional data from hash maps
					float inun_catch_area = inuncatchareahash.count(target_id) ? inuncatchareahash[target_id] : nodata;
					float catch_area = catchareahash.count(target_id) ? catchareahash[target_id] : nodata;
					float inun_ratio = nodata;
					if (inun_catch_area != nodata && catch_area != nodata)
						inun_ratio = inun_catch_area / catch_area;

					fprintf(depthfp, "%ld,%.6f,%.6f,%.6f,%.6f,%.6f\n",
						target_id, target_flow, interpolated_depth, inun_catch_area, catch_area, inun_ratio);
				}
				fclose(depthfp);
				printf("Depth file written: %s\n", depthfile);
			}
		}

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
					// hfc (height of forecast) is the interpolated depth/height based on flow
					hfc =  catchhash[comid];
					if (hfc < 0.0) continue;
					// TODO: we can use catchhash instead of inunratiohash here - that means we don't need to create the inunratiohash
					if (maskfile != NULL && inunratiohash.count(comid) > 0) continue; // ignore catchments with small pit-removed waterbodies. they are in the hash if inunratio > 10%
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
		free(flowlist);
		if (maskfile != NULL) {
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
			printf("Inundation depth Compute time: %f\n", compute);

			//Brackets force MPI-dependent objects to go out of scope before Finalize is called
		}MPI_Finalize();

		return 0;
	}
