/*  MoveOutletsToStrm function to move outlets to a stream.
     
  David Tarboton, Teklu Tesfa, Dan Watson
  Utah State University  
  May 23, 2010 
  

  This function moves outlet point that are off a stream raster grid down D8 flow directions 
  until a stream raster grid is encountered.  Input is a flow direction grid, stream raster grid 
  and outlets shapefile.  Output is a new outlets shapefile where each point has been moved to 
  coincide with the stream raster grid if possible.  A field 'dist_moved' is added to the new 
  outlets shapefile to indicate the changes made to each point.  Points that are already on the 
  stream raster (src) grid are not moved and their 'dist_moved' field is assigned a value 0.  
  Points that are initially not on the stream raster grid are moved by sliding them along D8 
  flow directions until one of the following occurs:
  a.	A stream raster grid cell is encountered before traversing the max_dist number of grid cells.  
   The point is moved and 'dist_moved' field is assigned a value indicating how many grid cells the 
   point was moved.
  b.	More thanthe max_number of grid cells are traversed, or the traversal ends up going out of 
  the domain (encountering a no data D8 flow direction value).  The point is not moved and the 
  'dist_moved' field is assigned a value of -1.

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

// 1/25/14.  Modified to use shapelib by Chris George

#include <mpi.h>
#include <math.h>
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include "ogr_api.h"
//#include "shapelib/shapefil.h"
#include "MoveOutletsToStrm.h"
#
using namespace std;
OGRSFDriverH    driver;
OGRDataSourceH  hDSsh, hDSshmoved;
OGRLayerH       hLayersh, hLayershmoved;
OGRFeatureDefnH hFDefnsh,hFDefnshmoved;
OGRFieldDefnH   hFieldDefnsh,hFieldDefnshmoved,hFieldDefn;
OGRFeatureH     hFeaturesh,hFeatureshmoved;
OGRGeometryH    hGeometrysh, hGeometryshmoved;


int outletstosrc(char *pfile, char *srcfile, char *outletsdatasrc, char *outletslayer,int uselyrname,int lyrno, char *outletmoveddatasrc,char *outletmovedlayer, int maxdist)
{

	MPI_Init(NULL, NULL); {
		int rank, size;
		MPI_Comm_rank(MCW, &rank);
		MPI_Comm_size(MCW, &size);
		if (rank == 0) {
			printf("MoveOutletsToStreams version %s\n", TDVERSION);
			fflush(stdout);
		}

		double begin, end;
		//Begin timer
		begin = MPI_Wtime();
		int d1[9] = { -99,0,-1,-1,-1,0,1,1,1 };
		int d2[9] = { -99,1,1,0,-1,-1,-1,0,1 };

		//load the stream raster grid into a linear partition
		//Create tiff object, read and store header info
		//	MPI_Abort(MCW,5);
		tiffIO src(srcfile, SHORT_TYPE);
		long srcTotalX = src.getTotalX();
		long srcTotalY = src.getTotalY();
		double srcdx = src.getdxA();
		double srcdy = src.getdyA();
		OGRSpatialReferenceH hSRSRaster;
		hSRSRaster = src.getspatialref();
		if (rank == 0)
		{
			float timeestimate = (2e-7*srcTotalX*srcTotalY / pow((double)size, 0.65)) / 60 + 1;  // Time estimate in minutes
			fprintf(stderr, "This run may take on the order of %.0f minutes to complete.\n", timeestimate);
			fprintf(stderr, "This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


		//Create partition and read data
		tdpartition *srcData;
		srcData = CreateNewPartition(src.getDatatype(), srcTotalX, srcTotalY, srcdx, srcdy, src.getNodata());
		int srcnx = srcData->getnx();
		int srcny = srcData->getny();
		int srcxstart, srcystart;  // DGT Why are these declared as int if they are to be used as long
		srcData->localToGlobal(0, 0, srcxstart, srcystart);  //  DGT here no typecast - but 2 lines down there is typecast - why

		src.read((long)srcxstart, (long)srcystart, (long)srcny, (long)srcnx, srcData->getGridPointer());

		//load the d8 flow grid into a linear partition
		//Create tiff object, read and store header info
		tiffIO p(pfile, SHORT_TYPE);
		long pTotalX = p.getTotalX();
		long pTotalY = p.getTotalY();
		double pdx = p.getdxA();
		double pdy = p.getdyA();

		//Create partition and read data
		tdpartition *flowData;
		flowData = CreateNewPartition(p.getDatatype(), pTotalX, pTotalY, pdx, pdy, p.getNodata());
		int pnx = flowData->getnx();
		int pny = flowData->getny();
		int pxstart, pystart;

		//char fnamedebug[100];
		//sprintf(fnamedebug, "temp%d.txt", rank);
		//FILE *fdebug;
		//fdebug = fopen(fnamedebug, "w");

		//fprintf(fdebug, "Processor: %d, pnx: %d, pny: %d, pxstart: %d, pystart: %d\n", rank, pnx, pny, pxstart, pystart);
		//fflush(fdebug);

		flowData->localToGlobal(0, 0, pxstart, pystart);
		p.read(pxstart, pystart, pny, pnx, flowData->getGridPointer());

		if (!p.compareTiff(src)) {
			printf("src and p files not the same size. Exiting \n");
			fflush(stdout);
			MPI_Abort(MCW, 4);
		}
		//


	   //load the shapefile that contains the unmoved src points
	   //copy the shapefile to a new shapefile called shpmoved on p0

	   //  Code added to read shape file
	   //SHPHandle sh, shmoved;
	   //DBFHandle dbf, dbfmoved;
		double *xnode, *ynode;
		double *origxnode, *origynode;
		int nxy;
		long *dist_moved;  //*ismoved,  DGT decided ismoved is not needed
		int *is_done;  // variable to keep track of whether this outlet is complete
		int nfields;
		int i, j;
		int * indexMap;
		//DBFFieldType * types;
		int dmIndex;

		if (rank == 0) {

			OGRRegisterAll();
			//read exsitng outlet datasource

			hDSsh = OGROpen(outletsdatasrc, FALSE, NULL);
			if (hDSsh == NULL)
			{
				printf("Error Opening datasource %s.\n", outletsdatasrc);
				exit(1);
			}


			if (hDSsh != NULL) {


				//get layer from layer name
				if (uselyrname == 1) { hLayersh = OGR_DS_GetLayerByName(hDSsh, outletslayer); }
				//get layerinfo from layer number
				else { hLayersh = OGR_DS_GetLayer(hDSsh, lyrno); } // get layerinfo from layername

				if (hLayersh == NULL)getlayerfail(hDSsh, outletsdatasrc, lyrno);
				OGRwkbGeometryType gtype;
				gtype = OGR_L_GetGeomType(hLayersh);
				if (gtype != wkbPoint)getlayerfail(hDSsh, outletsdatasrc, lyrno);
				// char *layernamesh; 
			   //  layernamesh=getLayername(outletshapefile); // get layer name which is file name without extension

				 //hLayersh = OGR_DS_GetLayerByName( hDSsh,outletslayer );




				hFeaturesh = OGR_L_GetFeature(hLayersh, 0);// read  feature of the shapefile
				nxy = OGR_L_GetFeatureCount(hLayersh, 1);//  

			   //creating new moved outlet shapefile
				const char *pszDriverName;
				pszDriverName = getOGRdrivername(outletmoveddatasrc);


				driver = OGRGetDriverByName(pszDriverName);
				if (driver == NULL)
				{
					printf("%s warning:driver not available.\n", pszDriverName);
					//exit( 1 );
				}


				// Create new file using this driver if the datasoruce exists 
				if (pszDriverName == "SQLite")hDSshmoved = OGROpen(outletmoveddatasrc, TRUE, NULL);
				// create new data source if data source does not exist 
				if (hDSshmoved == NULL) {
					hDSshmoved = OGR_Dr_CreateDataSource(driver, outletmoveddatasrc, NULL);
				}
				else { hDSshmoved = hDSshmoved; }

				if (hDSshmoved != NULL) {

					//hLayershmoved = OGR_DS_CreateLayer( hDSshmoved, outletmovedlayer, hSRSRaster, wkbPoint, NULL ); // create layer for moved outlet, where raster layer spatial reference is used fro shapefile
					if (strlen(outletmovedlayer) == 0) {
					// Chris George
						char layernameshmoved[MAXLN];
						getLayername(outletmoveddatasrc,layernameshmoved); // get layer name which is file name without extension
						//char * layernameshmoved;
						//layernameshmoved = getLayername(outletmoveddatasrc); // get layer name which is file name without extension
						hLayershmoved = OGR_DS_CreateLayer(hDSshmoved, layernameshmoved, hSRSRaster, wkbPoint, NULL);
					}

					else {
						hLayershmoved = OGR_DS_CreateLayer(hDSshmoved, outletmovedlayer, hSRSRaster, wkbPoint, NULL);
					}// provide same spatial reference as raster in streamnetshp file

					if (hLayershmoved == NULL)
					{
						printf("warning: Layer creation failed.\n");
						//exit( 1 );
					}

					//OGR_L_ResetReading(hLayersh);
					hFDefnsh = OGR_L_GetLayerDefn(hLayersh); //get schema for the outlet shapefile
					hFDefnshmoved = OGR_L_GetLayerDefn(hLayershmoved);
					int iField;
					// read field from outlet shapefile and create that field to moved outlet shapefile
					for (iField = 0; iField < OGR_FD_GetFieldCount(hFDefnsh); iField++) {
						hFieldDefnsh = OGR_FD_GetFieldDefn(hFDefnsh, iField);
						OGR_L_CreateField(hLayershmoved, hFieldDefnsh, 0);
					}
					// add new field to moved outlet shapefile
					hFieldDefnshmoved = OGR_Fld_Create("Dist_moved", OFTInteger);
					OGR_Fld_SetWidth(hFieldDefnshmoved, 6); // set field width
					OGR_L_CreateField(hLayershmoved, hFieldDefnshmoved, 0);
					// nxy=OGR_L_GetFeatureCount(hLayersh,1);//  
				}


			}
			else {
				printf("\nError opening shapefile.\n\n");
				nxy = 0;
				//		MPI_Abort(MCW,5);
			}
		}

		MPI_Bcast(&nxy, 1, MPI_INT, 0, MCW);
		if (nxy == 0)
		{  // Attempt to exit gracefully on all processors
			if (rank == 0)
				printf("Unable to read any points from shapefile\n\n");

			MPI_Finalize();
		}

		xnode = new double[nxy];
		ynode = new double[nxy];
		origxnode = new double[nxy];
		origynode = new double[nxy];
		//	ismoved = new long[nxy];  // DGT decided not needed 
		dist_moved = new long[nxy];
		is_done = new int[nxy];  // Variable to keep track of whether this outlet is complete
		int itresh = 1;  // Thresholding to 1 done in source

		if (rank == 0) {
			int i = 0;
			OGR_L_ResetReading(hLayersh);
			while ((hFeaturesh = OGR_L_GetNextFeature(hLayersh)) != NULL)
			{
				//hFeaturesh=OGR_L_GetFeature(hLayersh,i);// does not work for geojson or sqlite file
			   // hFeaturesh = OGR_L_GetNextFeature(hLayersh);
				hGeometrysh = OGR_F_GetGeometryRef(hFeaturesh);
				xnode[i] = OGR_G_GetX(hGeometrysh, 0); // get x coordinate for the outlet shapefile
				ynode[i] = OGR_G_GetY(hGeometrysh, 0); // get y coordiante for the outelet shapefile
				origxnode[i] = xnode[i];
				origynode[i] = ynode[i];
				//  Initializing
				//			ismoved[i] = 0;
				dist_moved[i] = 0;
				is_done[i] = 1; // use 1 to indicate not done, 0 to indicate done.  Sum of all = 0 will then indicate all done
				//SHPDestroyObject(shp);
				//OGR_F_Destroy( hFeaturesh );
				i = i + 1;
				//OGR_F_Destroy( hFeaturesh );
			}
			// destrpy shapefile feature after reading	
		//OGR_F_Destroy( hDSsh );

		}

		MPI_Bcast(xnode, nxy, MPI_DOUBLE, 0, MCW);
		MPI_Bcast(ynode, nxy, MPI_DOUBLE, 0, MCW);
		//	MPI_Bcast(ismoved, nxy, MPI_LONG, 0, MCW);
		MPI_Bcast(dist_moved, nxy, MPI_LONG, 0, MCW);
		MPI_Bcast(is_done, nxy, MPI_INT, 0, MCW);
		/*printf("Processor: %d, nxy: %d, xnode: %f, ynode: %f, distmoved: %d, part_has: %d\n", rank, nxy, xnode[0], ynode[0], dist_moved[0], part_has[0]);
		fflush(stdout);*/

		// oooooooooooooooooooooo  begin processing

		// Variables invoved here are
		// Global dimensions
		//	pTotalX, pTotalY

		//	Partition dimensions
		//	pnx, pny

		//	Outlet variables
		//	nyx, OutletX[], OutletY[], is_done[], dist_moved[]

		//	dist_moved = -1 means could not move so retain original


		int *outletsX, *outletsY;
		int tx, ty;
		short td;
		outletsX = new int[nxy];
		outletsY = new int[nxy];
		int done = 0;
		//int localdone = 0;
		//int localnodes;
		//int totalnodes;
		//int totaldone;
		short dirn;
		int nextx, nexty;

		//Convert geo coords to grid coords and initialize done
		done = 0;
		for (i = 0; i < nxy; i++) {
			p.geoToGlobalXY(xnode[i], ynode[i], outletsX[i], outletsY[i]);
			// Check if in domain
			if (outletsX[i] < 0 || outletsX[i] >= pTotalX || outletsY[i] < 0 || outletsY[i] >= pTotalY) {  // Here out of domain
				is_done[i] = 0;
				dist_moved[i] = -1;  // Cannot be moved
			}			
			done = done + is_done[i];
		}

		//printf("Processor: %d, nxy: %d, xnode: %f, ynode: %f, outletsX: %d, outletsY: %d\n", rank, nxy, xnode[0], ynode[0], outletsX[0], outletsY[0]);
		//fflush(stdout);

		while (done > 0) {
			done = 0;  // reinitialize check on parse through
			for (i = 0; i < nxy; i++) {
				flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
				while (flowData->isInPartition(tx, ty) && is_done[i] != 0) {
					if (flowData->isNodata(tx, ty)) {
						is_done[i] = 0;  //If location has no data for flow direction it is done
					}
					//else part_has[i] = -1;  // some other partition has control
					//if (flowData->isInPartition(tx, ty) && dist_moved[i] >= 0) {
					td = srcData->getData(tx, ty, td);
					//	td = (char) td;  //DGT Why do we have to do this CHAR.  It seems like it was declared short so should all work
					if (srcData->isNodata(tx, ty) || td < itresh) {  //  If not on stream.  The rule is any value >= itresh is a stream, less or no data is not stream
						//move the outlet
						dirn = flowData->getData(tx, ty, dirn);
						//dirn = (char) dirn;
						//  DGT added dist_moved condition below which is: 
						//  More than the max_number of grid cells are traversed, or the traversal ends up going out of the 
						//  domain (encountering a no data D8 flow direction value).  
						//  The point is not moved and the 'dist_moved' field is assigned a value of -1.
						if (dirn >= 1 && dirn <= 8 && dist_moved[i] < maxdist) {
							nextx = outletsX[i] + d2[dirn];
							nexty = outletsY[i] + d1[dirn];
							if (nextx >= 0 && nexty >= 0 && nextx < pTotalX && nexty < pTotalY) {  // If is within global domain
								outletsX[i] = nextx;
								outletsY[i] = nexty;
								//ismoved[i]=1;
								dist_moved[i]++;
								//printf("Outlet: %d, x: %d, y: %d\n",i,nextx,nexty);
							}
							else {
								// moved off the map
								dist_moved[i] = -1;
								is_done[i] = 0;
							}
						}
						else {
							//flow data not a direction or moved more than max distance so do not move
							dist_moved[i] = -1;
							is_done[i] = 0;
						}
					}
					else {  // Here on stream as determined by there being data for src and it being >= ithresh
						is_done[i] = 0;
					}
					//  Since any moves were on global variable, update the local values
					flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
				}
				//  Here we drop out of the while loop.  The point is either done or has moved to another partition
				done = done + is_done[i];
			}
			//  Here we drop out of the outlet loop and have looped over all outlets and done as much as this partition process can do.  
			//  done holds the number of undone outlets.
			//  Share and reconcile across partitions

			// Debugging on outlet 0
			//fprintf(fdebug, "\nProcessor: %d, done: %d, nxy: %d, outletsX: %d, outletsY: %d, dist_moved: %d\n", rank, done, nxy, outletsX[0], outletsY[0], dist_moved[0]);
			//fflush(fdebug);

			if (size > 1) {
				//share data with neighbors
				//this code is linear partition specific
				int * toutletsX = new int[nxy];
				int * toutletsY = new int[nxy];
				int * tis_done = new int[nxy];
				long * tdist_moved = new long[nxy];
				MPI_Status status;


				int txn, tyn;

				// Sharing logic
				//  Process 0 sends to 1.
				//	Process 1 receives, merges and sends to 2
				//	Process 2 receives, merges and sends to 3 and so on until last
				//	Last process receives, merges and sends to 0
				//	Process 0 receives merges, checks for overall completion and broadcasts result that is now consistent across processes

				//  Based on ideas from http://mpitutorial.com/tutorials/mpi-send-and-receive/
				//  and MPI reference http://mpi-forum.org/docs/mpi-3.0/mpi30-report.pdf

				//  This block sends outlets to the partition above (destination rank one less)
				int dest = rank + 1;
				if (dest >= size)dest = 0;  //Wrap to the first partition
				int origin = rank - 1;
				if (origin < 0)origin = size - 1;
				if (rank == 0)
				{
					MPI_Send(outletsX, nxy, MPI_INT, dest, 0, MCW);
					MPI_Send(outletsY, nxy, MPI_INT, dest, 0, MCW);
					MPI_Send(dist_moved, nxy, MPI_LONG, dest, 0, MCW);
					MPI_Send(is_done, nxy, MPI_INT, dest, 0, MCW);

					// Receive then send
					MPI_Recv(toutletsX, nxy, MPI_INT, origin, 0, MCW, &status);
					MPI_Recv(toutletsY, nxy, MPI_INT, origin, 0, MCW, &status);
					MPI_Recv(tdist_moved, nxy, MPI_LONG, origin, 0, MCW, &status);
					MPI_Recv(tis_done, nxy, MPI_INT, origin, 0, MCW, &status);
					//  merge received outlets with local copy
					// Logic for merging
					//	If local copy is_done = 0 
					//     retain local copies
					//  else
					//	  if received is_done = 0 
					//       retain received copies
					//	  else if dist_moved received > dist_moved local
					//		retain received copies

					//  Implement this as
					//	If local copy is_done > 0 // local copy not done so examine what is received
					//	  if received is_done = 0 
					//       retain received values
					//	  else if dist_moved received > dist_moved local  indicating that another process moved this
					//		retain received values
					for (i = 0; i < nxy; i++) {
						done = 0;
						if (is_done[i] > 0) {
							if (tis_done[i] == 0) {  // Retain received values
								is_done[i] = tis_done[i];
								outletsX[i] = toutletsX[i];
								outletsY[i] = toutletsY[i];
								dist_moved[i] = tdist_moved[i];
							}
							else if (tdist_moved[i] > dist_moved[i]) {  // Retain received values
								is_done[i] = tis_done[i];
								outletsX[i] = toutletsX[i];
								outletsY[i] = toutletsY[i];
								dist_moved[i] = tdist_moved[i];
							}
						}
						done = done + is_done[i];  // This tracks overall completion
					}
				}
				else {  // Other processes recieve, merge, send
					MPI_Recv(toutletsX, nxy, MPI_INT, origin, 0, MCW, &status);
					MPI_Recv(toutletsY, nxy, MPI_INT, origin, 0, MCW, &status);
					MPI_Recv(tdist_moved, nxy, MPI_LONG, origin, 0, MCW, &status);
					MPI_Recv(tis_done, nxy, MPI_INT, origin, 0, MCW, &status);

					//  merge received outlets with local copy
					// 
					//	If local copy is_done > 0 // local copy not done so examine what is received
					//	  if received is_done = 0 
					//       retain received values
					//	  else if dist_moved received > dist_moved local  indicating that another process moved this
					//		retain received values
					for (i = 0; i < nxy; i++) {
						if (is_done[i] > 0) {
							if (tis_done[i] == 0) {  // Retain received values
								is_done[i] = tis_done[i];
								outletsX[i] = toutletsX[i];
								outletsY[i] = toutletsY[i];
								dist_moved[i] = tdist_moved[i];
							}
							else if (tdist_moved[i] > dist_moved[i]) {  // Retain received values
								is_done[i] = tis_done[i];
								outletsX[i] = toutletsX[i];
								outletsY[i] = toutletsY[i];
								dist_moved[i] = tdist_moved[i];
							}
						}
					}

					MPI_Send(outletsX, nxy, MPI_INT, dest, 0, MCW);
					MPI_Send(outletsY, nxy, MPI_INT, dest, 0, MCW);
					MPI_Send(dist_moved, nxy, MPI_LONG, dest, 0, MCW);
					MPI_Send(is_done, nxy, MPI_INT, dest, 0, MCW);
				}

				// Here we are back in the flow for all processes
				MPI_Bcast(outletsX, nxy, MPI_INT, 0, MCW);
				MPI_Bcast(outletsY, nxy, MPI_INT, 0, MCW);
				MPI_Bcast(dist_moved, nxy, MPI_LONG, 0, MCW);
				MPI_Bcast(is_done, nxy, MPI_INT, 0, MCW);
				MPI_Bcast(&done, 1, MPI_INT, 0, MCW);

				delete[] toutletsX;
				delete[] toutletsY;
				delete[] tis_done;
				delete[] tdist_moved;
			}  //  This is the end of if (size > 1) for sharing
		}//  This is the end of while(done > 0)


				//if (rank >= 0 && rank <= 2) {
				//	fprintf(fdebug, "After send/receive up: Processor: %d, nxy: %d, toutletsX: %d, toutletsY: %d, tdist_moved: %d\n", rank, nxy, toutletsX[0], toutletsY[0], tdist_moved[0]);
				//	fflush(fdebug);
				//}



				//for (i = 0; i < nxy; i++) {
				//	//if a node in the temp arrays belongs to us, and we don't already have it, get it
				//	flowData->globalToLocal(toutletsX[i], toutletsY[i], txn, tyn);  //DGT with single partition this is being passed uninitialized information
				//	if (rank >= 0 && rank <= 2) {
				//		int isin = 0;
				//		if (flowData->isInPartition(txn, tyn))isin = 1;
				//		fprintf(fdebug, "Grab check up: Processor: %d, txn: %d, tyn: %d, isin: %d, part_has: %d\n", rank, txn, tyn, isin, part_has[0]);
				//		fflush(fdebug);
				//	}
				//	if (flowData->isInPartition(txn, tyn) && !(part_has[i] == rank)) {  // if an outlet not in partition is now in partition - grab it
				//		outletsX[i] = toutletsX[i];
				//		outletsY[i] = toutletsY[i];
				//		part_has[i] = rank;
				//		dist_moved[i] = tdist_moved[i];
				//		fprintf(fdebug, "Up Grab: Processor: %d, outletsX: %d, outletsY: %d\n", rank, outletsX[i], outletsX[i]);
				//		fflush(fdebug);
				//	}
				//	flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
				//	if (!flowData->isInPartition(tx, ty))part_has[i] = -1;  // Make sure part_has is updated if out of partition
				//}


				////  This block sends outlets to the partition below (destination rank one more)
				//dest = rank + 1;
				//if (dest >= size)dest = 0;  //Wrap to the top partition  
				//origin = rank - 1;
				//if (origin < 0)origin = size - 1;
				//if (rank == 0)
				//{
				//	// Receive then send
				//	MPI_Recv(toutletsX, nxy, MPI_INT, origin, 0, MCW, &status);
				//	MPI_Recv(toutletsY, nxy, MPI_INT, origin, 0, MCW, &status);
				//	MPI_Recv(tdist_moved, nxy, MPI_LONG, origin, 0, MCW, &status);

				//	MPI_Send(outletsX, nxy, MPI_INT, dest, 0, MCW);
				//	MPI_Send(outletsY, nxy, MPI_INT, dest, 0, MCW);
				//	MPI_Send(dist_moved, nxy, MPI_LONG, dest, 0, MCW);
				//}
				//else {
				//	// Send then receive
				//	MPI_Send(outletsX, nxy, MPI_INT, dest, 0, MCW);
				//	MPI_Send(outletsY, nxy, MPI_INT, dest, 0, MCW);
				//	MPI_Send(dist_moved, nxy, MPI_LONG, dest, 0, MCW);


				//	MPI_Recv(toutletsX, nxy, MPI_INT, origin, 0, MCW, &status);
				//	MPI_Recv(toutletsY, nxy, MPI_INT, origin, 0, MCW, &status);
				//	MPI_Recv(tdist_moved, nxy, MPI_LONG, origin, 0, MCW, &status);
				//}
				////MPI_Buffer_attach(buf,bsize);
				////MPI_Bsend(outletsX, nxy, MPI_INT, dest, 4, MCW);
				////MPI_Buffer_detach(&ptr,&place);
				////MPI_Buffer_attach(buf,bsize);
				////MPI_Bsend(outletsY, nxy, MPI_INT, dest, 5, MCW);
				////MPI_Buffer_detach(&ptr,&place);
				////MPI_Buffer_attach(lbuf,lsize);
				////MPI_Bsend(dist_moved, nxy, MPI_LONG, dest, 7, MCW);
				////MPI_Buffer_detach(&ptr,&place);
				////MPI_Recv(toutletsX, nxy, MPI_INT, MPI_ANY_SOURCE, 4, MCW, &status);
				////MPI_Recv(toutletsY, nxy, MPI_INT, MPI_ANY_SOURCE, 5, MCW, &status);
				////MPI_Recv(tdist_moved, nxy, MPI_LONG, MPI_ANY_SOURCE, 7, MCW, &status);

				//if (rank >= 0 && rank <= 2) {
				//	fprintf(fdebug, "After send/receive down: Processor: %d, nxy: %d, toutletsX: %d, toutletsY: %d, tdist_moved: %d\n", rank, nxy, toutletsX[0], toutletsY[0], tdist_moved[0]);
				//	fflush(fdebug);
				//}
				//for (i = 0; i < nxy; i++) {
				//	//if a node in the temp arrays belongs to us, and we don't already have it, get it
				//	flowData->globalToLocal(toutletsX[i], toutletsY[i], txn, tyn);
				//	if (rank >= 0 && rank <= 2) {
				//		int isin = 0;
				//		if (flowData->isInPartition(txn, tyn))isin = 1;
				//		fprintf(fdebug, "Grab check down: Processor: %d, txn: %d, tyn: %d, isin: %d, part_has: %d\n", rank, txn, tyn, isin, part_has[0]);
				//		fflush(fdebug);
				//	}
				//	if (flowData->isInPartition(txn, tyn) && !(part_has[i] == rank)) {
				//		outletsX[i] = toutletsX[i];
				//		outletsY[i] = toutletsY[i];
				//		part_has[i] = rank;
				//		dist_moved[i] = tdist_moved[i];
				//		fprintf(fdebug, "Down Grab: Processor: %d, outletsX: %d, outletsY: %d\n", rank, outletsX[i], outletsY[i]);
				//		fflush(fdebug);
				//	}
				//	flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
				//	if (!flowData->isInPartition(tx, ty))part_has[i] = -1;  // Make sure part_has is updated if out of partition
				//}


		//	// each process figures out how many nodes it currently has and how many are done
		//	localnodes = 0;
		//	localdone = 0;
		//	for (i = 0; i < nxy; i++) {
		//		flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
		//		if (flowData->isInPartition(tx, ty)) {
		//			localnodes++;
		//			td = srcData->getData(tx, ty, td);
		//			//td = (char) td; 
		//			if (td >= itresh && !srcData->isNodata(tx, ty))localdone++;
		//			else if (dist_moved[i] < 0)localdone++;  // DGT added this as a termination condition
		//		}
		//	}

		//	//printf("Processor: %d, localnodes: %d, localdone: %d\n", rank, localnodes, localdone);
		//	//fflush(stdout);
		//	//total up all the nodes and all the finished nodes
		//	MPI_Reduce(&localnodes, &totalnodes, 1, MPI_INT, MPI_SUM, 0, MCW);
		//	MPI_Reduce(&localdone, &totaldone, 1, MPI_INT, MPI_SUM, 0, MCW);
		//	//if(!rank)printf("dist = %d\tfinished %d out of %d nodes.\n",dist, totaldone,totalnodes);

		//	//if all the nodes are done or we've moved them all maxdist, terminate the loop
		//	if (rank == 0) {
		//		if (totaldone == totalnodes) {
		//			done = 1;
		//		}
		//	}
		//	MPI_Bcast(&done, 1, MPI_INT, 0, MCW);
		//}

		//// look for unresolved outlets, set dist_moved to -1
		//for( i=0; i<nxy; i++){
		//	flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
		//	if(flowData->isInPartition(tx,ty)){
		//		td=srcData->getData(tx,ty,td);
		//		if(td<itresh && dist_moved[i]==maxdist){
		//			dist_moved[i]=-1;
		//			//				ismoved[i]=0;
		//		}
		//	}
		//}

		// oooooooooooooooooooooo  end  processing

		
		// In rank 0 write the shapefile that contains the moved src points
		if (rank == 0) {
			// Temporary arrays for output
		//double *tempxnode, *tempynode;
		//long  *tempdist_moved;  //  *tempismoved,
		//tempxnode = new double[nxy];
		//tempynode = new double[nxy];
		//tempdist_moved = new long[nxy];

		//for(i=0;i<nxy;++i){
		//	tempxnode[i]=0;
		//	tempynode[i]=0;
		//	//		tempismoved[i]=0;
		//	tempdist_moved[i]=0;
		//}

		//if(!rank)printf("setting up temp arrays...",dist, totaldone,totalnodes);
		// set temp arrays with local data in each process
		// each local array should have it's local data in the array at the proper place
		// zeros elsewhere
			for (i = 0; i < nxy; i++) {
				if (dist_moved[i] < 0) {
					// Point is not moved
					xnode[i] = origxnode[i];
					ynode[i] = origynode[i];
				}
				else {
					// Get geo values from new location 
					p.globalXYToGeo(outletsX[i], outletsY[i], xnode[i], ynode[i]);
				}
			}

			//flowData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
			//if(flowData->isInPartition(tx,ty)){
			//	tempxnode[i]=xnode[i];
			//	tempynode[i]=ynode[i];
			//	//			tempismoved[i]=ismoved[i];
			//	tempdist_moved[i]=dist_moved[i];
			//	/*
			//	printf("p%d:\t",rank);
			//	printf("x:%d\t",outletsX[i]);
			//	printf("y:%d\t",outletsY[i]);
			//	printf("strm:%d\t",td);
			//	printf("dir:%d\t",tf);
			//	printf("distmvd:%d\t",dist_moved[i]);
			//	printf("ismoved:%d\t",ismoved[i]);
			//	printf("\n");
			//	*/
			//}
		//if(!rank){
		//	for( i=0; i<nxy; i++){
		//		// if the outlet is outside the DEM, p0 puts it back in the temp array
		//		if(outletsX[i]<0||outletsX[i]>=pTotalX||outletsY[i]<0||outletsY[i]>=pTotalY){
		//			tempxnode[i]=origxnode[i];
		//			tempynode[i]=origynode[i];
		//			//			tempismoved[i]=0;
		//			tempdist_moved[i]=-1;
		//		} 
		//		// if dist_moved == 0, use original points
		//	}
		//}
		////if(!rank)printf("done.\n",dist, totaldone,totalnodes);


		////if(!rank)printf("reducing data...",dist, totaldone,totalnodes);
		//MPI_Reduce(tempxnode, xnode, nxy, MPI_DOUBLE, MPI_SUM, 0, MCW);
		//MPI_Reduce(tempynode, ynode, nxy, MPI_DOUBLE, MPI_SUM, 0, MCW);
		////	MPI_Reduce(tempismoved, ismoved, nxy, MPI_LONG, MPI_SUM, 0, MCW);
		//MPI_Reduce(tempdist_moved, dist_moved, nxy, MPI_LONG, MPI_SUM, 0, MCW);
		////if(!rank)printf("done\n.",dist, totaldone,totalnodes);

		//if(!rank){
		//	for( i=0; i<nxy; i++){
		//		// if the distance moved is 0, use original x y
		//		if(dist_moved[i]<=0){  // DGT changed condition to dist_moved <=0 because original values kept whenever not moved for whatever reason
		//			xnode[i]=origxnode[i];
		//			ynode[i]=origynode[i];
		//		} 
		//	}
		//}

		//delete [] tempxnode;
		//delete [] tempynode;
		////	delete [] tempismoved;
		//delete [] tempdist_moved;


		 //if(rank==0){

			int i = 0;
			OGR_L_ResetReading(hLayersh);

			while ((hFeaturesh = OGR_L_GetNextFeature(hLayersh)) != NULL) {


				//hFeaturesh=OGR_L_GetFeature(hLayersh,i); // get feature infor for outlet shapefile
				double x = xnode[i];  // DGT says does not need +pdx/2.0;
				double y = ynode[i];  // DGT +pdy/2.0;
				if (hDSshmoved != NULL) {
					hFeatureshmoved = OGR_F_Create(OGR_L_GetLayerDefn(hLayershmoved)); // create new feature with null fields and no geometry
				  //  
					//hFDefnsh = OGR_L_GetLayerDefn(hLayersh);
					int iField;
					int nField = OGR_FD_GetFieldCount(hFDefnshmoved); // get field count

					// get field value from outlet shapefile and put into in moved outletshapefile

					for (iField = 0; iField < OGR_FD_GetFieldCount(hFDefnsh); iField++)

					{
						hFieldDefn = OGR_FD_GetFieldDefn(hFDefnsh, iField); //get field from outletshapefile

						// for interger type field 
						if (OGR_Fld_GetType(hFieldDefn) == OFTInteger) {
							//printf( "%d,", OGR_F_GetFieldAsInteger( hFeaturesh, iField ));
							int val1 = OGR_F_GetFieldAsInteger(hFeaturesh, iField);
							OGR_F_SetFieldInteger(hFeatureshmoved, iField, val1);
						} // set field value in moveoutletshapefile
// for double type field
						else if (OGR_Fld_GetType(hFieldDefn) == OFTReal) {
							double val2 = OGR_F_GetFieldAsDouble(hFeaturesh, iField);
							OGR_F_SetFieldDouble(hFeatureshmoved, iField, val2);
						}
						// for string type field

						else if (OGR_Fld_GetType(hFieldDefn) == OFTString) {
							const char *val3 = OGR_F_GetFieldAsString(hFeaturesh, iField);
							OGR_F_SetFieldString(hFeatureshmoved, iField, val3);
						}

						else {
							const char *val4 = OGR_F_GetFieldAsString(hFeaturesh, iField);
							OGR_F_SetFieldString(hFeatureshmoved, iField, val4);
						}

					}

					// set value of dist_moved field  to moved outlet shapefile

					OGR_F_SetFieldInteger(hFeatureshmoved, (nField - 1), (int)dist_moved[i]);
					hGeometryshmoved = OGR_G_CreateGeometry(wkbPoint);// create geometry
					OGR_G_SetPoint_2D(hGeometryshmoved, 0, x, y);
					OGR_F_SetGeometry(hFeatureshmoved, hGeometryshmoved);
					OGR_G_DestroyGeometry(hGeometryshmoved);
					if (OGR_L_CreateFeature(hLayershmoved, hFeatureshmoved) != OGRERR_NONE)
					{
						printf(" warning: Failed to create feature in shapefile.\n");
						//exit( 1 );
					}

					OGR_F_Destroy(hFeatureshmoved); // destroy feature
					i = i + 1;
				}
			}
			//if(!rank)printf("closing file...",dist, totaldone,totalnodes);
			//delete [] indexMap;
			//delete [] types;
			//SHPClose(sh);
			//DBFClose(dbf);
			//SHPClose(shmoved);
			//DBFClose(dbfmoved);

			//OGR_DS_Destroy( hDSsh );
			OGR_DS_Destroy(hDSshmoved); // destroy datasource
		}
		//if(!rank)printf("done\n.",dist, totaldone,totalnodes);

		//OGR_F_Destroy( hFeature1);
		delete [] xnode;
		delete [] ynode;
		//	delete [] ismoved;
		delete [] dist_moved;
		delete [] outletsX;
		delete [] outletsY;
		//delete [] part_has;
		end = MPI_Wtime();
		double total,temp;
		total = end-begin;
		MPI_Allreduce (&total, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
		total = temp/size;

		
		

		if( rank == 0) 
			printf("Total time: %f\n",total);

	}MPI_Finalize();

	
	return 0;
}


