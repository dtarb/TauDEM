/*  TauDEM Read Outlets Function
     
  David G Tarboton
  Utah State University     
  May 23, 2010
  
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

#include <stdio.h>
#include <string.h>
#include "commonLib.h"
#include "ogr_api.h"


int readoutlets(char *outletsfile, OGRSpatialReferenceH hSRSRaster,int *noutlets, double*& x, double*& y)

{   
		//OGRSFDriverH    driver;
	OGRDataSourceH  hDS1;
	OGRLayerH       hLayer1;
	OGRFeatureDefnH hFDefn1;
	OGRFieldDefnH   hFieldDefn1;
	OGRFeatureH     hFeature1;
	OGRGeometryH    geometry, line;
	OGRSpatialReferenceH hRSOutlet;
	OGRRegisterAll();
	hDS1 = OGROpen(outletsfile, FALSE, NULL );
	if( hDS1 == NULL )
	{
		printf( "Eorro Opening in Shapefile .\n" );
		exit( 1 );
	}
	// extracting layer information from the shapefile

	char layername[MAXLN]; // layer name is file name without extension
	size_t len = strlen(outletsfile);
	memcpy(layername, outletsfile, len-4);
	layername[len - 4] = 0; // get file name without extension 
	hLayer1 = OGR_DS_GetLayerByName( hDS1,layername );
	OGR_L_ResetReading(hLayer1);
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1);
	int comSRS=OSRIsSame(hRSOutlet,hSRSRaster);

    if(comSRS==0) {
	printf( "Warning : Spatial References of Outlet shapefile and Raster data of are not matched .\n" );
	
    }

	long countPts=0;
	countPts=OGR_L_GetFeatureCount(hLayer1,1); // count number of feature
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1);
	int nxy=0;
	for( int j=0; j<countPts; j++) {

         hFeature1=OGR_L_GetFeature(hLayer1,j);
		 geometry = OGR_F_GetGeometryRef(hFeature1);
		 x[nxy] = OGR_G_GetX(geometry, 0);
		 y[nxy] =  OGR_G_GetY(geometry, 0);
		 nxy++;
		}
    *noutlets=nxy;
	OGR_F_Destroy( hFeature1);
	OGR_DS_Destroy( hDS1);
	return 0;
}

int readoutlets(char *outletsfile,OGRSpatialReferenceH hSRSRaster, int *noutlets, double*& x, double*& y, int*& id)

{
 
	//OGRSFDriverH    driver;
	OGRDataSourceH  hDS1;
	OGRLayerH       hLayer1;
	OGRFeatureDefnH hFDefn1;
	OGRFieldDefnH   hFieldDefn1;
	OGRFeatureH     hFeature1;
	OGRGeometryH    geometry, line;
	// OGRSpatialReferenceH  hSRSOutlet;
	OGRRegisterAll();
	OGRSpatialReferenceH hRSOutlet;
	hDS1 = OGROpen(outletsfile, FALSE, NULL );
	if( hDS1 == NULL )
	{
	printf( "Eorro Opening in Shapefile .\n" );
	exit( 1 );
	}
	char layername[MAXLN];

	size_t len = strlen(outletsfile);
	memcpy(layername, outletsfile, len-4);
	layername[len - 4] = 0;
	hLayer1 = OGR_DS_GetLayerByName( hDS1,layername );

	OGR_L_ResetReading(hLayer1);
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1);
	int comSRS=OSRIsSame(hRSOutlet,hSRSRaster);
    if(comSRS==0) {
	    printf( "Warning : Spatial References of Outlet shapefile and Raster data of are not matched .\n" );
	  
	}

	long countPts=0;
	countPts=OGR_L_GetFeatureCount(hLayer1,1);
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	int nxy=0;

	hFDefn1 = OGR_L_GetLayerDefn(hLayer1);
	hFeature1 = OGR_L_GetNextFeature(hLayer1);
	int idfld =OGR_F_GetFieldIndex(hFeature1,"id");
	if (idfld >= 0)id = new int[countPts];
	for( int j=0; j<countPts; j++) {

		 hFeature1=OGR_L_GetFeature(hLayer1,j);
		 geometry = OGR_F_GetGeometryRef(hFeature1);
         x[nxy] = OGR_G_GetX(geometry, 0);
		 y[nxy] =  OGR_G_GetY(geometry, 0);
		 if (idfld >= 0)
		   {
			OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn( hFDefn1,idfld);
			if( OGR_Fld_GetType(hFieldDefn) == OFTInteger ) {
					id[nxy] =OGR_F_GetFieldAsInteger( hFeature1, idfld );}
		    }
			nxy++;
			
		                         }
	*noutlets=nxy;
	OGR_F_Destroy( hFeature1 );
	OGR_DS_Destroy( hDS1);
	return 0;
}

