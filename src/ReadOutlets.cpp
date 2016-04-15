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




int readoutlets(char *outletsds,char *lyrname, int uselayername,int outletslyr,OGRSpatialReferenceH hSRSRaster,int *noutlets, double*& x, double*& y)

{   
	// initializing datasoruce,layer,feature, geomtery, spatial reference
    OGRSFDriverH    driver;
    OGRDataSourceH  hDS1;
	OGRLayerH       hLayer1;
	OGRFeatureDefnH hFDefn1;
	OGRFieldDefnH   hFieldDefn1;
	OGRFeatureH     hFeature1;
	OGRGeometryH    geometry, line;
	OGRSpatialReferenceH hRSOutlet;
	// regiser all ogr driver related to OGR
	OGRRegisterAll(); 
	// open datasource
	hDS1 = OGROpen(outletsds, FALSE,NULL); 
	if( hDS1 == NULL )
	{
		printf( "Error Opening datasource %s.\n",outletsds );
		exit( 1 );
	}

	//get layer from layer name
	if(uselayername==1) { hLayer1 = OGR_DS_GetLayerByName(hDS1,lyrname);}
		//get layerinfo from layer number
	else { hLayer1 = OGR_DS_GetLayer(hDS1,outletslyr);} // get layerinfo from layername

	if(hLayer1 == NULL)getlayerfail(hDS1,outletsds,outletslyr);
	OGRwkbGeometryType gtype;
	gtype=OGR_L_GetGeomType(hLayer1);
	if(gtype != wkbPoint)getlayerfail(hDS1,outletsds,outletslyr);
	// get spatial reference 
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1); 

	//if there is spatial reference then write warnings 

	if(hRSOutlet!=NULL) {

		int pj_raster=OSRIsProjected(hSRSRaster); // find if projected or not
		int pj_outlet=OSRIsProjected(hRSOutlet);
		const char *sprs;
		if(pj_raster==0) {sprs="GEOGCS";} else { sprs="PROJCS"; }

		const char* RasterProjectionName;
		const char* OutletProjectionName;
		RasterProjectionName = OSRGetAttrValue(hSRSRaster,sprs,0); // get projection name
		OutletProjectionName = OSRGetAttrValue(hRSOutlet,sprs,0);

		if (pj_raster==pj_outlet){
			 int rc= strcmp(RasterProjectionName,OutletProjectionName); // compare string
			 if(rc!=0){
				printf("Projection of Raster datasource %s.\n", RasterProjectionName);
                printf("Projecttion of Outlet feature %s.\n", OutletProjectionName);
				printf( "Warning: Projection of Outlet shapefile and Raster data are different.\n" );
				// TODO - Print the WKT and EPSG code of each.  If no spatial reference information, print unknown
				// TODO - Test how this works if spatial reference information is incomplete, and create at least one of the unit test functions with a shapefile without a .prj file, and one of the unit test functions a raster without a projection (eg an ASCII file)
			 }
		}
    
		else {
			  printf( "Warning: Spatial References of Outlet shapefile and Raster data are different.\n" );
			  // TODO - Print the WKT of each.  The general idea is that if these match, do not print anything.  
			  //  If these do not match give the user a warning.  Only give an error if the program can not proceed, such as would be the case if rows and columns did not match.
		}

	}


	long countPts=0;
	// count number of feature
	countPts=OGR_L_GetFeatureCount(hLayer1,0); 
	// get schema i.e geometry, properties (e.g. ID)
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1); 
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	int nxy=0;
	// loop through each feature and  get the latitude and longitude for each feature
	OGR_L_ResetReading(hLayer1);
    while( (hFeature1 = OGR_L_GetNextFeature(hLayer1)) != NULL ){
	//for( int j=0; j<countPts; j++) { //does not work for geojson file
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

int readoutlets(char *outletsds,char *lyrname,int uselayername,int outletslyr,OGRSpatialReferenceH hSRSRaster, int *noutlets, double*& x, double*& y, int*& id)

{
 
	// initializing datasoruce,layer,feature, geomtery, spatial reference
    OGRSFDriverH    driver;
	OGRDataSourceH  hDS1;
	OGRLayerH       hLayer1;
	OGRFeatureDefnH hFDefn1;
	OGRFieldDefnH   hFieldDefn1;
	OGRFeatureH     hFeature1;
	OGRGeometryH    geometry, line;
	OGRSpatialReferenceH hRSOutlet;
	OGRFieldDefnH hFieldDefn;
	OGRRegisterAll();
	// open data soruce

	hDS1 = OGROpen(outletsds, FALSE, NULL );
	if( hDS1 == NULL )
	{
	printf( "Error Opening in Shapefile .\n" );
	//exit( 1 );
	}
	
    //get layer from layer name
	if(uselayername==1) { hLayer1 = OGR_DS_GetLayerByName(hDS1,lyrname);}
		//get layerinfo from layer number
	else { hLayer1 = OGR_DS_GetLayer(hDS1,outletslyr);} // get layerinfo from layername

	if(hLayer1 == NULL)getlayerfail(hDS1,outletsds,outletslyr);
	OGRwkbGeometryType gtype;
	gtype=OGR_L_GetGeomType(hLayer1);
	if(gtype != wkbPoint)getlayerfail(hDS1,outletsds,outletslyr);
    //OGR_L_ResetReading(hLayer1);
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1);
	//if there is spatial reference then write warnings 
	if(hRSOutlet!=NULL){
	    int pj_raster=OSRIsProjected(hSRSRaster); // find if projected or not
		int pj_outlet=OSRIsProjected(hRSOutlet);
		const char *sprs;
		if(pj_raster==0) {sprs="GEOGCS";} else { sprs="PROJCS"; }

		const char* RasterProjectionName;
		const char* OutletProjectionName;
		RasterProjectionName = OSRGetAttrValue(hSRSRaster,sprs,0); // get projection name
		OutletProjectionName = OSRGetAttrValue(hRSOutlet,sprs,0);

		if (pj_raster==pj_outlet){
			  
			 int rc= strcmp(RasterProjectionName,OutletProjectionName); // compare string
			 if(rc!=0){
				printf("Projection of Raster datasource %s.\n",RasterProjectionName);
                printf("Projecttion of Outlet feature %s.\n",OutletProjectionName);
				printf( "Warning: Projection of Outlet shapefile and Raster data are different.\n" );
				// TODO - Print the WKT and EPSG code of each.  If no spatial reference information, print unknown
				// TODO - Test how this works if spatial reference information is incomplete, and create at least one of the unit test functions with a shapefile without a .prj file, and one of the unit test functions a raster without a projection (eg an ASCII file)
			 }
		}
    
		else {
			  printf( "Warning: Spatial References of Outlet shapefile and Raster data are different.\n" );
			  // TODO - Print the WKT of each.  The general idea is that if these match, do not print anything.  
			  //  If these do not match give the user a warning.  Only give an error if the program can not proceed, such as would be the case if rows and columns did not match.
		}
	}

	long countPts=0;
	countPts=OGR_L_GetFeatureCount(hLayer1,0); // get feature count
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1); // get schema i.e geometry, properties (e.g. ID)
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	int nxy=0;

	//hFeature1 = OGR_L_GetNextFeature(hLayer1);
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1);
	//hFeature1=OGR_L_GetFeature(hLayer1,0); // read first feature to get all field info
	//int idfld =OGR_F_GetFieldIndex(hFeature1,"Id"); // get index for the 'id' field
	//if (idfld >= 0)
	id = new int[countPts];
	// loop through each feature and get lat,lon and id information

    OGR_L_ResetReading(hLayer1);
    while( (hFeature1 = OGR_L_GetNextFeature(hLayer1)) != NULL ) {

		 //hFeature1=OGR_L_GetFeature(hLayer1,j); // get feature info
		 geometry = OGR_F_GetGeometryRef(hFeature1); // get geometry
         x[nxy] = OGR_G_GetX(geometry, 0);
		 y[nxy] =  OGR_G_GetY(geometry, 0);
		 int idfld =OGR_F_GetFieldIndex(hFeature1,"id");

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

