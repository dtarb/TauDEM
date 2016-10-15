/*  Taudem common library functions 

David Tarboton, Kim Schreuders, Dan Watson
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

#include <mpi.h>
#include <stdio.h>
#include <memory>
#include "gdal.h"
#include "ogrsf_frmts.h"
#include "ogr_api.h"
// #include "tiffIO.h"  Part of commonLib.h
#include <ogr_spatialref.h>
#include <math.h>
#include "commonLib.h"
#include <iostream>
using namespace std;

tiffIO::tiffIO(char *fname, DATA_TYPE newtype) {
	MPI_Status status;
	MPI_Offset mpiOffset;

	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);

	strcpy(filename, fname); // Copy file name
	datatype = newtype;

	GDALAllRegister();
	fh = GDALOpen(filename, GA_ReadOnly);
	if (fh == NULL) {
		printf("Error opening file %s.\n", fname);
		MPI_Abort(MCW, 21);
	}
	hDriver = GDALGetDatasetDriver( fh );

    //OGRSpatialReferenceH  hSRS;
	char  *pszProjection;
	pszProjection = (char *) GDALGetProjectionRef( fh );
	hSRS = OSRNewSpatialReference(pszProjection);
    IsGeographic=OSRIsGeographic(hSRS);
	if (IsGeographic ==0) {
		if(rank == 0)printf("Input file %s has projected coordinate system.\n",fname);
	}
	else
		if(rank == 0)printf("Input file %s has geographic coordinate system.\n",fname);
    // cout<<getproj<<endl; // for test
	char *test_unit=NULL;
	double ss;
	ss=OSRGetLinearUnits(hSRS,&test_unit); // provide linear units
	//cout<<ss<<endl;// for test
	bandh = GDALGetRasterBand(fh, 1);
	valueUnit=GDALGetRasterUnitType(fh); // provide value units
	//cout<<valueUnit<<endl; // for test

	totalX = GDALGetRasterXSize(fh);
	totalY = GDALGetRasterYSize(fh);
	double adfGeoTransform[6];
	GDALGetGeoTransform(fh, adfGeoTransform);
    dlon = abs(adfGeoTransform[1]); //modified by Nazmus 02/1/15
	dlat = abs(adfGeoTransform[5]);
	xleftedge = adfGeoTransform[0]; // geo-coordinate
	ytopedge = adfGeoTransform[3];  
	xllcenter=xleftedge+dlon/2.;
	yllcenter=ytopedge-(totalY*dlat)-dlat/2.;

//	double  xllcenter_g = adfGeoTransform[0] + adfGeoTransform[1] * (totalX / 2.0) + adfGeoTransform[2] * (totalY / 2.0);//geo-coordinate
//	double  yllcenter_g = adfGeoTransform[3] + adfGeoTransform[4] * (totalX / 2.0) + adfGeoTransform[5] * (totalY / 2.0);

	
	//xleftedge= adfGeoTransform[0];
	//double *ytopedge_p, *yllcenter_p;
	//ytopedge_p = geotoLength(dlon,dlat,ytopedge_g);
	//xllcenter = adfGeoTransform[0] + adfGeoTransform[1] * (totalX / 2.0) + adfGeoTransform[2] * (totalY / 2.0);
	//yllcenter_p = geotoLength(dlon,dlat,yllcenter_g);
	
	
	//double beta1 = atan(boa*tan(PI*ytopedge_g/180));
	//double beta2 = atan(boa*tan(PI*yllcenter_g/180));

	//xleftedge=elipa*cos(beta1);// convert cartesian coordinate
	//ytopedge=elipb*sin(beta1);  

	//xllcenter= elipa*cos(beta2); // convert cartesian coordinate
	//yllcenter=elipb*sin(beta2);

	int i,j;double xp3,Xp1,Yp1;
	double xp2[2];
	dxc = new double[totalY];	
	dyc = new double [totalY];
	if (IsGeographic ==1) 
	{
	//for(i=0; i<totalX; i++ ) {
		for( j=0;j<totalY;j++){
			// latitude corresponding to row
			float rowlat = yllcenter+(totalY-j-1)*dlat;
			//Xp1 = adfGeoTransform[0] + i*adfGeoTransform[1] + j*adfGeoTransform[2];
			//Yp1= adfGeoTransform[3] + adfGeoTransform[4] + j*adfGeoTransform[5];
			//printf("%f",Yp1);
			geotoLength(dlon,dlat,rowlat,xp2);
			dxc[j]=xp2[0];
			//printf("%f",dxc[j]);
			dyc[j]=xp2[1];
			//printf("%f",dyc[j]);
		}
	//}
	}
	else
	{
		for( j=0;j<totalY;j++)
		{
			dxc[j]=dlon;
			dyc[j]=dlat;
		}
	}


	

	//dxA=(dxc[totalY/2]<0.0) ? -dxc[totalY/2] : dxc[totalY/2] ;   //abs(dxc[totalY/2]);  //  DGT This is ugly but we encountered a compiler that the abs function rounded the results which introduced a bug
	//dyA=(dyc[totalY/2]<0.0) ? -dyc[totalY/2] : dyc[totalY/2] ;  //abs(dyc[totalY/2]);
	 dxA=fabs(dxc[totalY/2]);
        dyA= fabs(dyc[totalY/2]);
       datatype = newtype;
	if (datatype == SHORT_TYPE) {
		nodata = new short;
		*((short*) nodata) = (short) GDALGetRasterNoDataValue(bandh, NULL);		

	} else if (datatype == FLOAT_TYPE) {
		nodata = new float;
		*((float*) nodata) = (float) GDALGetRasterNoDataValue(bandh, NULL);		

	} else if (datatype == LONG_TYPE) {
		nodata = new int32_t;
		*((int32_t*) nodata) = (int32_t) GDALGetRasterNoDataValue(bandh, NULL);		

	}

}

//Copy constructor.  Requires datatype in addition to the object to copy from.

tiffIO::tiffIO(char *fname, DATA_TYPE newtype, void* nd, const tiffIO &copy) {
	//MPI_Status status;
	//MPI_Offset mpiOffset;

	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);

	isFileInititialized = 0;

	strcpy(filename, fname); // Copy file name
	if (rank == 0)
		copyfh = copy.fh;

	
	datatype = newtype;
	if (datatype == SHORT_TYPE) {
		nodata = new short;
		*((short*) nodata) = *((short*) nd);
	} else if (datatype == FLOAT_TYPE) {
		nodata = new float;
		*((float*) nodata) = *((float*) nd);
	} else if (datatype == LONG_TYPE) {
		nodata = new int32_t;
		*((int32_t*) nodata) = *((int32_t*) nd);
	}

	totalX = copy.totalX;
	totalY = copy.totalY;
	dxA=copy.dxA;
	dyA=copy.dyA;
	xllcenter = copy.xllcenter;
	yllcenter = copy.yllcenter;
	xleftedge = copy.xleftedge;
	ytopedge = copy.ytopedge;
	dlon=copy.dlon;
	dlat=copy.dlat;
	//note: is it necessary to get these values in writing???
	dxc = new double[totalY];	
	dyc = new double [totalY];
    int i;
	for(i=0; i<totalY; i++ ) {
		dxc[i] = copy.dxc[i];
		dyc[i] = copy.dyc[i];
	}
	
}

tiffIO::~tiffIO() {

	delete dxc;
	delete dyc;
}

//Read tiff file data/image values beginning at xstart, ystart (gridwide coordinates) for the numRows, and numCols indicated to memory locations specified by dest
//BT void tiffIO::read(unsigned long long xstart, unsigned long long ystart, unsigned long long numRows, unsigned long long numCols, void* dest) {

void tiffIO::read(long xstart, long ystart, long numRows, long numCols, void* dest) {
	//cout << "read: " << xstart << " " << ystart << " " << numRows << " " << numCols << endl;
	GDALDataType eBDataType;
		if (datatype == FLOAT_TYPE)
			eBDataType = GDT_Float32;
		else if (datatype == SHORT_TYPE)
			eBDataType = GDT_Int16;
		else if (datatype == LONG_TYPE)
			eBDataType = GDT_Int32;

	GDALRasterIO(bandh, GF_Read, xstart, ystart, numCols, numRows,
		dest, numCols, numRows,eBDataType ,
		0, 0);
}

//Create/re-write tiff output file
//BT void tiffIO::write(unsigned long long xstart, unsigned long long ystart, unsigned long long numRows, unsigned long long numCols, void* source) {

void tiffIO::write(long xstart, long ystart, long numRows, long numCols, void* source) {
	MPI_Status status;
	fflush(stdout);
	char **papszMetadata;
	char **papszOptions = NULL;
	const char *extension_list[6] = {".tif",".img",".sdat",".bil",".bin",".tiff"};  // extension list --can add more 
	const char *driver_code[6] = {"GTiff","HFA","SAGA","EHdr","ENVI","GTiff"};   //  code list -- can add more
	const char *compression_meth[6] = {"LZW","YES"," "," "," "," "};   //  code list -- can add more
	size_t extension_num=6;
	char *ext; 
	int index = -1; 

	// get extension  of the file 
	ext = strrchr(filename, '.'); 
	if(!ext){
		strcat(filename,".tif");
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
			if (strcmp(ext,extension_list[i])==0) {
				index=i; //get the index where extension of the outputfile matches with the extensionlist 
				break;
			}
		}
		if(index < 0)  // Extension not matched so set it to tif
		{
			char filename_withoutext[MAXLN]; // layer name is file name without extension
			size_t len = strlen(filename);
			size_t len1 = strlen(ext+1);
			memcpy(filename_withoutext, filename, len-len1);
			filename_withoutext[len - len1] = 0; 
			strcpy(filename,filename_withoutext);
			strcat(filename,"tif");
			index=0;
		}
	}
	if (rank == 0) {
		//if (isFileInititialized == 0) {
			hDriver = GDALGetDriverByName(driver_code[index]);
		    if (hDriver == NULL) {
		        printf("driver is not available\n");
				MPI_Abort(MPI_COMM_WORLD, 22);
			}
			// Set options
			if(index==0){  // for .tif files.  Refer to http://www.gdal.org/frmt_gtiff.html for GTiff options.
				papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", compression_meth[index]); 
			}
			else if(index==1){ // .img files.  Refer to http://www.gdal.org/frmt_hfa.html where COMPRESSED = YES are create options for ERDAS .img files
				papszOptions = CSLSetNameValue( papszOptions, "COMPRESSED", compression_meth[index]);
			}
			int cellbytes=4;	
			if (datatype == SHORT_TYPE)cellbytes=2;
			double fileGB=(double)cellbytes*(double)totalX*(double)totalY/1000000000.0;  // This purposely neglects the lower significant digits to overvalue GB to allow space for header information in the file
			if(fileGB > 4.0){
				if(index==0 || index==6){  // .tiff files.  Need to explicity indicate BIGTIFF.  See http://www.gdal.org/frmt_gtiff.html.
					papszOptions = CSLSetNameValue( papszOptions, "BIGTIFF", "YES");
					printf("Setting BIGTIFF, File: %s, Anticipated size (GB):%.2f\n", filename,fileGB);
				}
			}

			GDALDataType eBDataType;
			if (datatype == FLOAT_TYPE)
				eBDataType = GDT_Float32;
			else if (datatype == SHORT_TYPE)
			{
				eBDataType = GDT_Int16;
			}
			else if (datatype == LONG_TYPE)
				eBDataType = GDT_Int32;
	        
   
			fh = GDALCreate(hDriver, filename, totalX , totalY, 1, eBDataType, papszOptions);
			GDALSetProjection(fh, GDALGetProjectionRef(copyfh));

			double adfGeoTransform[6];
			GDALGetGeoTransform(copyfh, adfGeoTransform);

			GDALSetGeoTransform(fh, adfGeoTransform);

			bandh = GDALGetRasterBand(fh, 1);
			if (datatype == FLOAT_TYPE) 
	             GDALSetRasterNoDataValue(bandh, (double) *((float*) nodata));
			else if (datatype == SHORT_TYPE)			
				GDALSetRasterNoDataValue(bandh, (double) *((short*) nodata));			
			else if (datatype == LONG_TYPE)		
				GDALSetRasterNoDataValue(bandh, (double) *((int32_t*) nodata));

			//isFileInititialized = 1;
		//} else {
			//  Open file if it has already been initialized
			//fh = GDALOpen(filename, GA_Update);
			//bandh = GDALGetRasterBand(fh, 1);
		//}
		//  Now write the data from rank 0 and close the file
		//GDALDataType eBDataType;
		//if (datatype == FLOAT_TYPE)
			//eBDataType = GDT_Float32;
		//else if (datatype == SHORT_TYPE)
			//eBDataType = GDT_Int16;
		//else if (datatype == LONG_TYPE)
			//eBDataType = GDT_Int32;

		GDALRasterIO(bandh, GF_Write, xstart, ystart, numCols, numRows,
			source, numCols, numRows, eBDataType,
			0, 0);
		
		GDALFlushCache(fh);  //  DGT effort get large files properly written
		GDALClose(fh);

		//  Send message to rank 1
		int d = 0;
		if (size > rank + 1){
			//     buffer, count, datatype, dest, tag, comm
			MPI_Send(&d, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
			//printf("Written rank: %d\n", rank);
			fflush(stdout);
		}

	} else {
		int d = 0;
		//   buffer, count, datatype, source, tag, com, status
		MPI_Recv(&d, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &status);  //DGT check status to see that receive was correct.  Print status and rank
		
		int number_amount;
		MPI_Get_count(&status, MPI_INT, &number_amount);
        //printf("Triggered in rank: %d Received %d\n", rank,number_amount);
		fflush(stdout);

		//  Once message received open file and write the data from rank > 0 then close
		fh = GDALOpen(filename, GA_Update);
		bandh = GDALGetRasterBand(fh, 1);

		GDALDataType eBDataType;
		if (datatype == FLOAT_TYPE)
			eBDataType = GDT_Float32;
		else if (datatype == SHORT_TYPE)
			eBDataType = GDT_Int16;
		else if (datatype == LONG_TYPE)
			eBDataType = GDT_Int32;

		GDALRasterIO(bandh, GF_Write, xstart, ystart, numCols, numRows,
			source, numCols, numRows, eBDataType,
			0, 0);

		GDALFlushCache(fh);  //  DGT effort get large files properly written
		GDALClose(fh);	

		//  send message to next rank up only if not last one
		if (size > rank + 1){
			//     buffer, count, datatype, dest, tag, comm
			MPI_Send(&d, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD);
			//printf("Written rank: %d\n", rank);
			fflush(stdout);
		}
	}
}





void tiffIO::geotoLength(double dlon,double dlat, double lat, double *xyc){
	double ds2,beta,dbeta;
	
  dlat=dlat*PI/180.;
  dlon=dlon*PI/180.;
  lat=lat*PI/180.;
  beta = atan(boa*tan(lat));
  dbeta=dlat*boa*(cos(beta)/cos(lat))*(cos(beta)/cos(lat));
  ds2=(pow(elipa*sin(beta),2)+pow(elipb*cos(beta),2))*pow(dbeta,2);
  xyc[0]=elipa*cos(beta)*abs(dlon);
  xyc[1]=double(sqrt(double(ds2)));    
}



bool tiffIO::compareTiff(const tiffIO &comp) {
	double tol = 0.0001;
	if (totalX != comp.totalX) {
		printf("Columns do not match: %d %d\n", totalX, comp.totalX);
		return false;
	}
	if (totalY != comp.totalY) {
		printf("Rows do not match: %d %d\n", totalY, comp.totalY);
		return false;
	}
	if (abs(dxA - comp.dxA) > tol) {
		printf("dx does not match: %lf %lf\n", dxA, comp.dxA);
		return false;
	}
	if (abs(dyA - comp.dyA) > tol) {
		printf("dy does not match: %lf %lf\n", dyA, comp.dyA);
		return false;
	}
	if (abs(xleftedge - comp.xleftedge) > 0.0) {
		if (rank == 0) {
			printf("Warning! Left edge does not match exactly:\n");
			printf(" %lf in file %s\n", xleftedge, filename);
			printf(" %lf in file %s\n", comp.xleftedge, comp.filename);
		}
		//return false;  //DGT decided to relax this to a warning as some TIFF files seem to use center or corner to reference edges in a way not understood 
	}
	if (abs(ytopedge - comp.ytopedge) > 0.0) {
		if (rank == 0) {
			printf("Warning! Top edge does not match exactly:\n");
			printf(" %lf in file %s\n", ytopedge, filename);
			printf(" %lf in file %s\n", comp.ytopedge, comp.filename);
		}
		//return false;  //DGT decided to relax this to a warning as some TIFF files seem to use center or corner to reference edges in a way not understood 
	}
	// 6/25/10.  DGT decided to not check spatial reference information as tags may be
	//  in different orders and may include comments that result in warnings when differences
	//  are immaterial.  Rather we rely on leftedge and topedge comparisons to catch projection
	//  differences as if the data is in a different projection and really different it would be
	//  highly coincidental for the leftedge and topedge to coincide to within tol

	//  Check spatial reference information
	//bool warning = false;
	//if(filedata.geoAsciiSize != comp.filedata.geoAsciiSize || 
	//	filedata.geoKeySize != comp.filedata.geoKeySize) warning=true;
	//else
	//{
	//	if(filedata.geoAsciiSize > 0)
	//		if(strncmp(filedata.geoAscii,comp.filedata.geoAscii,filedata.geoAsciiSize)!=0)
	//			warning=true;
	//	if(filedata.geoKeySize > 0)
	//	{
	//		for(long i=0; i< filedata.geoKeySize; i++)
	//			if(filedata.geoKeyDir[i]!=comp.filedata.geoKeyDir[i])
	//				warning=true;
	//	}
	//}
	//if(warning)
	//{
	//	if(rank == 0){
	//		printf("Warning:  Spatial references different.  Results may not be correct.\n");
	//		printf("File 1: %s\n",filename);
	//		if(filedata.geoAsciiSize>0)
	//			printf("  %s\n",filedata.geoAscii);
	//		else
	//			printf("  Unknown spatial reference\n");
	//		if(filedata.geoKeySize>0)
	//		{
	//			printf("  Projection Params:\n  ");
	//			for(long i=0; i< filedata.geoKeySize; i++)
	//				printf("%d,",filedata.geoKeyDir[i]);
	//			printf("\n");
	//		}
	//		else
	//			printf("  Unknown projection parameters\n");

	//		printf("File 2: %s\n",comp.filename);
	//		if(comp.filedata.geoAsciiSize>0)
	//			printf("  %s\n",comp.filedata.geoAscii);
	//		else
	//			printf("  Unknown spatial reference\n");
	//		if(comp.filedata.geoKeySize>0)
	//		{
	//			printf("  Projection Params:\n  ");
	//			for(long i=0; i< comp.filedata.geoKeySize; i++)
	//				printf("%d,",comp.filedata.geoKeyDir[i]);
	//			printf("\n");
	//		}
	//		else
	//			printf("  Unknown projection parameters\n");
	//	}
	//}
	return true;
}

/*void tiffIO::readIfd(ifd &obj) {
MPI_Status status;
MPI_File_read(fh, &obj.tag, 2, MPI_BYTE, &status);
MPI_File_read(fh, &obj.type, 2, MPI_BYTE, &status);
MPI_File_read(fh, &obj.count, 4, MPI_BYTE, &status);
MPI_File_read(fh, &obj.offset, 4, MPI_BYTE, &status);
}

void tiffIO::writeIfd(ifd &obj) {
MPI_Status status;
MPI_File_write(fh, &obj.tag, 2, MPI_BYTE, &status);
MPI_File_write(fh, &obj.type, 2, MPI_BYTE, &status);
MPI_File_write(fh, &obj.count, 4, MPI_BYTE, &status);
MPI_File_write(fh, &obj.offset, 4, MPI_BYTE, &status);
}*/

//void tiffIO::printIfd(ifd obj) {
//	printf("Tag: %hu\n", obj.tag);
//	printf("Type: %hu\n", obj.type);
//	printf("Value: %u\n", obj.count);
//	printf("offset: %u\n", obj.offset);
//}

//BT void tiffIO::geoToGlobalXY(double geoX, double geoY, unsigned long long &globalX, unsigned long long &globalY){
//BT unsigned long x0 = (unsigned long)(xllcenter - dx/2);
//BT unsigned long y0 = (unsigned long)(yllcenter - dy/2);
//BT globalX = (unsigned long)((geoX - x0) / dx);
//BT globalY = (unsigned long)((y0 - geoY) / dy);

//void tiffIO::geoToGlobalXY_real(double geoX, double geoY, int &globalX, int &globalY) {
	//Original version had dx's intead of dy's - in case of future problems
	//	int x0 = (int)(xllcenter - dx/2);
	//	int y0 = (int)(yllcenter + dy/2);  // yllcenter actually is the y coordinate of the center of the upper left grid cell 
//	globalX = (int) ((geoX - xleftedge)/dxA);
//	globalY = (int) ((ytopedge-geoY)/dyA);
//}

void tiffIO::geoToGlobalXY(double geoX, double geoY, int &globalX, int &globalY) {
	//  input geoX and GeoY are coordinates in the geographic or projected reference system. 
	//  This returns the corresponding row and column in the array.  
	//  The function is the same for geographic and projected coordinates 

	globalX = (int)((geoX - xleftedge) / dlon);
	globalY = (int)((ytopedge - geoY) / dlat);

}


//BT void tiffIO::globalXYToGeo(unsigned long long globalX, unsigned long long globalY, double &geoX, double &geoY){

void tiffIO::globalXYToGeo(long globalX, long globalY, double &geoX, double &geoY) {
	
	geoX = xleftedge + dlon / 2. + globalX*dlon;
	geoY = ytopedge - dlat / 2. - globalY*dlat;
}

