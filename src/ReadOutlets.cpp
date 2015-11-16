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
	// initializing datasoruce,layer,feature, geomtery, spatial reference
    OGRDataSourceH  hDS1;
	OGRLayerH       hLayer1;
	OGRFeatureDefnH hFDefn1;
	OGRFieldDefnH   hFieldDefn1;
	OGRFeatureH     hFeature1;
	OGRGeometryH    geometry, line;
	OGRSpatialReferenceH hRSOutlet;
	// regiser all ogr driver related to OGR
	OGRRegisterAll(); 
	// open shapefile
	hDS1 = OGROpen(outletsfile, FALSE, NULL ); 
	if( hDS1 == NULL )
	{
		printf( "Eorro Opening in Shapefile .\n" );
		exit( 1 );
	}

	// extracting layer name from the shapefile (e.g. from outlet.shp to outlet)
    char *layername; 
    layername=getLayername(outletsfile); // get layer name 
    hLayer1 = OGR_DS_GetLayerByName( hDS1,layername );
	// get spatial reference of ogr
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1); 
	
	const char* pszAuthorityNameRaster;
	const char* pszAuthorityNameOutlet;
	int pj_raster=OSRIsProjected(hSRSRaster); // find if projected or not
	int pj_outlet=OSRIsProjected(hRSOutlet);
	OSRAutoIdentifyEPSG(hSRSRaster); //identify EPSG code
	OSRAutoIdentifyEPSG(hRSOutlet);
	const char *sprs;
	if(pj_raster==0) {sprs="GEOGCS";} else { sprs="PROJCS"; }
	if (pj_raster==pj_outlet){
		 pszAuthorityNameRaster=OSRGetAuthorityCode(hSRSRaster,sprs);// get EPSG code
	     pszAuthorityNameOutlet=OSRGetAuthorityCode(hRSOutlet,sprs);
         if(atoi(pszAuthorityNameRaster)==atoi( pszAuthorityNameOutlet)){
			 printf("EPSG code of Outlet shapefile and Raster data are matched .\n" ); }
	   
    else {
	      printf( "Warning : EPSG code of Outlet shapefile and Raster data are different .\n" );
	 
	}}
    
    else {
	      printf( "Error : Spatial References of Outlet shapefile and Raster data are different .\n" );
	      exit(1);   
	}

	long countPts=0;
	// count number of feature
	countPts=OGR_L_GetFeatureCount(hLayer1,1); 
	// get schema i.e geometry, properties (e.g. ID)
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1); 
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	int nxy=0;
	// loop through each feature and  get the latitude and longitude for each feature
	for( int j=0; j<countPts; j++) {

         hFeature1=OGR_L_GetFeature(hLayer1,j); //get feature
		 geometry = OGR_F_GetGeometryRef(hFeature1); // get geometry type
		 x[nxy] = OGR_G_GetX(geometry, 0); 
		 y[nxy] =  OGR_G_GetY(geometry, 0); 
		 OGR_F_Destroy( hFeature1); // destroy feature
		 nxy++;
		}
    *noutlets=nxy; // total number of outlets point
	
	OGR_DS_Destroy( hDS1); // destroy data source
	return 0;
}

int readoutlets(char *outletsfile,OGRSpatialReferenceH hSRSRaster, int *noutlets, double*& x, double*& y, int*& id)

{
 
	//OGRSFDriverH    driver;
	// initializing 
	OGRDataSourceH  hDS1;
	OGRLayerH       hLayer1;
	OGRFeatureDefnH hFDefn1;
	OGRFieldDefnH   hFieldDefn1;
	OGRFeatureH     hFeature1;
	OGRGeometryH    geometry, line;
	OGRSpatialReferenceH hRSOutlet;
	OGRFieldDefnH hFieldDefn;
	OGRRegisterAll();
	// open shapefile

	hDS1 = OGROpen(outletsfile, FALSE, NULL );
	if( hDS1 == NULL )
	{
	printf( "Eorro Opening in Shapefile .\n" );
	exit( 1 );
	}
	// get layer name from shapefile
	char *layername; 
    layername=getLayername(outletsfile); // layer name is file name without extension
	hLayer1 = OGR_DS_GetLayerByName( hDS1,layername );
    //OGR_L_ResetReading(hLayer1);
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1);
    const char* pszAuthorityNameRaster;
	const char* pszAuthorityNameOutlet;
	int pj_raster=OSRIsProjected(hSRSRaster); // find if projected or not
	int pj_outlet=OSRIsProjected(hRSOutlet);
	OSRAutoIdentifyEPSG(hSRSRaster); //identify EPSG code
	OSRAutoIdentifyEPSG(hRSOutlet);
	const char *sprs;
	if(pj_raster==0) {sprs="GEOGCS";} else { sprs="PROJCS"; }
	if (pj_raster==pj_outlet){
		 pszAuthorityNameRaster=OSRGetAuthorityCode(hSRSRaster,sprs);// get EPSG code
	     pszAuthorityNameOutlet=OSRGetAuthorityCode(hRSOutlet,sprs);

	     if(atoi(pszAuthorityNameRaster)==atoi( pszAuthorityNameOutlet)){
			 printf("EPSG code of Outlet shapefile and Raster data are matched .\n" ); }
	   
    else {
	      printf( "Warning : EPSG code of Outlet shapefile and Raster data are different .\n" );
	      
	}}
    
    else {
	      printf( "Error: Spatial References of Outlet shapefile and Raster data of are different .\n" );
	      exit(1); // this means that outlets and raster in different system ( one is in geographic another is in projected ) 
	}

	long countPts=0;
	countPts=OGR_L_GetFeatureCount(hLayer1,1); // get feature count
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1); // get schema i.e geometry, properties (e.g. ID)
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	int nxy=0;

	
	//hFeature1 = OGR_L_GetNextFeature(hLayer1);
	hFeature1=OGR_L_GetFeature(hLayer1,0); // read first feature to get all field info
	int idfld =OGR_F_GetFieldIndex(hFeature1,"id"); // get index for the 'id' field
	if (idfld >= 0)id = new int[countPts];
	// loop through each feature and get lat,lon and id information
	for( int j=0; j<countPts; j++) {

		 hFeature1=OGR_L_GetFeature(hLayer1,j); // get feature info
		 geometry = OGR_F_GetGeometryRef(hFeature1); // get geometry
         x[nxy] = OGR_G_GetX(geometry, 0);
		 y[nxy] =  OGR_G_GetY(geometry, 0);

		 if (idfld >= 0)
		   {
			 
			hFieldDefn = OGR_FD_GetFieldDefn( hFDefn1,idfld); // get field definiton based on index
			if( OGR_Fld_GetType(hFieldDefn) == OFTInteger ) {
					id[nxy] =OGR_F_GetFieldAsInteger( hFeature1, idfld );} // get id value 
		    }
			nxy++; // count number of outlets point
		   OGR_F_Destroy( hFeature1 ); // destroy feature
		    }
	*noutlets=nxy;
	
	OGR_DS_Destroy( hDS1); // destroy data source
	return 0;
}

