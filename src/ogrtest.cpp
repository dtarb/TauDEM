#include "ogr_api.h"
#include "commonLib.h"
using namespace std;
int readwriteogr(char *datasrc,char *layername,char *datasrcnew, char *layernamenew){

	OGRSFDriverH hDriver,hDriver1;
	OGRDataSourceH  hDS1,hDS2,hDS3;
	OGRLayerH       hLayer1,hLayer2,hLayer3;
	OGRFeatureDefnH hFDefn1,hFDefn2,hFDefn3;
	OGRFieldDefnH   hFieldDefn1,hFieldDefn2,hFieldDefn3;
	OGRFeatureH     hFeature1,hFeature2,hFeature3;
	OGRGeometryH    geometry, line;
	OGRSpatialReferenceH hRSOutlet;
	OGRFieldDefnH hFieldDefn;
	OGRRegisterAll();

	// open ogr file based on layer name

    hDS1 = OGROpen(datasrc, TRUE, NULL );
	if( hDS1 == NULL )
	{
	printf( "Error Opening datasource .\n" );
	exit( 1 );
	}
	double *x, *y; int *id, *noutlets;

    hLayer1 = OGR_DS_GetLayerByName( hDS1,layername);
    OGR_L_ResetReading(hLayer1);
	hRSOutlet = OGR_L_GetSpatialRef(hLayer1);

	long countPts=0;
	countPts=OGR_L_GetFeatureCount(hLayer1,1); // get feature count
	hFDefn1 = OGR_L_GetLayerDefn(hLayer1); // get schema i.e geometry, properties (e.g. ID)
	x = new double[countPts];
	y = new double[countPts];
	int iField;
	int nxy=0;	
	hFeature1=OGR_L_GetFeature(hLayer1,0); // read first feature to get all field info
	int idfld =OGR_F_GetFieldIndex(hFeature1,"id"); // get index for the 'id' field
	if (idfld >= 0)id = new int[countPts];

	// loop through each feature and get lat,lon and id information
	OGR_L_ResetReading(hLayer1);
    while( (hFeature1 = OGR_L_GetNextFeature(hLayer1)) != NULL ) {

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
	
	 //OGR_DS_Destroy( hDS1); // destroy data source


	const char *ogrextension_list[3] = {".sqlite",".shp",".geojson",};  // extension list --can add more 
	const char *ogrdriver_code[3] = {"SQLite","ESRI Shapefile","GeoJSON"};   //  code list -- can add more




// write new ogr file in different  data source

	

	size_t extension_num=3;
	char *ext; 
	int index = 1; //default is ESRI shapefile

	ext = strrchr(datasrcnew, '.'); 
	if(!ext){
		//strcat(datasrcnew,".tif");
		index=0;
	}
	else
	{

		//  convert to lower case for matching
		for(int i = 0; ext[i]; i++){
			ext[i] = tolower(ext[i]);
		}
		// if extension matches then set driver
		for (size_t i = 0; i < extension_num; i++) {
			if (strcmp(ext,ogrextension_list[i])==0) {
				index=i; //get the index where extension of the outputfile matches with the extensionlist 
				break;
			}
		}
		
	}


	 hDriver1 = OGRGetDriverByName( ogrdriver_code[index] );
    if( hDriver1 == NULL )
    {
        printf( "%s driver not available.\n", ogrdriver_code[index]);
        exit( 1 );
    }
	if (strcmp(datasrc,datasrcnew)!=0){
	  hDS3 = OGR_Dr_CreateDataSource( hDriver1, datasrcnew, NULL);
    if( hDS3 == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }

	}
	else {
		hDS3=hDS1;}
   

    hLayer3 = OGR_DS_CreateLayer( hDS3, layernamenew, NULL, wkbPoint, NULL );
    if( hLayer3 == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }


  
	for(int i=0;i<nxy;++i){

	
     double x1 = x[i];  // DGT says does not need +pdx/2.0;
	 double y1 = y[i];  // DGT +pdy/2.0;
	 OGRFeatureH hFeature3;
	 OGRGeometryH hPt3;

	 hFeature3 = OGR_F_Create( OGR_L_GetLayerDefn( hLayer3 ) );
	 //OGR_F_SetFieldString( hFeature3, OGR_F_GetFieldIndex(hFeature3, "Name"), "ID");

	 hPt3 = OGR_G_CreateGeometry(wkbPoint);
	 OGR_G_SetPoint_2D(hPt3, 0, x1, y1);

	 OGR_F_SetGeometry( hFeature3, hPt3 ); 
	 OGR_G_DestroyGeometry(hPt3);

	 if( OGR_L_CreateFeature( hLayer3, hFeature3 ) != OGRERR_NONE )
	 {
		printf( "Failed to create feature in shapefile.\n" );
		exit( 1 );
	 }

	 OGR_F_Destroy( hFeature3 );

	}


	 OGR_DS_Destroy( hDS3 );



	return 0;

}