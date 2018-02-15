/*  CatchOutlets function to derive catchment outlets at the downstream end of each stream reach.
     
   David Tarboton
  Utah State University  
  October 22, 2016
  
This function to take as input tree.dat, coord.dat, D8 flow directions *p.tif and a distance threshold.
To write an outlets shapefile that is the downstream end of each stream segment to be used in rapid watershed delineation functions.

Additional things it should do

1. Where the outlet is a downstream end move one cell down to have the catchment extend into downstream catchments to facilitate internally draining sinks
2. Where an outlet is within the threshold distance of the downstream end it is not written to thin the number of outlets and partitions that will be created.

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
#include "CatchOutlets.h"

using namespace std;
OGRSFDriverH    driver;
//OGRDataSourceH  hDSsh, hDSshmoved;
//OGRLayerH       hLayersh, hLayershmoved;
//OGRFeatureDefnH hFDefnsh,hFDefnshmoved;

//  Globals for recursion
int nstreams;
int *linknos;
int gwcount;
struct netlink {
	//int32_t linkno;
	int32_t dslinkno;
	int32_t uslinkno1;
	int32_t uslinkno2;
	double doutend;
	double uscontarea;
	double x, y;
};
OGRLayerH hLayer, hLayerpt;
int32_t linkfield, dslinkfield, uslink1field, uslink2field, doutendfield;
OGRGeometryH hGeometry, hGeometrypt;

netlink* allinks;

void CheckPoint(int thelink, double downout, double mindist, double minarea)
{
	int istream=0;
	while (linknos[istream] != thelink && istream < nstreams) {
		istream = istream + 1;
	}
	if (istream < nstreams) // Here a stream was found.  This traps the passing over streams with none upstream
	{
		if (allinks[istream].doutend - downout > mindist) {
			if (allinks[istream].uscontarea > minarea) {  // Only write if min area is exceeded
				//printf("%d, %g, %g\n", linknos[istream], allinks[istream].x, allinks[istream].y);
				hGeometrypt = OGR_G_CreateGeometry(wkbPoint);// create geometry
				OGRFeatureH hFeaturept = OGR_F_Create(OGR_L_GetLayerDefn(hLayerpt)); // create new feature with null fields and no geometry
				OGR_F_SetFieldInteger(hFeaturept, 0, linknos[istream]);
				OGR_F_SetFieldInteger(hFeaturept, 1, allinks[istream].dslinkno);
				OGR_F_SetFieldInteger(hFeaturept, 2, allinks[istream].uslinkno1);
				OGR_F_SetFieldInteger(hFeaturept, 3, allinks[istream].uslinkno2);
				OGR_F_SetFieldDouble(hFeaturept, 4, allinks[istream].doutend);
				OGR_F_SetFieldInteger(hFeaturept, 5, gwcount);
				gwcount = gwcount + 1;
				OGR_G_SetPoint_2D(hGeometrypt, 0, allinks[istream].x, allinks[istream].y);
				OGR_F_SetGeometry(hFeaturept, hGeometrypt);
				OGR_G_DestroyGeometry(hGeometrypt);
				OGR_L_CreateFeature(hLayerpt, hFeaturept);
				OGR_F_Destroy(hFeaturept);

				downout = allinks[istream].doutend;
			}
		}
		if (allinks[istream].uscontarea > minarea) {  // Only recurse up if minarea is satisified
			if (allinks[istream].uslinkno1 >= 0) CheckPoint(allinks[istream].uslinkno1, downout, mindist, minarea);
			if (allinks[istream].uslinkno2 >= 0) CheckPoint(allinks[istream].uslinkno2, downout, mindist, minarea);
		}
	}
}


// int catchoutlets(char *pfile, char *streamnetsrc, char *streamnetlyr, char *outletsdatasrc, char *outletslayer, int  lyrno, float maxdist)

int catchoutlets(char *pfile, char *streamnetsrc, char *outletsdatasrc, double mindist, int gwstartno, double minarea)
{

	MPI_Init(NULL,NULL);{
		int rank,size;
		MPI_Comm_rank(MCW,&rank);
		MPI_Comm_size(MCW,&size);
		if (rank == 0) {
			printf("Catchment Outlets version %s\n", TDVERSION);
			fflush(stdout);
		}

		double begin,end;
		//Begin timer
		begin = MPI_Wtime();
		int d1[9] = {-99,0,-1,-1,-1,0,1,1,1};
		int d2[9] = {-99,1,1,0,-1,-1,-1,0,1};

		//load the d8 flow grid into a linear partition
		//Create tiff object, read and store header info
		tiffIO p(pfile, SHORT_TYPE);
		long pTotalX = p.getTotalX();
		long pTotalY = p.getTotalY();
		double pdx = p.getdxA();
		double pdy = p.getdyA();

		if (rank == 0)
		{
			float timeestimate = (2e-7*pTotalX*pTotalY / pow((double)size, 0.65)) / 60 + 1;  // Time estimate in minutes
			fprintf(stderr, "This run may take on the order of %.0f minutes to complete.\n", timeestimate);
			fprintf(stderr, "This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

		OGRSpatialReferenceH hSRSRaster;
		hSRSRaster = p.getspatialref();

		//Create partition and read data
		tdpartition *flowData;
		flowData = CreateNewPartition(p.getDatatype(), pTotalX, pTotalY, pdx, pdy, p.getNodata());
		int pnx = flowData->getnx();
		int pny = flowData->getny();
		int pxstart, pystart;
		flowData->localToGlobal(0, 0, pxstart, pystart);
		p.read(pxstart, pystart, pny, pnx, flowData->getGridPointer());

		if (rank == 0) {

			//  OGR code based on http://gdal.org/1.11/ogr/ogr_apitut.html
			//   and http://www.gdal.org/ogr__api_8h.html 

			OGRRegisterAll();
			//read stream network datasource
			OGRDataSourceH hDS, hDSpt;

			OGRFeatureH hFeature;

			hDS = OGROpen(streamnetsrc, FALSE, NULL);
			if (hDS == NULL)
			{
				printf("Open failed.\n");
				exit(1);
			}

			//creating outlet shapefile
			const char *pszDriverName;
			pszDriverName = getOGRdrivername(outletsdatasrc);
			driver = OGRGetDriverByName(pszDriverName);
			if (driver == NULL)
			{
				printf("%s warning:driver not available.\n", pszDriverName);
				//exit( 1 );
			}
			// Create new file using this driver if the data source exists 
			if (pszDriverName == "SQLite")
			{
				hDSpt = OGROpen(outletsdatasrc, TRUE, NULL);
			}
			else {
				hDSpt = OGR_Dr_CreateDataSource(driver, outletsdatasrc, NULL);
			}

			if (hDSpt != NULL) {

				//hLayershmoved = OGR_DS_CreateLayer( hDSshmoved, outletmovedlayer, hSRSRaster, wkbPoint, NULL ); // create layer for moved outlet, where raster layer spatial reference is used fro shapefile
				if (strlen(outletsdatasrc) == 0) {  //Code not ready as outletdatasrc should never be 0
					//char * layernamept;
					//layernamept = getLayername(outletsdatasrc); // get layer name which is file name without extension
					// Chris George Suggestion
					char layernamept[MAXLN];
					getLayername(outletsdatasrc,layernamept); // get layer name which is file name without extension
					hLayerpt = OGR_DS_CreateLayer(hDSpt, layernamept, hSRSRaster, wkbPoint, NULL);
				}

				else {
					hLayerpt = OGR_DS_CreateLayer(hDSpt, outletsdatasrc, hSRSRaster, wkbPoint, NULL);
				}// provide same spatial reference as raster in point file file

				if (hLayerpt == NULL)
				{
					printf("warning: Layer creation failed.\n");
					//exit( 1 );
				}
			}


			int lyrno = 0;
			hLayer = OGR_DS_GetLayer(hDS, lyrno);

			nstreams = OGR_L_GetFeatureCount(hLayer, TRUE);
			allinks = new netlink[nstreams];
			linknos = new int32_t[nstreams];

			OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(hLayer);
			OGRFieldDefnH hFieldDefn;

			// Replicate fields we want in point shapefile
			linkfield = OGR_FD_GetFieldIndex(hFDefn, "LINKNO");
			hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, linkfield);
			OGR_L_CreateField(hLayerpt, hFieldDefn, 0);

			dslinkfield = OGR_FD_GetFieldIndex(hFDefn, "DSLINKNO");
			hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, dslinkfield);
			OGR_L_CreateField(hLayerpt, hFieldDefn, 0);

			uslink1field = OGR_FD_GetFieldIndex(hFDefn, "USLINKNO1");
			hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, uslink1field);
			OGR_L_CreateField(hLayerpt, hFieldDefn, 0);

			uslink2field = OGR_FD_GetFieldIndex(hFDefn, "USLINKNO2");
			hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, uslink2field);
			OGR_L_CreateField(hLayerpt, hFieldDefn, 0);

			doutendfield = OGR_FD_GetFieldIndex(hFDefn, "DOUTEND");
			hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, doutendfield);
			OGR_L_CreateField(hLayerpt, hFieldDefn, 0);

			// add new id field to number gages
			hFieldDefn = OGR_Fld_Create("Id", OFTInteger);
			OGR_Fld_SetWidth(hFieldDefn, 10); // set field width
			OGR_L_CreateField(hLayerpt, hFieldDefn, 0);

			//  Fields we use but do not add to new feature class
			int32_t doutstartfield = OGR_FD_GetFieldIndex(hFDefn, "DOUTSTART");
			int32_t uscontareafield = OGR_FD_GetFieldIndex(hFDefn, "USContArea");	

			int32_t istream = 0;
			gwcount = gwstartno;  // Initialize count

			OGR_L_ResetReading(hLayer);
			while ((hFeature = OGR_L_GetNextFeature(hLayer)) != NULL)
			{
				//  Here pick the downstream end of a link that is an outlet.  Otherwise pick the upstream end 
				//  skipping links without any upstream
				int32_t linkno= OGR_F_GetFieldAsInteger(hFeature, linkfield);
				int32_t dslinkno = OGR_F_GetFieldAsInteger(hFeature, dslinkfield);
				int32_t uslinkno1 = OGR_F_GetFieldAsInteger(hFeature, uslink1field);
				int32_t uslinkno2 = OGR_F_GetFieldAsInteger(hFeature, uslink2field);
				// Only do for outlet or if it has no links upstream
				if(dslinkno < 0  || uslinkno1 >= 0 || uslinkno2 >= 0)   //  Here pick the downstream end
				{
					linknos[istream] = linkno;
					allinks[istream].dslinkno = dslinkno;
					allinks[istream].uslinkno1 = uslinkno1;
					allinks[istream].uslinkno2 = uslinkno2;

					hGeometry = OGR_F_GetGeometryRef(hFeature);
					int num_points = OGR_G_GetPointCount(hGeometry);
					double X1, Y1, Z1;
					if (dslinkno < 0) // points are digitized down to up so downstream end use first entered
					{
						OGR_G_GetPoint(hGeometry, 0, &X1, &Y1, &Z1);
						allinks[istream].doutend = OGR_F_GetFieldAsDouble(hFeature, doutendfield);  // Note here end field
					}
					else  // use last
					{
						OGR_G_GetPoint(hGeometry, num_points-1, &X1, &Y1, &Z1);  
						allinks[istream].doutend = OGR_F_GetFieldAsDouble(hFeature, doutstartfield); // Note here start field
					}
					allinks[istream].uscontarea = OGR_F_GetFieldAsDouble(hFeature, uscontareafield);
					allinks[istream].x = X1;
					allinks[istream].y = Y1;
					//printf("%d, %lf, %lf,%lf\n", istream, X1, Y1, Z1);
					istream = istream + 1;
				}
			}
			nstreams = istream;  // Reset as we do not work with every stream
			// For parallel version here share allinks or perhaps we are lucky and parallel OGR reading is OK
			// Now loop over links recursively evaluating distances
			for (istream = 0; istream < nstreams; istream++)
			{
				if (allinks[istream].dslinkno < 0) // Here this is a down end
				{
					int32_t gx, gy, tx, ty, nextx, nexty;
					double geoX, geoY;
					p.geoToGlobalXY(allinks[istream].x, allinks[istream].y, gx, gy);
					flowData->globalToLocal(gx, gy, tx, ty);
					if (flowData->isInPartition(tx, ty))
					{
						short dirn;
						dirn = flowData->getData(tx, ty, dirn);
						if (dirn >= 1 && dirn <= 8) {
							nextx = gx + d2[dirn];
							nexty = gy + d1[dirn];
							//  Don't go out of total domain
							if (nextx < 0 || nextx >= pTotalX || nexty < 0 || nexty >= pTotalY)
							{
								nextx = gx;
								nexty = gy;
							}
							p.globalXYToGeo(nextx, nexty, geoX, geoY);
							hGeometrypt = OGR_G_CreateGeometry(wkbPoint);// create geometry
							OGRFeatureH hFeaturept = OGR_F_Create(OGR_L_GetLayerDefn(hLayerpt)); // create new feature with null fields and no geometry
							OGR_F_SetFieldInteger(hFeaturept, 0, linknos[istream]);
							OGR_F_SetFieldInteger(hFeaturept, 1, allinks[istream].dslinkno);
							OGR_F_SetFieldInteger(hFeaturept, 2, allinks[istream].uslinkno1);
							OGR_F_SetFieldInteger(hFeaturept, 3, allinks[istream].uslinkno2);
							OGR_F_SetFieldDouble(hFeaturept, 4, allinks[istream].doutend);	
							//  Write and increment gage watershed count.  Here trouble for parallel
							OGR_F_SetFieldInteger(hFeaturept, 5, gwcount);
							gwcount = gwcount + 1;

							OGR_G_SetPoint_2D(hGeometrypt, 0, geoX, geoY);
							OGR_F_SetGeometry(hFeaturept, hGeometrypt);
							OGR_G_DestroyGeometry(hGeometrypt);
							OGR_L_CreateFeature(hLayerpt, hFeaturept);
							OGR_F_Destroy(hFeaturept);
							double downout = 0.0;
							// Recursive calls to traverse up
							if (allinks[istream].uslinkno1 >= 0) CheckPoint(allinks[istream].uslinkno1, downout,mindist,minarea);
							if (allinks[istream].uslinkno2 >= 0) CheckPoint(allinks[istream].uslinkno2, downout,mindist,minarea);
						}
					}
				}
			}



			/*			//Create a new feature corresponding to each existing feature
						OGRFeatureH hFeaturept = OGR_F_Create(OGR_L_GetLayerDefn(hLayerpt)); // create new feature with null fields and no geometry

						// Copy the field values from the old field to the new field
						int iField;

						for (iField = 0; iField < OGR_FD_GetFieldCount(hFDefn); iField++)
						{
							OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);

							if (OGR_Fld_GetType(hFieldDefn) == OFTInteger)
							{
								printf("%d,", OGR_F_GetFieldAsInteger(hFeature, iField));
								int val1 = OGR_F_GetFieldAsInteger(hFeature, iField);
								OGR_F_SetFieldInteger(hFeaturept, iField, val1);
							}
							else if (OGR_Fld_GetType(hFieldDefn) == OFTReal)
							{
								printf("%.3f,", OGR_F_GetFieldAsDouble(hFeature, iField));
								double val2 = OGR_F_GetFieldAsDouble(hFeature, iField);
								OGR_F_SetFieldDouble(hFeaturept, iField, val2);
							}
							else if (OGR_Fld_GetType(hFieldDefn) == OFTString)
							{
								printf("%s,", OGR_F_GetFieldAsString(hFeature, iField));
								const char *val3 = OGR_F_GetFieldAsString(hFeature, iField);
								OGR_F_SetFieldString(hFeaturept, iField, val3);
							}
							else
							{
								printf("%s,", OGR_F_GetFieldAsString(hFeature, iField));
								const char *val4 = OGR_F_GetFieldAsString(hFeature, iField);
								OGR_F_SetFieldString(hFeaturept, iField, val4);
							}

						}

						OGRGeometryH hGeometry, hGeometrypt;

						hGeometry = OGR_F_GetGeometryRef(hFeature);
						int num_points = OGR_G_GetPointCount(hGeometry);
						double X1, Y1, Z1;
						if (num_points < 2) {

						}
						for (int ii = 0; ii < num_points; ii++) {
							// Get the point!
							OGR_G_GetPoint(hGeometry, ii, &X1, &Y1, &Z1);
							printf("%d, %lf, %lf,%lf\n",ii,X1,Y1,Z1);
						}
						hGeometrypt = OGR_G_CreateGeometry(wkbPoint);// create geometry
						OGR_G_SetPoint_2D(hGeometrypt, 0, X1, Y1);  // Last value read
						OGR_F_SetGeometry(hFeaturept, hGeometrypt);
						OGR_G_DestroyGeometry(hGeometrypt);
						//  Now create the feature in the layer giving an error if failed
						if (OGR_L_CreateFeature(hLayerpt, hFeaturept) != OGRERR_NONE)
						{
							printf(" warning: Failed to create feature in shapefile.\n");
							//exit( 1 );
						}
						// destroy in memory features no longer needed
						OGR_F_Destroy(hFeaturept);
						OGR_F_Destroy(hFeature);
						*/

			OGR_DS_Destroy(hDS);
		}
	}MPI_Finalize();

	
	return 0;
}


