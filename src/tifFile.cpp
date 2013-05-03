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
#include <stdlib.h>
#include <string.h>
#include <memory>
#include "tifFile.h"

#include <iostream>
using namespace std;

tifFile::tifFile(char *fname, DATA_TYPE newtype){
	MPI_Status status;
	MPI_Offset mpiOffset;

	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);

	int totalbuf;
	char* buffer;

	int file_error = MPI_File_open( MCW, fname, MPI_MODE_RDONLY , MPI_INFO_NULL, &fh);
	if( file_error != MPI_SUCCESS) { 
		printf("Error opening file %s.\n", fname);
		printf("TauDEM input files have to be GeoTiff files.\n");
		MPI_Abort(MCW,21);
	}
	strcpy(filename,fname);  // Copy file name

	if (rank == 0) {
	//printf("newtype: %d.\n", newtype);

			
	//Generate datatype constants
	datatype = newtype;
	if( datatype == SHORT_TYPE ) {
		dataSizeObj = sizeof (short);
	}
	else if( datatype == LONG_TYPE) {
		dataSizeObj = sizeof (long);
	}
	else if( datatype == FLOAT_TYPE) {
		dataSizeObj = sizeof (float);
	}
//	printf("dataSizeObj: %d.\n", dataSizeObj);

	//Read byte order word
	short endian;
	MPI_File_read( fh, &endian, 2, MPI_BYTE, &status);
	if( endian != LITTLEENDIAN ) {
		printf("File or machine not in correct endian form. (%d)!=18761\n",endian);
		MPI_Abort(MCW,-1);
	}

	//Read the TIFF version word
	MPI_File_read( fh, &version, 2, MPI_BYTE, &status);

	//Process the remiander of the file header based on version type
	if ( version == TIFF ) {
		//Read first IFD offset
		//KATS does not read right with unsigned long even though the data is really an unsigned long!
		uint32_t offset;
		MPI_File_read( fh, &offset, 4, MPI_BYTE, &status);
		mpiOffset = offset;
		MPI_File_seek( fh, offset, MPI_SEEK_SET );
	}
	else if ( version == BIGTIFF ) {
	
		//Does not properly handle BIGTIFF right now, so throw error
		printf("Not a geoTiff file %s\n", fname);
		printf("TauDEM input files have to be GeoTiff files.\n");
		printf("The BigTiff format for files greater than 4GB is not yet supported.\n");
		MPI_Abort(MCW,-1);

		//Read offset bytesize
		short offsetByteSize;
		MPI_File_read( fh, &offsetByteSize, 2, MPI_BYTE, &status);
		if ( offsetByteSize != 8 ) {
			printf("BIGTIFF file must have an offset bytesize of 8. (%d)!=18761\n",endian);
			MPI_Abort(MCW,-1);
		}
		//Read empty word in BIGTIFF
		short emptyWord;
		MPI_File_read( fh, &emptyWord, 2, MPI_BYTE, &status);
		if ( emptyWord != 0 ) {
			printf("BIGTIFF file must have a value of 0 for the 4th word. (%d)!=18761\n",endian);
			MPI_Abort(MCW,-1);
		}
		//Read first IFD offset
		unsigned long long offset8;
		MPI_File_read( fh, &offset8, 8, MPI_BYTE, &status);
		mpiOffset = offset8;
		MPI_File_seek( fh, offset8, MPI_SEEK_SET );
	}
	else {
		printf("Error opening file %s.\n", fname);
		printf("File verion not TIFF or BIGTIFF. (%d)!=18761\n",endian);
		printf("TauDEM input files have to be GeoTiff files.\n");
		MPI_Abort(MCW,-1);
	}

	//Read IFD Header
	if ( version == TIFF ) {
		MPI_File_read( fh, &numEntries, 2, MPI_BYTE, &status);
	}
	else {
		MPI_File_read( fh, &numEntries, 8, MPI_BYTE, &status);
	}

	//ifds = ( ifd*) malloc( sizeof(ifd) * numEntries );
	ifds = new ifd[numEntries];
	//BT unsigned long index;
	long index;
	for( index=0; index<numEntries; ++index ) {
        readIfd(ifds[index]);
		//printIfd( ifds[index] );
	}

	//Set tiff values to defaults
	//datatype = UNKNOWN_TYPE;
	int noDataDef = 0;
	long rasterTypeIndex = 0;
	int origRasterType = 0;
	tileOrRow = 0;
	
	filedata.geoKeySize =0;
	filedata.geoDoubleSize=0;
	filedata.geoAsciiSize=0;
	filedata.gdalAsciiSize=0;

	for( index=0;index<numEntries;++index ) {
		//Reference http://www.awaresystems.be/imaging/tiff/tifftags/extension.html
		switch( ifds[index].tag ) {
		case 256: //ImageWidth, unsigned short or long
			//BT totalX = (unsigned long) (ifds[index].offset);
			totalX = ifds[index].offset;
			break;
		case 257: //ImageLength, unsigned short or long
			//BT totalY = (unsigned long) (ifds[index].offset);
			totalY = ifds[index].offset;
			break;
		case 258: //BitsPerSample, unsigned short
			if( ifds[index].offset % 8 != 0 ) {
				printf("Error opening file %s.\n", fname);
				printf("Datasize is %d bits. Must be multiple of 8 (a byte)\n",ifds[index].offset);	
				printf("TauDEM input files have to be GeoTiff files.\n");
				MPI_Abort(MCW,4);
			}
			//BT dataSize = (unsigned short) (ifds[index].offset/8); //always an unsigned short
			dataSizeFileIn = (unsigned short) (ifds[index].offset/8); //always an unsigned short
			break;
		case 259: //Compression, unsigned short
			if( ifds[index].offset != 1 ) {
				printf("Error opening file %s.\n", fname);
				printf("Tiff file compressed. Unable to process compressed tiff files.\n");
				MPI_Abort(MCW,-4);
			}
			break;
		case 273: //StripOffsets, unsigned short or long
			tileOrRow = 2;
			//BT numOffsets = (unsigned long) (ifds[index].count);
			//BT offsets = (unsigned long long*) malloc( 4*ifds[index].count);
			numOffsets = ifds[index].count;
			//SizeOf offsets = (long*) malloc( 4*ifds[index].count);
			offsets = (uint32_t*) malloc(sizeof(uint32_t)*ifds[index].count);  //DGT
			//if the stripoffset data is within the 4 bytes stored within the IFD
			if( ifds[index].count == 1)
				offsets[0] = ifds[index].offset;
			//else use those 4 bytes to find the actual stripoffset data
			else {
				mpiOffset = ifds[index].offset;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				//BT for(unsigned long long i=0; i<ifds[index].count;++i) {
				for(unsigned long i=0; i<ifds[index].count;++i) {
  					MPI_File_read( fh, &offsets[i], 4, MPI_BYTE, &status);
				}
			}
			break;
		case 277: //SamplesPerPixel, unsigned short
			if( ifds[index].offset != 1 ) {
				printf("Error opening file %s.\n", fname);
				printf("Samples per pixel not equal to one. Unable to interpret tiff\n");
				MPI_Abort(MCW,-5);
			}
			break;
		case 278: //RowsPerStrip, unsigned short or long or long long
			//BT tileLength= (unsigned long) (ifds[index].offset);
			tileLength = ifds[index].offset;
			tileWidth = totalX;
			break;
		case 282: //XResolution, rational
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_read( fh, &filedata.xresNum, 4, MPI_BYTE, &status);
			MPI_File_read( fh, &filedata.xresDen, 4, MPI_BYTE, &status);
			break;
		case 283: //YResolution, rational
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_read( fh, &filedata.yresNum, 4, MPI_BYTE, &status);
			MPI_File_read( fh, &filedata.yresDen, 4, MPI_BYTE, &status);
			break;
		case 284: //PlanarConfiguration, unsigned short
			//BT filedata.planarConfig = (unsigned short) (ifds[index].offset);
			filedata.planarConfig = (unsigned short) (ifds[index].offset);
			break;
		case 322: //TileWidth, unsigned short or long
			//BT tileWidth = (unsigned long) (ifds[index].offset);
			tileWidth = ifds[index].offset;
			break;
		case 323: //TileLength, unsigned short or long
			//BT tileLength = (unsigned long) (ifds[index].offset);
			tileLength = ifds[index].offset;
			break;
		case 324: //TileOffsets, unsigned long
			tileOrRow = 1;
			//BT numOffsets = (unsigned long) (ifds[index].count);
			//BT offsets = (unsigned long long*) malloc( 4 * ifds[index].count );
			numOffsets = ifds[index].count;
			//SizeOf offsets = (long*) malloc( 4*ifds[index].count);
			offsets = (uint32_t*) malloc(sizeof(uint32_t)*ifds[index].count);  //DGT
			if( ifds[index].count == 1 )
				offsets[0] = ifds[index].offset;
			else {
				mpiOffset = ifds[index].offset;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				//BT for( unsigned long i=0; i<ifds[index].count; ++i ) {
				for( unsigned long i=0; i<ifds[index].count; ++i ) {
					MPI_File_read( fh, &offsets[i],4,MPI_BYTE, &status);
				}
			}
			break;
		case 279: //StripByteCounts, unsigned short or long
		case 325: //TileByteCounts, unsigned short or long
			//BT bytes = (unsigned long*) malloc( 4*ifds[index].count);
			//SizeOf bytes = (long*) malloc( 4*ifds[index].count);
			bytes = (uint32_t*) malloc(sizeof(uint32_t)*ifds[index].count);  //DGT
			bytesize = ifds[index].count;
			if( ifds[index].count == 1 )
				bytes[0] = ifds[index].offset;
			else if( ifds[index].type == 3 ) {
				short s;
				//BT for( unsigned long i=0; i<ifds[index].count; ++i) {
				for( unsigned long i=0; i<ifds[index].count; ++i) {
					MPI_File_read( fh, &s, 2, MPI_BYTE,&status);
					bytes[i] = s;
				}
			}
			else if( ifds[index].type == 4) {
				//BT for( unsigned long i=0; i<ifds[index].count; ++i) {
				for( unsigned long i=0; i<ifds[index].count; ++i) {
					MPI_File_read( fh, &bytes[i], 4, MPI_BYTE,&status);
				}
			}
			break;
		case 339: //SampleFormat, unsigned short, 1=unsigned integer, 2=signed integer, 3=float, 4=undefined
			if( ifds[index].count > 1) {
				printf("Error opening file %s.\n", fname);
				printf("Samples per pixel larger than one. Unable to interpret tiff\n");
				MPI_Abort(MCW,-6);
			}
			sampleFormat = (short) ifds[index].offset;
			break;
		case 33550: //GeoTIFF-ModelPixelScaleTag
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_read( fh, &dx, 8, MPI_BYTE, &status );
			MPI_File_read( fh, &dy, 8, MPI_BYTE, &status );
			break;
		case 33922: //GeoTIFF-ModelTiepointTag
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			double tempDouble;
			MPI_File_read( fh, &tempDouble, 8, MPI_BYTE, &status );
			//printf("%f\t",tempDouble);
			MPI_File_read( fh, &tempDouble, 8, MPI_BYTE, &status );
			//printf("%f\t",tempDouble);
			MPI_File_read( fh, &tempDouble, 8, MPI_BYTE, &status );
			//printf("%f\n",tempDouble);
			MPI_File_read( fh, &xleftedge, 8, MPI_BYTE, &status );
			MPI_File_read( fh, &ytopedge, 8, MPI_BYTE, &status );   
			break;
		case 34735: //GeoTIFF-GeoKeyDirectoryTag
			filedata.geoKeySize = ifds[index].count;
			//BT filedata.geoKeyDir = (unsigned short *) malloc( 2*filedata.geoKeySize);
			//SizeOf filedata.geoKeyDir = (short *) malloc( 2*filedata.geoKeySize);
			filedata.geoKeyDir = (uint16_t *) malloc( sizeof(uint16_t) * filedata.geoKeySize);
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			//BT for( unsigned long i=0; i<filedata.geoKeySize; ++i) {
			for( long i=0; i<filedata.geoKeySize; ++i) {
				MPI_File_read( fh, &filedata.geoKeyDir[i], 2, MPI_BYTE, &status);
				if (( i >= 4 ) && ( i % 4 == 0 ) && ( filedata.geoKeyDir[i] == 1025 )) {
					rasterTypeIndex = i;
				}
				//printf("%d,",filedata.geoKeyDir[i]);
			}
			if ( rasterTypeIndex > 0 ) {
				origRasterType = filedata.geoKeyDir[rasterTypeIndex + 3];
				filedata.geoKeyDir[rasterTypeIndex + 3] = 1;
			}
			//printf("\n");
			break;
		case 34736: //GeoTIFF-GeoDoubleParamsTag
			filedata.geoDoubleSize = ifds[index].count;
			//SizeOf filedata.geoDoubleParams = (double*) malloc( 8*filedata.geoDoubleSize );
			filedata.geoDoubleParams = (double*) malloc( sizeof(double) * filedata.geoDoubleSize );
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			//BT for( unsigned long i=0; i<filedata.geoDoubleSize; ++i) {
			for( long i=0; i<filedata.geoDoubleSize; ++i) {
				MPI_File_read( fh, &filedata.geoDoubleParams[i], 8, MPI_BYTE, &status);
			}
			break;
		case 34737: //GeoTIFF-GeoAsciiParamsTag
			//TODO Fix Type Cast of ifds[index].count
			filedata.geoAsciiSize = ifds[index].count;  
			//SizeOf filedata.geoAscii = (char*) malloc( filedata.geoAsciiSize );
			filedata.geoAscii = (char*) malloc( sizeof(char) * (filedata.geoAsciiSize));  
			if( filedata.geoAscii == NULL ) {
				printf("Error opening file %s.\n", fname);
				printf("Memory allocation error.\n");
				MPI_Abort(MCW,-9);
			}
			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			//BT MPI_File_read( fh, filedata.geoAscii, (long) ifds[index].count, MPI_BYTE, &status);
			MPI_File_read( fh, filedata.geoAscii, ifds[index].count, MPI_BYTE, &status);
			//filedata.geoAscii[ ifds[index].count ] = '\0';
			//printf("%s\n",filedata.geoAscii);
			break;	
		case 42112: //gdalAscii
			filedata.gdalAsciiSize = ifds[index].count;
			//SizeOf filedata.gdalAscii = (char*) malloc(ifds[index].count+1);
			filedata.gdalAscii = (char*) malloc( sizeof(char) * filedata.gdalAsciiSize );
			if( filedata.gdalAscii == NULL ) {
				printf("Error opening file %s.\n", fname);
				printf("Memory allocation error.\n");
				MPI_Abort(MCW,-9);
			}

			mpiOffset = ifds[index].offset;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_read( fh, filedata.gdalAscii, (long) ifds[index].count, MPI_BYTE, &status);
//			printf("%s\n",filedata.gdalAscii);
//			filedata.gdalAscii[ ifds[index].count ] = '\0';
			break;
		case 42113: //gdalnodata
			{
			noDataDef = 1;
			double noDataDiff=0.0;
			//If 4 or less bytes of data in tag get nodata value from offset
			char *noD;
			if( ifds[index].count <=4 ) 
				noD = (char*) &ifds[index].offset;
			else{
				//SizeOf char *noD = (char*) malloc(ifds[index].count+1);
				noD = (char*) malloc( sizeof(char) * ifds[index].count+1 );
				mpiOffset = ifds[index].offset;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				//BT MPI_File_read( fh, noD, (int) ifds[index].count, MPI_BYTE, &status);
				MPI_File_read( fh, noD, ifds[index].count, MPI_BYTE, &status);
				noD[ ifds[index].count ] = '\0';
			}
			//  Set filenodata based on file data type
			//  This assumes that sampleFormat and dataSizeFileIn have been read already
			//  The conversions below using atoi are weak because return from atoi is an int.  
			//  atol could be used for long types, but there do not appear to be ascii to ...
			//  conversions for all the specific width integers
			if((sampleFormat == 1) && (dataSizeFileIn == 1)) { 
				filenodata=new uint8_t;
				filenodatasize = sizeof(uint8_t);
				*((uint8_t*)filenodata) = (uint8_t)atoi(noD);
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
				filenodata=new uint16_t;
				filenodatasize = sizeof(uint16_t);
				*((uint16_t*)filenodata) = (uint16_t)atoi(noD);
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
				filenodata=new uint32_t;
				filenodatasize = sizeof(uint32_t);
				*((uint32_t*)filenodata) = (uint32_t)atol(noD);
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
				filenodata=new uint64_t;
				filenodatasize = sizeof(uint64_t);
				*((uint64_t*)filenodata) = (uint64_t)atol(noD);
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
				filenodata = new int8_t;
				filenodatasize = sizeof(int8_t);
				*((int8_t*)filenodata) = (int8_t)atoi(noD);
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
				filenodata=new int16_t;
				filenodatasize = sizeof(int16_t);
				*((int16_t*)filenodata) = (int16_t)atoi(noD);
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
				filenodata=new int32_t;
				filenodatasize = sizeof(int32_t);
				*((int32_t*)filenodata) = (int32_t)atol(noD);
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
				filenodata=new int64_t;
				filenodatasize = sizeof(int64_t);
				*((int64_t*)filenodata) = (int64_t)atol(noD);
			} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
				filenodata=new float;
				filenodatasize = sizeof(float);
				*((float*)filenodata) = (float) atof(noD);
			} else {
				printf("Error opening file %s.\n", fname);
				printf("Unsupported TIFF file type or filetype not known when reading nodata tag.  sampleFormat = %d, dataSizeFileIn = %d.\n", sampleFormat, dataSizeFileIn);
				MPI_Abort(MCW,-1);
			}
			// Now map each of the 9 supported file data types onto the 3 supported output data types
			// Mapping ruled to handle typecasts may need adjusting.
			// TauDEM often makes the assumption that no data values are negative and less than 
			// valid data values.  Need to work in the code to do proper nodata checks rather than handling here
			// For now mapping rules are:
			// All integer flavors - leave alone, do whatever compiler does
			// Float to int or short use MISSINGSHORT if float typecast of filenodata differs from value read
			if( datatype == SHORT_TYPE ) { 
				nodata = new short;
				nodatasize = sizeof(short);
				if((sampleFormat == 1) && (dataSizeFileIn == 1)) {
					*((short*)nodata)=(short)(*((uint8_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
					*((short*)nodata)=(short)(*((uint16_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
					*((short*)nodata)=(short)(*((uint32_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
					*((short*)nodata)=(short)(*((uint64_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
					*((short*)nodata)=(short)(*((int8_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
					*((short*)nodata)=(short)(*((int16_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
					*((short*)nodata)=(short)(*((int32_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
					*((short*)nodata)=(short)(*((int64_t*)filenodata));
				} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
					*((short*)nodata)=(short)(*((float*)filenodata));
					noDataDiff = *((short*)nodata) - (*((float*)filenodata));
					if(noDataDiff > 1e-6) (*((short*)nodata))=MISSINGSHORT;
				} 
			}
			else if( datatype == LONG_TYPE) {
				nodata = new long;
				nodatasize = sizeof(long);
				if((sampleFormat == 1) && (dataSizeFileIn == 1)) {
					*((long*)nodata)=(long)(*((uint8_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
					*((long*)nodata)=(long)(*((uint16_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
					*((long*)nodata)=(long)(*((uint32_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
					*((long*)nodata)=(long)(*((uint64_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
					*((long*)nodata)=(long)(*((int8_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
					*((long*)nodata)=(long)(*((int16_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
					*((long*)nodata)=(long)(*((int32_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
					*((long*)nodata)=(long)(*((int64_t*)filenodata));
				} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
					*((long*)nodata)=(long)(*((float*)filenodata));
					noDataDiff = *((long*)nodata) - (*((float*)filenodata));
					if(noDataDiff > 1e-6) (*((long*)nodata))=MISSINGLONG;
				} 
			}
			else if( datatype == FLOAT_TYPE) {
				nodata = new float;
				nodatasize = sizeof(float);
				if((sampleFormat == 1) && (dataSizeFileIn == 1)) {
					*((float*)nodata)=(float)(*((uint8_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
					*((float*)nodata)=(float)(*((uint16_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
					*((float*)nodata)=(float)(*((uint32_t*)filenodata));
				} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
					*((float*)nodata)=(float)(*((uint64_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
					*((float*)nodata)=(float)(*((int8_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
					*((float*)nodata)=(float)(*((int16_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
					*((float*)nodata)=(float)(*((int32_t*)filenodata));
				} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
					*((float*)nodata)=(float)(*((int64_t*)filenodata));
				} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
					*((float*)nodata)=(float)(*((float*)filenodata));
				} 
			}
			}
			break;
		default: 
			{
//				printf("Unhandled tag: %d\n",ifds[index].tag);
			}
			break;
		}
	}

	//Adjust grid for PixelIsPoint Raster Space
	if ( origRasterType==2 ) {
		xleftedge=xleftedge-dx/2.;
		ytopedge=ytopedge+dy/2.;
		if(rank==0)printf("The input file: %s\n",filename);
		if(rank==0)printf("uses the PixelIsPoint Raster Space convention.  The output files have adjusted \ntop and left edge values for the PixelIsArea default convention.\n");
		//printf("xleftedge: %f, ytopedge: %f\n", xleftedge, ytopedge);
	}

	//Compute xllcenter and yllcenter
	xllcenter=xleftedge+dx/2.;
	yllcenter=ytopedge-(totalY*dy)-dy/2.;

	//TODO - DGT please add code to also read ESRI no data value
	if( noDataDef != 1 ) {
		//printf("Error opening file %s.\n", fname);
		if(rank==0)printf("NoData not defined. Tiff tag 42113 (GDAL_NODATA) should be defined.\n");
		//  Assume values
		if(datatype==SHORT_TYPE)
		{
			nodata = new short;
			nodatasize = sizeof(short);
			if((sampleFormat == 1) && (dataSizeFileIn == 1)) { 
				filenodata=new uint8_t;
				filenodatasize = sizeof(uint8_t);
				*((uint8_t*)filenodata) = (uint8_t)255;
				*((short*)nodata)=(short)(*((uint8_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
				filenodata=new uint16_t;
				filenodatasize = sizeof(uint16_t);
				*((uint16_t*)filenodata) = (uint16_t)32767;
				*((short*)nodata)=(short)(*((uint16_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
				filenodata=new uint32_t;
				filenodatasize = sizeof(uint32_t);
				*((uint32_t*)filenodata) = (uint32_t)32767;
				*((short*)nodata)=(short)(*((uint32_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
				filenodata=new uint64_t;
				filenodatasize = sizeof(uint64_t);
				*((uint64_t*)filenodata) = (uint64_t)32767;
				*((short*)nodata)=(short)(*((uint64_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
				filenodata = new int8_t;
				filenodatasize = sizeof(int8_t);
				*((int8_t*)filenodata) = (int8_t)(-128);
				*((short*)nodata)=(short)(*((int8_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
				filenodata=new int16_t;
				filenodatasize = sizeof(int16_t);
				*((int16_t*)filenodata) = (int16_t)MISSINGSHORT;
				*((short*)nodata)=(short)(*((int16_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
				filenodata=new int32_t;
				filenodatasize = sizeof(int32_t);
				*((int32_t*)filenodata) = (int32_t)MISSINGSHORT;
				*((short*)nodata)=(short)(*((int32_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
				filenodata=new int64_t;
				filenodatasize = sizeof(int64_t);
				*((int64_t*)filenodata) = (int64_t)MISSINGSHORT;
				*((short*)nodata)=(short)(*((int64_t*)filenodata));
			} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
				filenodata=new float;
				filenodatasize = sizeof(float);
				*((float*)filenodata) = (float) MISSINGSHORT;
				*((short*)nodata)=(short)(*((float*)filenodata));
			}
			if(rank==0)printf("The value: %d will be used as representing missing data.\n",*((short*)nodata));
		}else if(datatype == LONG_TYPE)
		{
			nodata = new long;
			nodatasize = sizeof(long);
			if((sampleFormat == 1) && (dataSizeFileIn == 1)) { 
				filenodata=new uint8_t;
				filenodatasize = sizeof(uint8_t);
				*((uint8_t*)filenodata) = (uint8_t)255;
				*((long*)nodata)=(long)(*((uint8_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
				filenodata=new uint16_t;
				filenodatasize = sizeof(uint16_t);
				*((uint16_t*)filenodata) = (uint16_t)32767;
				*((long*)nodata)=(long)(*((uint16_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
				filenodata=new uint32_t;
				*((uint32_t*)filenodata) = (uint32_t)2147483647;
				*((long*)nodata)=(long)(*((uint32_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
				filenodata=new uint64_t;
				filenodatasize = sizeof(uint64_t);
				*((uint64_t*)filenodata) = (uint64_t)2147483647;
				*((long*)nodata)=(long)(*((uint64_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
				filenodata = new int8_t;
				filenodatasize = sizeof(int8_t);
				*((int8_t*)filenodata) = (int8_t)(-128);
				*((long*)nodata)=(long)(*((int8_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
				filenodata=new int16_t;
				filenodatasize = sizeof(int16_t);
				*((int16_t*)filenodata) = (int16_t)MISSINGSHORT;
				*((long*)nodata)=(long)(*((int16_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
				filenodata=new int32_t;
				filenodatasize = sizeof(int32_t);
				*((int32_t*)filenodata) = (int32_t)MISSINGLONG;
				*((long*)nodata)=(long)(*((int32_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
				filenodata=new int64_t;
				filenodatasize = sizeof(int64_t);
				*((int64_t*)filenodata) = (int64_t)MISSINGLONG;
				*((long*)nodata)=(long)(*((int64_t*)filenodata));
			} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
				filenodata=new float;
				filenodatasize = sizeof(float);
				*((float*)filenodata) = (float) MISSINGLONG;
				*((long*)nodata)=(long)(*((float*)filenodata));
			}
			if(rank==0)printf("The value: %d will be used as representing missing data.\n",*((long*)nodata));
		}
		else
		{
			nodata = new float;
			nodatasize = sizeof(float);
			if((sampleFormat == 1) && (dataSizeFileIn == 1)) { 
				filenodata=new uint8_t;
				filenodatasize = sizeof(uint8_t);
				*((uint8_t*)filenodata) = (uint8_t)255;
				*((float*)nodata)=(float)(*((uint8_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
				filenodata=new uint16_t;
				filenodatasize = sizeof(uint16_t);
				*((uint16_t*)filenodata) = (uint16_t)32767;
				*((float*)nodata)=(float)(*((uint16_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
				filenodata=new uint32_t;
				filenodatasize = sizeof(uint32_t);
				*((uint32_t*)filenodata) = (uint32_t)4294967295;
				*((float*)nodata)=(float)(*((uint32_t*)filenodata));
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 8)) {
				filenodata=new uint64_t;
				filenodatasize = sizeof(uint64_t);
				*((uint64_t*)filenodata) = ((uint64_t)(4294967296))*((uint64_t)(4294967295))+((uint64_t)(4294967295));
				*((float*)nodata)=(float)(*((uint64_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
				filenodata = new int8_t;
				filenodatasize = sizeof(int8_t);
				*((int8_t*)filenodata) = (int8_t)(-128);
				*((float*)nodata)=(float)(*((int8_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
				filenodata=new int16_t;
				filenodatasize = sizeof(int16_t);
				*((int16_t*)filenodata) = (int16_t)(MISSINGSHORT);
				*((float*)nodata)=(float)(*((int16_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
				filenodata=new int32_t;
				filenodatasize = sizeof(int32_t);
				*((int32_t*)filenodata) = (int32_t)MISSINGLONG;
				*((float*)nodata)=(float)(*((int32_t*)filenodata));
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 8)) {
				filenodata=new int64_t;
				filenodatasize = sizeof(int64_t);
				*((int64_t*)filenodata) = (int64_t)MISSINGLONG;
				*((float*)nodata)=(float)(*((int64_t*)filenodata));
			} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
				filenodata=new float;
				filenodatasize = sizeof(float);
				*((float*)filenodata) = MISSINGFLOAT;
				*((float*)nodata)=(float)(*((float*)filenodata));
			}
			if(rank==0)printf("The value: %g will be used as representing missing data.\n",*((float*)nodata));
		}
		//MPI_Abort(MCW,-5);
	}
	gystart = ytopedge / dy;
	gyend = gystart - totalY;

	gxstart = xleftedge / dx;
	gxend = gxstart + totalX;

	int geotifbufsize = (4 * sizeof(long)) + sizeof(short) + (4 * sizeof(uint32_t)) +
			(sizeof(uint16_t) * filedata.geoKeySize) + (sizeof(double) * filedata.geoDoubleSize) +
			(sizeof(char) * filedata.geoAsciiSize) + (sizeof(char) * filedata.gdalAsciiSize);
	int ifdbufsize = numEntries * (sizeof(unsigned short) + sizeof(unsigned short) + sizeof(uint32_t) + sizeof(uint32_t));
	int tifbufsize = (6 * sizeof(short)) + (10 * sizeof(uint32_t)) + (numOffsets * sizeof(uint32_t)) +
			(10 * sizeof(double)) + (3 * sizeof(int)) + (nodatasize + filenodatasize) + (sizeof(uint32_t) * bytesize) + sizeof(DATA_TYPE);

	totalbuf = tifbufsize + geotifbufsize + ifdbufsize;
	buffer = (char*) malloc(totalbuf);

	int bufi = 0;

	// copy tifFile data members except filedata and ifds
	memcpy((char*)(buffer + bufi), (char*) &dataSizeFileIn, sizeof(short));
	bufi += sizeof(short);
	memcpy((char*)(buffer + bufi), (char*) &dataSizeObj, sizeof(short));
	bufi += sizeof(short);
	memcpy((char*)(buffer + bufi), (char*) &sampleFormat, sizeof(short));
	bufi += sizeof(short);
	memcpy((char*)(buffer + bufi), (char*) &numOffsets, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) offsets, (numOffsets * sizeof(uint32_t)));
	bufi += (numOffsets * sizeof(uint32_t));
	memcpy((char*)(buffer + bufi), (char*) &tileOrRow, sizeof(short));
	bufi += sizeof(short);
	memcpy((char*)(buffer + bufi), (char*) &tileLength, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &tileWidth, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &bytesize, sizeof(int));
	bufi += sizeof(int);
	memcpy((char*)(buffer + bufi), (char*) bytes, (sizeof(uint32_t) * bytesize));
	bufi += (sizeof(uint32_t) * bytesize);
	memcpy((char*)(buffer + bufi), (char*) &numEntries, sizeof(unsigned short));
	bufi += sizeof(unsigned short);
	memcpy((char*)(buffer + bufi), (char*) &version, sizeof(short));
	bufi += sizeof(short);
	memcpy((char*)(buffer + bufi), (char*) &totalX, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &totalY, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &dx, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &dy, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &xllcenter, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &yllcenter, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &xleftedge, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &ytopedge, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &datatype, sizeof(DATA_TYPE));
	bufi += sizeof(DATA_TYPE);
	memcpy((char*)(buffer + bufi), (char*) &nodatasize, sizeof(int));
	bufi += sizeof(int);
	memcpy((char*)(buffer + bufi), (char*) &filenodatasize, sizeof(int));
	bufi += sizeof(int);
	memcpy((char*)(buffer + bufi), (char*) nodata, nodatasize);
	bufi += nodatasize;
	memcpy((char*)(buffer + bufi), (char*) filenodata, filenodatasize);
	bufi += filenodatasize;
	memcpy((char*)(buffer + bufi), (char*) &geoystart, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &geoyend, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &geoxstart, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &geoxend, sizeof(double));
	bufi += sizeof(double);
	memcpy((char*)(buffer + bufi), (char*) &gystart, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &gyend, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &gxstart, sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &gxend, sizeof(uint32_t));
	bufi += sizeof(uint32_t);

	// copy file data into buffer

	memcpy((char*)(buffer + bufi), (char*) &(filedata.xresNum), sizeof(long));
	bufi += sizeof(long);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.xresDen), sizeof(long));
	bufi += sizeof(long);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.yresNum), sizeof(long));
	bufi += sizeof(long);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.yresDen), sizeof(long));
	bufi += sizeof(long);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.planarConfig), sizeof(short));
	bufi += sizeof(short);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.geoKeySize), sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.geoDoubleSize), sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.geoAsciiSize), sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) &(filedata.gdalAsciiSize), sizeof(uint32_t));
	bufi += sizeof(uint32_t);
	memcpy((char*)(buffer + bufi), (char*) filedata.geoKeyDir, (sizeof(uint16_t) * filedata.geoKeySize));
	bufi += (sizeof(uint16_t) * filedata.geoKeySize);
	memcpy((char*)(buffer + bufi), (char*) filedata.geoDoubleParams, (sizeof(double) * filedata.geoDoubleSize));
	bufi += (sizeof(double) * filedata.geoDoubleSize);
	memcpy((char*)(buffer + bufi), (char*) filedata.geoAscii, (sizeof(char) * filedata.geoAsciiSize));
	bufi += (sizeof(char) * filedata.geoAsciiSize);
	memcpy((char*)(buffer + bufi), (char*) filedata.gdalAscii, (sizeof(char) * filedata.gdalAsciiSize));
	bufi += (sizeof(char) * filedata.gdalAsciiSize);

	// copy ifds data into buffer
	for( index=0; index<numEntries; ++index ) {
	        memcpy((char*)(buffer + bufi), (char*) &(ifds[index].tag), sizeof(unsigned short));
	        bufi += sizeof(unsigned short);
	        memcpy((char*)(buffer + bufi), (char*) &(ifds[index].type), sizeof(unsigned short));
	        bufi += sizeof(unsigned short);
	        memcpy((char*)(buffer + bufi), (char*) &(ifds[index].count), sizeof(uint32_t));
	        bufi += sizeof(uint32_t);
	        memcpy((char*)(buffer + bufi), (char*) &(ifds[index].offset), sizeof(uint32_t));
	        bufi += sizeof(uint32_t);
	}

	}

	MPI_Bcast(&totalbuf, sizeof(int), MPI_INT, 0, MCW);

	if (rank != 0) {
		buffer = (char*) malloc(totalbuf);
	}

	MPI_Bcast(buffer, totalbuf, MPI_BYTE, 0, MCW);

	if (rank != 0) {
		int bufi = 0;

		// construct tifFile data members except filedata and ifds
		memcpy((char*) &dataSizeFileIn, (char*)(buffer + bufi), sizeof(short));
		bufi += sizeof(short);
		memcpy((char*) &dataSizeObj, (char*)(buffer + bufi), sizeof(short));
		bufi += sizeof(short);
		memcpy((char*) &sampleFormat, (char*)(buffer + bufi), sizeof(short));
		bufi += sizeof(short);
		memcpy((char*) &numOffsets, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		offsets = (uint32_t*) malloc(sizeof(uint32_t)*numOffsets);
		memcpy((char*) offsets, (char*)(buffer + bufi), (numOffsets * sizeof(uint32_t)));
		bufi += (numOffsets * sizeof(uint32_t));
		memcpy((char*) &tileOrRow, (char*)(buffer + bufi), sizeof(short));
		bufi += sizeof(short);
		memcpy((char*) &tileLength, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &tileWidth, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &bytesize, (char*)(buffer + bufi), sizeof(int));
		bufi += sizeof(int);
		bytes = (uint32_t*) malloc(sizeof(uint32_t)*bytesize);
		memcpy((char*) bytes, (char*)(buffer + bufi), (sizeof(uint32_t) * bytesize));
		bufi += (sizeof(uint32_t) * bytesize);
		memcpy((char*) &numEntries, (char*)(buffer + bufi), sizeof(unsigned short));
		bufi += sizeof(unsigned short);
		memcpy((char*) &version, (char*)(buffer + bufi), sizeof(short));
		bufi += sizeof(short);
		memcpy((char*) &totalX, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &totalY, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &dx, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &dy, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &xllcenter, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &yllcenter, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &xleftedge, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &ytopedge, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &datatype, (char*)(buffer + bufi), sizeof(DATA_TYPE));
		bufi += sizeof(DATA_TYPE);
		memcpy((char*) &nodatasize, (char*)(buffer + bufi), sizeof(int));
		bufi += sizeof(int);
		memcpy((char*) &filenodatasize, (char*)(buffer + bufi), sizeof(int));
		bufi += sizeof(int);
		nodata = malloc(nodatasize);
		filenodata = malloc(filenodatasize);
		memcpy((char*) nodata, (char*)(buffer + bufi), nodatasize);
		bufi += nodatasize;
		memcpy((char*) filenodata, (char*)(buffer + bufi), filenodatasize);
		bufi += filenodatasize;
		memcpy((char*) &geoystart, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &geoyend, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &geoxstart, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &geoxend, (char*)(buffer + bufi), sizeof(double));
		bufi += sizeof(double);
		memcpy((char*) &gystart, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &gyend, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &gxstart, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &gxend, (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);

		// copy file data from buffer

		memcpy((char*) &(filedata.xresNum), (char*)(buffer + bufi), sizeof(long));
		bufi += sizeof(long);
		memcpy((char*) &(filedata.xresDen), (char*)(buffer + bufi), sizeof(long));
		bufi += sizeof(long);
		memcpy((char*) &(filedata.yresNum), (char*)(buffer + bufi), sizeof(long));
		bufi += sizeof(long);
		memcpy((char*) &(filedata.yresDen), (char*)(buffer + bufi), sizeof(long));
		bufi += sizeof(long);
		memcpy((char*) &(filedata.planarConfig), (char*)(buffer + bufi), sizeof(short));
		bufi += sizeof(short);
		memcpy((char*) &(filedata.geoKeySize), (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &(filedata.geoDoubleSize), (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &(filedata.geoAsciiSize), (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		memcpy((char*) &(filedata.gdalAsciiSize), (char*)(buffer + bufi), sizeof(uint32_t));
		bufi += sizeof(uint32_t);
		filedata.geoKeyDir = (uint16_t *) malloc( sizeof(uint16_t) * filedata.geoKeySize);
		memcpy((char*) filedata.geoKeyDir, (char*)(buffer + bufi), (sizeof(uint16_t) * filedata.geoKeySize));
		bufi += (sizeof(uint16_t) * filedata.geoKeySize);
		filedata.geoDoubleParams = (double*) malloc( sizeof(double) * filedata.geoDoubleSize );
		memcpy((char*) filedata.geoDoubleParams, (char*)(buffer + bufi), (sizeof(double) * filedata.geoDoubleSize));
		bufi += (sizeof(double) * filedata.geoDoubleSize);
		filedata.geoAscii = (char*) malloc( sizeof(char) * (filedata.geoAsciiSize));
		memcpy((char*) filedata.geoAscii, (char*)(buffer + bufi), (sizeof(char) * filedata.geoAsciiSize));
		bufi += (sizeof(char) * filedata.geoAsciiSize);
		filedata.gdalAscii = (char*) malloc( sizeof(char) * filedata.gdalAsciiSize );
		memcpy((char*) filedata.gdalAscii, (char*)(buffer + bufi), (sizeof(char) * filedata.gdalAsciiSize));
		bufi += (sizeof(char) * filedata.gdalAsciiSize);

		ifds = new ifd[numEntries];
		// copy ifds data into buffer
		long index;
		for( index=0; index<numEntries; ++index ) {
		        memcpy((char*) &(ifds[index].tag), (char*)(buffer + bufi), sizeof(unsigned short));
		        bufi += sizeof(unsigned short);
		        memcpy((char*) &(ifds[index].type), (char*)(buffer + bufi), sizeof(unsigned short));
		        bufi += sizeof(unsigned short);
		        memcpy((char*) &(ifds[index].count), (char*)(buffer + bufi), sizeof(uint32_t));
		        bufi += sizeof(uint32_t);
		        memcpy((char*) &(ifds[index].offset), (char*)(buffer + bufi), sizeof(uint32_t));
		        bufi += sizeof(uint32_t);
		}


	}

	free(buffer);
	nowrite = false;
}
//copy constructor without an output file.
tifFile::tifFile(char *fname, DATA_TYPE newtype, void* nd, const tifFile &copy, bool nofile) {
	//MPI_Status status;
	//MPI_Offset mpiOffset;

	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);
	
	//Create/open the output file
//	int file_error = MPI_File_open( MCW, fname, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
//	if(file_error != MPI_SUCCESS){
//		printf("Error opening file %s.\n",fname);
  //              printf("TauDEM input files have to be GeoTiff files.\n");
    //            MPI_Abort(MCW,22);

//	}
	//Copy metadata from copy tifFile object
	//TODO: ALL metadata needs to be copied here
	nowrite = true;
	datatype = newtype;
	if( datatype == SHORT_TYPE ) {
                dataSizeObj = sizeof (short);
        }
        else if( datatype == LONG_TYPE) {
                dataSizeObj = sizeof (long);
        }
        else if( datatype == FLOAT_TYPE) {
                dataSizeObj = sizeof (float);
        }
	numEntries = copy.numEntries;
	ifds = (ifd*) malloc( sizeof(ifd) * numEntries );
	memcpy( ifds, copy.ifds, sizeof(ifd) * numEntries );
	numOffsets = copy.numOffsets;
	//don't need offset or byte arrays since copied file will never do a data read
	ifds = copy.ifds;
	dataSizeFileIn = copy.dataSizeFileIn;
	dataSizeObj = copy.dataSizeObj;
	sampleFormat = copy.sampleFormat;
	offsets = copy.offsets;
	numOffsets = copy.numOffsets;
	version = copy.version;
	numEntries = copy.numEntries;
	bytes = copy.bytes;
	tileOrRow = copy.tileOrRow;
	tileLength = copy.tileLength;
	tileWidth = copy.tileWidth;
	totalX = copy.totalX;
	totalY = copy.totalY;
	dx = copy.dx;
	dy = copy.dy;
	xllcenter = copy.xllcenter;
	yllcenter = copy.yllcenter;
	xleftedge = copy.xleftedge;
	ytopedge = copy.ytopedge;
	//filenodata = copy.filenodata;
	nodata = nd;
	strncpy(filename, fname, MAXLN);//copy.filename
	gystart = copy.gystart;
	gyend  = copy.gyend;
	gxstart = copy.gxstart;
	gxend = copy.gxend;

	datatype = newtype;
	if( datatype == SHORT_TYPE ) {
		nodata = new short;
		*((short*)nodata) = *((short*)nd);
	}
	else if( datatype == LONG_TYPE) {
		nodata = new long;
		*((long*)nodata) = *((long*)nd);
	}
	else if( datatype == FLOAT_TYPE) {
		nodata = new float;
		*((float*)nodata) = *((float*)nd);
	}

	filedata = copy.filedata;
	if( filedata.geoAsciiSize > 0 ) {
		//SizeOf filedata.geoAscii = (char*) malloc ( filedata.geoAsciiSize);
		filedata.geoAscii = (char*) malloc ( sizeof(char) * filedata.geoAsciiSize );
		//strncpy(filedata.geoAscii, copy.filedata.geoAscii, filedata.geoAsciiSize );
		memcpy( filedata.geoAscii, copy.filedata.geoAscii, filedata.geoAsciiSize );
	}
	if( filedata.gdalAsciiSize > 0 ) {
		//SizeOf filedata.gdalAscii = (char*) malloc ( filedata.gdalAsciiSize);
		filedata.gdalAscii = (char*) malloc ( sizeof(char) * filedata.gdalAsciiSize );
		//strncpy(filedata.gdalAscii, copy.filedata.gdalAscii, filedata.gdalAsciiSize );
		memcpy( filedata.gdalAscii, copy.filedata.gdalAscii, filedata.gdalAsciiSize );
	}
	if( filedata.geoKeySize > 0 ) {
		//BT filedata.geoKeyDir = (unsigned short*) malloc ( filedata.geoKeySize * 2);
		//SizeOf filedata.geoKeyDir = (short*) malloc ( filedata.geoKeySize * 2);
		filedata.geoKeyDir = (uint16_t*) malloc ( sizeof(uint16_t) * filedata.geoKeySize * 2);
		memcpy( filedata.geoKeyDir, copy.filedata.geoKeyDir, filedata.geoKeySize * 2);
	}
	if( filedata.geoDoubleSize > 0 ) {
		//SizeOf filedata.geoDoubleParams = (double*) malloc ( 8 * filedata.geoDoubleSize);
		filedata.geoDoubleParams = (double*) malloc ( sizeof(double) * filedata.geoDoubleSize );
		memcpy( filedata.geoDoubleParams, copy.filedata.geoDoubleParams, filedata.geoDoubleSize*8 );
	}
	//  These copies are after the filedata=copy.filedata above
	//  so that they overwrite the datatype from the copied file
	if( datatype == SHORT_TYPE ) {
		dataSizeObj = sizeof (short);
	}
	else if( datatype == LONG_TYPE) {
		dataSizeObj = sizeof (int32_t);  // DGT Writing long as 32 bit because ArcGIS and GDAL do not appear to support reading of 8 byte (64 bit) integer grids
	}
	else if( datatype == FLOAT_TYPE) {
		dataSizeObj = sizeof (float);
	}
	//Update GeoTiff GeoKeyDirectoryTag to always output GTRasterTypeGeoKey as PixelIsArea
	for ( long i=4; i<filedata.geoKeySize; i+=4) {
		if ( filedata.geoKeyDir[i] == 1025 ) {
			filedata.geoKeyDir[i + 3] = 1;
		}
	}
}
//Copy constructor.  Requires datatype in addition to the object to copy from.
tifFile::tifFile(char *fname, DATA_TYPE newtype, void* nd, const tifFile &copy) {
	//MPI_Status status;
	//MPI_Offset mpiOffset;

	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);
	nowrite = false;
	//Create/open the output file
	
	//int file_error = MPI_File_open( MCW, fname, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
	//if(file_error != MPI_SUCCESS){
	//	printf("Error opening file %s.\n",fname);
 //               printf("TauDEM input files have to be GeoTiff files.\n");
 //               MPI_Abort(MCW,22);

	//}
	
	//Copy metadata from copy tifFile object
	//TODO: ALL metadata needs to be copied here
	datatype = newtype;
	if( datatype == SHORT_TYPE ) {
                dataSizeObj = sizeof (short);
        }
        else if( datatype == LONG_TYPE) {
                dataSizeObj = sizeof (long);
        }
        else if( datatype == FLOAT_TYPE) {
                dataSizeObj = sizeof (float);
        }
	numEntries = copy.numEntries;
	ifds = (ifd*) malloc( sizeof(ifd) * numEntries );
	memcpy( ifds, copy.ifds, sizeof(ifd) * numEntries );
	numOffsets = copy.numOffsets;
	//don't need offset or byte arrays since copied file will never do a data read
	ifds = copy.ifds;
	dataSizeFileIn = copy.dataSizeFileIn;
	dataSizeObj = copy.dataSizeObj;
	sampleFormat = copy.sampleFormat;
	offsets = copy.offsets;
	numOffsets = copy.numOffsets;
	version = copy.version;
	numEntries = copy.numEntries;
	bytes = copy.bytes;
	tileOrRow = copy.tileOrRow;
	tileLength = copy.tileLength;
	tileWidth = copy.tileWidth;
	totalX = copy.totalX;
	totalY = copy.totalY;
	dx = copy.dx;
	dy = copy.dy;
	xllcenter = copy.xllcenter;
	yllcenter = copy.yllcenter;
	xleftedge = copy.xleftedge;
	ytopedge = copy.ytopedge;
	filenodata = filenodata;
	nodata = nd;
	strncpy(filename, fname, MAXLN);//copy.filename
	gystart = copy.gystart;
	gyend  = copy.gyend;
	gxstart = copy.gxstart;
	gxend = copy.gxend;

	datatype = newtype;
	if( datatype == SHORT_TYPE ) {
		nodata = new short;
		*((short*)nodata) = *((short*)nd);
	}
	else if( datatype == LONG_TYPE) {
		nodata = new long;
		*((long*)nodata) = *((long*)nd);
	}
	else if( datatype == FLOAT_TYPE) {
		nodata = new float;
		*((float*)nodata) = *((float*)nd);
	}

	filedata = copy.filedata;
	if( filedata.geoAsciiSize > 0 ) {
		//SizeOf filedata.geoAscii = (char*) malloc ( filedata.geoAsciiSize);
		filedata.geoAscii = (char*) malloc ( sizeof(char) * filedata.geoAsciiSize );
		//strncpy(filedata.geoAscii, copy.filedata.geoAscii, filedata.geoAsciiSize );
		memcpy( filedata.geoAscii, copy.filedata.geoAscii, filedata.geoAsciiSize );
	}
	if( filedata.gdalAsciiSize > 0 ) {
		//SizeOf filedata.gdalAscii = (char*) malloc ( filedata.gdalAsciiSize);
		filedata.gdalAscii = (char*) malloc ( sizeof(char) * filedata.gdalAsciiSize );
		//strncpy(filedata.gdalAscii, copy.filedata.gdalAscii, filedata.gdalAsciiSize );
		memcpy( filedata.gdalAscii, copy.filedata.gdalAscii, filedata.gdalAsciiSize );
	}
	if( filedata.geoKeySize > 0 ) {
		//BT filedata.geoKeyDir = (unsigned short*) malloc ( filedata.geoKeySize * 2);
		//SizeOf filedata.geoKeyDir = (short*) malloc ( filedata.geoKeySize * 2);
		filedata.geoKeyDir = (uint16_t*) malloc ( sizeof(uint16_t) * filedata.geoKeySize * 2);
		memcpy( filedata.geoKeyDir, copy.filedata.geoKeyDir, filedata.geoKeySize * 2);
	}
	if( filedata.geoDoubleSize > 0 ) {
		//SizeOf filedata.geoDoubleParams = (double*) malloc ( 8 * filedata.geoDoubleSize);
		filedata.geoDoubleParams = (double*) malloc ( sizeof(double) * filedata.geoDoubleSize );
		memcpy( filedata.geoDoubleParams, copy.filedata.geoDoubleParams, filedata.geoDoubleSize*8 );
	}
	//  These copies are after the filedata=copy.filedata above
	//  so that they overwrite the datatype from the copied file
	if( datatype == SHORT_TYPE ) {
		dataSizeObj = sizeof (short);
	}
	else if( datatype == LONG_TYPE) {
		dataSizeObj = sizeof (int32_t);  // DGT Writing long as 32 bit because ArcGIS and GDAL do not appear to support reading of 8 byte (64 bit) integer grids
	}
	else if( datatype == FLOAT_TYPE) {
		dataSizeObj = sizeof (float);
	}
	//Update GeoTiff GeoKeyDirectoryTag to always output GTRasterTypeGeoKey as PixelIsArea
	for ( long i=4; i<filedata.geoKeySize; i+=4) {
		if ( filedata.geoKeyDir[i] == 1025 ) {
			filedata.geoKeyDir[i + 3] = 1;
		}
	}
}

tifFile::~tifFile(){
	if(nowrite)
		return;
	MPI_File_close(&fh);
}

//Read tiff file data/image values beginning at xstart, ystart (gridwide coordinates) for the numRows, and numCols indicated to memory locations specified by dest
//BT void tifFile::read(unsigned long long xstart, unsigned long long ystart, unsigned long long numRows, unsigned long long numCols, void* dest) {
void tifFile::read(long xstart, long ystart, long numRows, long numCols, void* dest, long partOffset, long DomainX) {

	//current code assumes Tiff uses Strips and Partition Type is Linear Partition; TODO - recode to eliminate these assumptions

	MPI_Status status;
	MPI_Offset mpiOffset;
	if(tileOrRow == 2)  
	{  // Strips

 	//Find the starting and ending indexes of the data requested
	long firstStripIndex = ystart/tileLength;  //index into tiff strip arrays of first strip to be read 
	long lastStripIndex = (ystart + numRows-1)/tileLength ; //index into tiff strip arrays of the last strip to be read
	long firstStripFirstRowIndex = ystart - ( firstStripIndex * tileLength); //index within the first tiff strip of the first row to be read
	long lastStripLastRowIndex = ystart + numRows-1 - ( lastStripIndex * tileLength ); //index within the last tiff strip of the last row to be read
	//Find the initial current strip indexes
	long currentStripFirstRowIndex; // = firstStripFirstRowIndex;  //index of first row needed within current strip; start with first strip needed
	long currentStripLastRowIndex;  // index of last row needed within current strip; start with first strip needed
//  DGT edited to properly handle multiple file cases
	//if (lastStripIndex > 0) {
	//	currentStripLastRowIndex = tileLength-1;
	//} else{
	//	currentStripLastRowIndex = lastStripLastRowIndex;
	//}

	//loop through needed TIFF Strips
	for( long i = firstStripIndex; i <= lastStripIndex; ++i ) {
		if( i > firstStripIndex)currentStripFirstRowIndex=0;
		else currentStripFirstRowIndex=firstStripFirstRowIndex;
		if(i < lastStripIndex)currentStripLastRowIndex=tileLength-1;
		else currentStripLastRowIndex = lastStripLastRowIndex;

		mpiOffset = offsets[i];
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		mpiOffset = dataSizeFileIn * tileWidth * currentStripFirstRowIndex;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_CUR);
		//loop through needed rows
		for( long j = currentStripFirstRowIndex; j <= currentStripLastRowIndex; ++j) {
			//  A separate block of code is needed for each supported input file type
			//  because even if we read into a void variable we still need the logic that
			//  determines how to interpret the read data to be able to typecast it onto
			//  the output file type  
			if((sampleFormat == 1) && (dataSizeFileIn == 1)) {
				uint8_t *tempDataRow= NULL;
				try
				{
					tempDataRow = new uint8_t[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint8_t tempVal = tempDataRow[k];
						//  DGT changed these to only write actual data values
						//  This is in case there are multiple files that overlap in their no data regions
						//  and one of them has data.  This is possible because partitions are always initialized 
						//  with a no data value
						if(tempVal != *(uint8_t*)filenodata) 
	/*						((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						else*/
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint8_t tempVal = tempDataRow[k];
						if(tempVal != *(uint8_t*)filenodata)  
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint8_t tempVal = tempDataRow[k];
						if(tempVal != *(uint8_t*)filenodata)  
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
				uint16_t *tempDataRow= NULL;
				try
				{
					tempDataRow = new uint16_t[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint16_t tempVal = tempDataRow[k];
						if(tempVal != *(uint16_t*)filenodata) 
						//	((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						//else
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint16_t tempVal = tempDataRow[k];
						if(tempVal != *(uint16_t*)filenodata)  
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint16_t tempVal = tempDataRow[k];
						if(tempVal != *(uint16_t*)filenodata)  
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
				uint32_t *tempDataRow= NULL ;
				try
				{
					tempDataRow = new uint32_t[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint32_t tempVal = tempDataRow[k];
						if(tempVal != *(uint32_t*)filenodata) 
						//	((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						//else
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint32_t tempVal = tempDataRow[k];
						if(tempVal != *(uint32_t*)filenodata)  
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint32_t tempVal = tempDataRow[k];
						if(tempVal != *(uint32_t*)filenodata)  
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
				int8_t* tempDataRow = NULL;
				try
				{
					tempDataRow = new int8_t[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int8_t tempVal = tempDataRow[k];
						if(tempVal != *(int8_t*)filenodata) 
						//	((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						//else
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int8_t tempVal = tempDataRow[k];
						if(tempVal != *(int8_t*)filenodata)  
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int8_t tempVal = tempDataRow[k];
						if(tempVal != *(int8_t*)filenodata)  
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
				int16_t* tempDataRow = NULL;
				try
				{
					tempDataRow = new int16_t[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int16_t tempVal = tempDataRow[k];
						if(tempVal != *(int16_t*)filenodata) 
						//	((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						//else
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int16_t tempVal = tempDataRow[k];
						if(tempVal != *(int16_t*)filenodata)  
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int16_t tempVal = tempDataRow[k];
						if(tempVal != *(int16_t*)filenodata)  
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
				int32_t* tempDataRow = NULL;
				try
				{
					tempDataRow = new int32_t[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int32_t tempVal = tempDataRow[k];
						if(tempVal != *(int32_t*)filenodata) 
						//	((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						//else
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int32_t tempVal = tempDataRow[k];
						if(tempVal != *(int32_t*)filenodata)  
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int32_t tempVal = tempDataRow[k];
						if(tempVal != *(int32_t*)filenodata)  
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;
			} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
				float *tempDataRow= NULL;
				try
				{
					tempDataRow = new float[tileWidth];
				}
				catch(bad_alloc&)
				{
				//  DGT added clause below to try trap for insufficient memory in the computer.
					fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
					fflush(stdout);
					MPI_Abort(MCW,-999);
				}
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						float tempVal = tempDataRow[k];
						if(tempVal != *(float*)filenodata)  // Risky conditional on float
						//	((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(short*)nodata;
						//else
							((short*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						float tempVal = tempDataRow[k];
						if(tempVal != *(float*)filenodata)  // Risky conditional on float
						//	((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(long*)nodata;
						//else
							((long*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						float tempVal = tempDataRow[k];
						if(tempVal != *(float*)filenodata)  // Risky conditional on float
						//	((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]= *(float*)nodata;
						//else
							((float*)dest)[(j+(i*tileLength)-ystart)*DomainX+k + partOffset]=(float)tempVal;
					}
				delete tempDataRow;

			} else {
				printf("Unsupported TIFF file type.  sampleFormat = %d, dataSizeFileIn = %d.\n", sampleFormat, dataSizeFileIn);
				MPI_Abort(MCW,-1);
			}
		}
		//  DGT doing this at top of loop to handle multiple files properly
		//update current strip indexes for next strip
		//currentStripFirstRowIndex = 0;
		//if ( i != lastStripIndex-1 ) {
		//	currentStripLastRowIndex = tileLength-1;
		//}else {
		//	currentStripLastRowIndex = lastStripLastRowIndex;
		//}
	}
	}
	else
	{  // tiles.  The strategy is to read one row at a time reading the parts 
		// of it that come from the appropriate tiles as blocks
		uint32_t y, tileStart, tilesAcross, tileEnd, tile, rowInTile, tileCols, destOffset;

		// tileStart - the first tile in the set being read from for row y
		// tilesAcross - the number of tiles across the grid
		// tileEnd - the last tile in the set being read form for row y
		// tile - the active tile being read from
		// rowInTile - the row in the tile corresponding to the tile being read
		// tileCols - the number of columns in the tile that fall within the grid
		//    This is either tileWidth or determined from totalX for the last tile
		// destOffset - the offset within the destination partition of the row being read for a tile

		tilesAcross=(totalX-1)/tileWidth+1;  // DGT 7/8/10 - was tilesAcross=totalX/tileWidth+1 which failed if totalX was a multiple of tileWidth
		for(y=ystart; y<ystart+numRows; y++)
		{
			tileStart=y/tileLength*tilesAcross;  // First tile in the read of a row
			if(tileStart > 0)
				tileStart=tileStart;
			tileEnd=tileStart+tilesAcross-1;   // Last tile in the read of a row
			rowInTile=y-(tileStart/tilesAcross)*tileLength;
			for(tile=tileStart; tile<=tileEnd;tile++)
			{
				tileCols = tileWidth;
				if(tile == tileEnd)tileCols=totalX-(tilesAcross-1)*tileWidth;
				mpiOffset=offsets[tile]+rowInTile*tileWidth*dataSizeFileIn;
				MPI_File_seek(fh,mpiOffset,MPI_SEEK_SET);
				destOffset=(tile-tileStart)*tileWidth+(y-ystart)*DomainX + partOffset;
				if((sampleFormat == 1) && (dataSizeFileIn == 1)) 
				{
					uint8_t *tempDataRow= NULL;
					try
					{
							tempDataRow = new uint8_t[tileWidth];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint8_t tempVal = tempDataRow[k];
							if(tempVal != *(uint8_t*)filenodata)  
							//	((short*)dest)[destOffset+k]= *(short*)nodata;
							//else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint8_t tempVal = tempDataRow[k];
							if(tempVal != *(uint8_t*)filenodata)  
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint8_t tempVal = tempDataRow[k];
							if(tempVal != *(uint8_t*)filenodata)  
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
				delete tempDataRow;
				}
				else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) 
				{
					uint16_t *tempDataRow= NULL;
					try
					{
							tempDataRow = new uint16_t[tileWidth];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);
					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint16_t tempVal = tempDataRow[k];
							if(tempVal != *(uint16_t*)filenodata)  
							//	((short*)dest)[destOffset+k]= *(short*)nodata;
							//else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint16_t tempVal = tempDataRow[k];
							if(tempVal != *(uint16_t*)filenodata)  // Risky conditional on float
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint16_t tempVal = tempDataRow[k];
							if(tempVal != *(uint16_t*)filenodata)  // Risky conditional on float
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
					delete tempDataRow;
				}
				else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) 
				{
					uint32_t *tempDataRow= NULL;				
					try
					{
							tempDataRow = new uint32_t[tileWidth];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);
					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint32_t tempVal = tempDataRow[k];
							if(tempVal != *(uint32_t*)filenodata)  
							//	((short*)dest)[destOffset+k]= *(short*)nodata;
							//else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint32_t tempVal = tempDataRow[k];
							if(tempVal != *(uint32_t*)filenodata)  
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint32_t tempVal = tempDataRow[k];
							if(tempVal != *(uint32_t*)filenodata)  
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
					delete tempDataRow;
				}
				else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) 
				{
					int8_t* tempDataRow = NULL;					
					try
					{
							tempDataRow = new int8_t[tileWidth];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int8_t tempVal = tempDataRow[k];
							if(tempVal != *(int8_t*)filenodata)  
							//	((short*)dest)[destOffset+k]= *(short*)nodata;
							//else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int8_t tempVal = tempDataRow[k];
							if(tempVal != *(int8_t*)filenodata)  
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int8_t tempVal = tempDataRow[k];
							if(tempVal != *(int8_t*)filenodata)  
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
					delete tempDataRow;
				}
				else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) 
				{
					int16_t* tempDataRow = NULL;
					try
					{
							tempDataRow = new int16_t[tileWidth];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}

					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int16_t tempVal = tempDataRow[k];
							if(tempVal != *(int16_t*)filenodata)  
						//		((short*)dest)[destOffset+k]= *(short*)nodata;
						//	else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int16_t tempVal = tempDataRow[k];
							if(tempVal != *(int16_t*)filenodata)  
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int16_t tempVal = tempDataRow[k];
							if(tempVal != *(int16_t*)filenodata) 
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
					delete tempDataRow;
				}
				else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) 
				{
					int32_t* tempDataRow = NULL;
					try
					{
							tempDataRow = new int32_t[tileWidth];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}
					
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int32_t tempVal = tempDataRow[k];
							if(tempVal != *(int32_t*)filenodata)  
							//	((short*)dest)[destOffset+k]= *(short*)nodata;
							//else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int32_t tempVal = tempDataRow[k];
							if(tempVal != *(int32_t*)filenodata)  
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int32_t tempVal = tempDataRow[k];
							if(tempVal != *(int32_t*)filenodata)  
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
					delete tempDataRow;
				}
				else if(sampleFormat == 3 && dataSizeFileIn == 4)
				{
					float *tempDataRow = NULL;
					try
					{
							tempDataRow = new float[tileCols];
					}
					catch(bad_alloc&)
					{
					//  DGT added clause below to try trap for insufficient memory in the computer.
						fprintf(stdout,"Memory allocation error during file read in process %d.\n",rank);
						fflush(stdout);
						MPI_Abort(MCW,-999);
					}
		//					cout << "1" << dataSizeFileIn << " " <<  endl;
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							float tempVal = tempDataRow[k];
							if(tempVal != *(float*)filenodata)  // Risky conditional on float
							//	((short*)dest)[destOffset+k]= *(short*)nodata;
							//else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							float tempVal = tempDataRow[k];
							if(tempVal != *(float*)filenodata)  // Risky conditional on float
							//	((long*)dest)[destOffset+k]= *(long*)nodata;
							//else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							float tempVal = tempDataRow[k];
							if(tempVal != *(float*)filenodata)  // Risky conditional on float
							//	((float*)dest)[destOffset+k]= *(float*)nodata;
							//else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}
//					cout <<"Releasing";
					delete tempDataRow;
				}
			}
		}
	}
}
void tifFile::read(long xstart, long ystart, long numRows, long numCols, void* dest) {

	//current code assumes Tiff uses Strips and Partition Type is Linear Partition; TODO - recode to eliminate these assumptions

	MPI_Status status;
	MPI_Offset mpiOffset;
	if(tileOrRow == 2)  
	{  // Strips

 	//Find the starting and ending indexes of the data requested
//	long firstStripIndex = (long) floor(ystart/tileLength);  //index into tiff strip arrays of first strip to be read 
//	long lastStripIndex = (long) floor((ystart + numRows)/tileLength ); //index into tiff strip arrays of the last strip to be read
	long firstStripIndex = ystart/tileLength;  //index into tiff strip arrays of first strip to be read 
	long lastStripIndex = (ystart + numRows-1)/tileLength ; //index into tiff strip arrays of the last strip to be read
	long firstStripFirstRowIndex = ystart - ( firstStripIndex * tileLength); //index within the first tiff strip of the first row to be read
	long lastStripLastRowIndex = ystart + numRows-1 - ( lastStripIndex * tileLength ); //index within the last tiff strip of the last row to be read

	//Find the initial current strip indexes
	long currentStripFirstRowIndex = firstStripFirstRowIndex;  //index of first row needed within current strip; start with first strip needed
	long currentStripLastRowIndex;  // index of last row needed within current strip; start with first strip needed
	if (lastStripIndex > 0) {
		currentStripLastRowIndex = tileLength-1;
	} else{
		currentStripLastRowIndex = lastStripLastRowIndex;
	}

	//loop through needed TIFF Strips
	for( long i = firstStripIndex; i <= lastStripIndex; ++i ) {
		mpiOffset = offsets[i];
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		mpiOffset = dataSizeFileIn * tileWidth * currentStripFirstRowIndex;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_CUR);
		//loop through needed rows
		for( long j = currentStripFirstRowIndex; j <= currentStripLastRowIndex; ++j) {
			//  A separate block of code is needed for each supported input file type
			//  because even if we read into a void variable we still need the logic that
			//  determines how to interpret the read data to be able to typecast it onto
			//  the output file type  
			if((sampleFormat == 1) && (dataSizeFileIn == 1)) {
				uint8_t *tempDataRow= new uint8_t[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint8_t tempVal = tempDataRow[k];
						if(tempVal == *(uint8_t*)filenodata) 
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint8_t tempVal = tempDataRow[k];
						if(tempVal == *(uint8_t*)filenodata)  
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint8_t tempVal = tempDataRow[k];
						if(tempVal == *(uint8_t*)filenodata)  
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) {
				uint16_t *tempDataRow= new uint16_t[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint16_t tempVal = tempDataRow[k];
						if(tempVal == *(uint16_t*)filenodata) 
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint16_t tempVal = tempDataRow[k];
						if(tempVal == *(uint16_t*)filenodata)  
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint16_t tempVal = tempDataRow[k];
						if(tempVal == *(uint16_t*)filenodata)  
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) {
				uint32_t *tempDataRow= new uint32_t[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint32_t tempVal = tempDataRow[k];
						if(tempVal == *(uint32_t*)filenodata) 
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint32_t tempVal = tempDataRow[k];
						if(tempVal == *(uint32_t*)filenodata)  
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						uint32_t tempVal = tempDataRow[k];
						if(tempVal == *(uint32_t*)filenodata)  
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) {
				int8_t* tempDataRow = new int8_t[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int8_t tempVal = tempDataRow[k];
						if(tempVal == *(int8_t*)filenodata) 
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int8_t tempVal = tempDataRow[k];
						if(tempVal == *(int8_t*)filenodata)  
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int8_t tempVal = tempDataRow[k];
						if(tempVal == *(int8_t*)filenodata)  
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) {
				int16_t* tempDataRow = new int16_t[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int16_t tempVal = tempDataRow[k];
						if(tempVal == *(int16_t*)filenodata) 
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int16_t tempVal = tempDataRow[k];
						if(tempVal == *(int16_t*)filenodata)  
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int16_t tempVal = tempDataRow[k];
						if(tempVal == *(int16_t*)filenodata)  
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) {
				int32_t* tempDataRow = new int32_t[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int32_t tempVal = tempDataRow[k];
						if(tempVal == *(int32_t*)filenodata) 
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int32_t tempVal = tempDataRow[k];
						if(tempVal == *(int32_t*)filenodata)  
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						int32_t tempVal = tempDataRow[k];
						if(tempVal == *(int32_t*)filenodata)  
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else if ((sampleFormat == 3) && (dataSizeFileIn == 4)) {
				float *tempDataRow= new float[tileWidth];
				MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileWidth, MPI_BYTE, &status);
				if(datatype == SHORT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						float tempVal = tempDataRow[k];
						if(tempVal == *(float*)filenodata)  // Risky conditional on float
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(short*)nodata;
						else
							((short*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(short)tempVal;
					}
				else if(datatype == LONG_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						float tempVal = tempDataRow[k];
						if(tempVal == *(float*)filenodata)  // Risky conditional on float
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(long*)nodata;
						else
							((long*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(long)tempVal;
					}
				else if(datatype == FLOAT_TYPE)
					for(long k = 0; k < tileWidth; k++)
					{
						float tempVal = tempDataRow[k];
						if(tempVal == *(float*)filenodata)  // Risky conditional on float
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]= *(float*)nodata;
						else
							((float*)dest)[(j+(i*tileLength)-ystart)*tileWidth+k]=(float)tempVal;
					}

					delete tempDataRow;
			} else {
				printf("Unsupported TIFF file type.  sampleFormat = %d, dataSizeFileIn = %d.\n", sampleFormat, dataSizeFileIn);
				MPI_Abort(MCW,-1);
			}
		}
		//update current strip indexes for next strip
		currentStripFirstRowIndex = 0;
		if ( i != lastStripIndex-1 ) {
			currentStripLastRowIndex = tileLength-1;
		}else {
			currentStripLastRowIndex = lastStripLastRowIndex;
		}
	}
	}
	else
	{  // tiles.  The strategy is to read one row at a time reading the parts 
		// of it that come from the appropriate tiles as blocks
		uint32_t y, tileStart, tilesAcross, tileEnd, tile, rowInTile, tileCols, destOffset;

		// tileStart - the first tile in the set being read from for row y
		// tilesAcross - the number of tiles across the grid
		// tileEnd - the last tile in the set being read form for row y
		// tile - the active tile being read from
		// rowInTile - the row in the tile corresponding to the tile being read
		// tileCols - the number of columns in the tile that fall within the grid
		//    This is either tileWidth or determined from totalX for the last tile
		// destOffset - the offset within the destination partition of the row being read for a tile

		tilesAcross=(totalX-1)/tileWidth+1;
		for(y=ystart; y<ystart+numRows; y++)
		{
			tileStart=y/tileLength*tilesAcross;  // First tile in the read of a row
			if(tileStart > 0)
				tileStart=tileStart;
			tileEnd=tileStart+tilesAcross-1;   // Last tile in the read of a row
			rowInTile=y-(tileStart/tilesAcross)*tileLength;
			for(tile=tileStart; tile<=tileEnd;tile++)
			{
				tileCols = tileWidth;
				if(tile == tileEnd)tileCols=totalX-(tilesAcross-1)*tileWidth;
				mpiOffset=offsets[tile]+rowInTile*tileWidth*dataSizeFileIn;
				MPI_File_seek(fh,mpiOffset,MPI_SEEK_SET);
				destOffset=(tile-tileStart)*tileWidth+(y-ystart)*numCols;
				if((sampleFormat == 1) && (dataSizeFileIn == 1)) 
				{
					uint8_t *tempDataRow= new uint8_t[tileWidth];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint8_t tempVal = tempDataRow[k];
							if(tempVal == *(uint8_t*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint8_t tempVal = tempDataRow[k];
							if(tempVal == *(uint8_t*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint8_t tempVal = tempDataRow[k];
							if(tempVal == *(uint8_t*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
				else if ((sampleFormat == 1) && (dataSizeFileIn == 2)) 
				{
					uint16_t *tempDataRow= new uint16_t[tileWidth];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);
					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint16_t tempVal = tempDataRow[k];
							if(tempVal == *(uint16_t*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint16_t tempVal = tempDataRow[k];
							if(tempVal == *(uint16_t*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint16_t tempVal = tempDataRow[k];
							if(tempVal == *(uint16_t*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
				else if ((sampleFormat == 1) && (dataSizeFileIn == 4)) 
				{
					uint32_t *tempDataRow= new uint32_t[tileWidth];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);
					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint32_t tempVal = tempDataRow[k];
							if(tempVal == *(uint32_t*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint32_t tempVal = tempDataRow[k];
							if(tempVal == *(uint32_t*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							uint32_t tempVal = tempDataRow[k];
							if(tempVal == *(uint32_t*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
				else if ((sampleFormat == 2) && (dataSizeFileIn == 1)) 
				{
					int8_t* tempDataRow = new int8_t[tileWidth];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int8_t tempVal = tempDataRow[k];
							if(tempVal == *(int8_t*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int8_t tempVal = tempDataRow[k];
							if(tempVal == *(int8_t*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int8_t tempVal = tempDataRow[k];
							if(tempVal == *(int8_t*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
				else if ((sampleFormat == 2) && (dataSizeFileIn == 2)) 
				{
					int16_t* tempDataRow = new int16_t[tileWidth];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int16_t tempVal = tempDataRow[k];
							if(tempVal == *(int16_t*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int16_t tempVal = tempDataRow[k];
							if(tempVal == *(int16_t*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int16_t tempVal = tempDataRow[k];
							if(tempVal == *(int16_t*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
				else if ((sampleFormat == 2) && (dataSizeFileIn == 4)) 
				{
					int32_t* tempDataRow = new int32_t[tileWidth];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int32_t tempVal = tempDataRow[k];
							if(tempVal == *(int32_t*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int32_t tempVal = tempDataRow[k];
							if(tempVal == *(int32_t*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							int32_t tempVal = tempDataRow[k];
							if(tempVal == *(int32_t*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
				else if(sampleFormat == 3 && dataSizeFileIn == 4)
				{
					float *tempDataRow=new float[tileCols];
					MPI_File_read( fh, tempDataRow, dataSizeFileIn * tileCols, MPI_BYTE, &status);

					if(datatype == SHORT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							float tempVal = tempDataRow[k];
							if(tempVal == *(float*)filenodata)  // Risky conditional on float
								((short*)dest)[destOffset+k]= *(short*)nodata;
							else
								((short*)dest)[destOffset+k]=(short)tempVal;
						}
					else if(datatype == LONG_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							float tempVal = tempDataRow[k];
							if(tempVal == *(float*)filenodata)  // Risky conditional on float
								((long*)dest)[destOffset+k]= *(long*)nodata;
							else
								((long*)dest)[destOffset+k]=(long)tempVal;
						}
					else if(datatype == FLOAT_TYPE)
						for(long k = 0; k < tileCols; k++)
						{
							float tempVal = tempDataRow[k];
							if(tempVal == *(float*)filenodata)  // Risky conditional on float
								((float*)dest)[destOffset+k]= *(float*)nodata;
							else
								((float*)dest)[destOffset+k]=(float)tempVal;
						}

						delete tempDataRow;
				}
			}
		}
	}
}

//Create/re-write tiff output file
//BT void tifFile::write(unsigned long long xstart, unsigned long long ystart, unsigned long long numRows, unsigned long long numCols, void* source) {
void tifFile::write(long xstart, long ystart, long numRows, long numCols, void* source, long srcOffset, long DomainX, bool mf) {

	MPI_Status status;
	MPI_Offset mpiOffset;

	//Calculate parameters needed by multiple tags
	//Calculate strip data offsets and sizes
	numOffsets = (totalY + tileLength - 1)/tileLength;
	//BT unsigned long long *dataOffsets;
	//BT unsigned long long *sizeOffsets;
	//BT dataOffsets = (unsigned long long*) malloc( 8 * numOffsets );
	//BT sizeOffsets = (unsigned long long*) malloc( 8 * numOffsets );
	//BT numOffsets = (totalY + tileLength - 1)/tileLength;
	uint32_t *dataOffsets;  //DGT
	uint32_t *sizeOffsets;  //DGT
	//SizeOf dataOffsets = (long*) malloc( 4 * numOffsets );
	//SizeOf sizeOffsets = (long*) malloc( 4 * numOffsets );
	dataOffsets = (uint32_t*) malloc( sizeof(uint32_t) * numOffsets );  //DGT
	sizeOffsets = (uint32_t*) malloc( sizeof(uint32_t) * numOffsets );  //DGT

	//BT unsigned long long sizeofStrip = tileLength * tileWidth * dataSize;
	//BT unsigned long long dataOffset = 8; //offset to beginning of image data block. Put it right after file header
	//BT for( unsigned long long i = 0; i< numOffsets; i++ ) {
	//  The below is hard coded for strips that inherit the tileLength of the copied object
	uint32_t sizeofStrip = tileLength * totalX * dataSizeObj;  // DGT made uint32_t, was long
	uint32_t dataOffset = 8; //offset to beginning of image data block. Put it right after file header  //DGT made uint32_t, was long
	for( long i = 0; i< numOffsets; i++ ) {
		sizeOffsets[i] = tileLength * totalX * dataSizeObj;
		dataOffsets[i] = dataOffset + (i * sizeofStrip);
	}
	// Recalculate size of final, potentially partial strip
	//DGT  Deleted dataOffset from line below
	//sizeOffsets[numOffsets-1] = dataOffset + ((totalY - (tileLength * (numOffsets-1))) * totalX * dataSize);
	sizeOffsets[numOffsets-1] = ((totalY - (tileLength * (numOffsets-1))) * totalX * dataSizeObj);
	//numEntries is the number of tags in the Oth (first and only) IFD
	//Consider implementing additional tags in the future: 34735 - 34797 and 42112

	numEntries = 14;
	//  DGT implementing spatial reference tag
	if(filedata.geoKeySize > 0)
	{
		numEntries++;
	}
	if(filedata.geoDoubleSize > 0)
	{
		numEntries++;
	}
	if(filedata.geoAsciiSize > 0)
	{
		numEntries++;
	}
	//  GDAL tag seems needed for ArcGIS to read GDAL spatial reference
	if(filedata.gdalAsciiSize > 0)
	{
		numEntries++;
	}

	//Write all of the file except the data/image block
	//This includes the file header, and all of the metadata, including the IFD header, footer, and data value blocks.
	//Only process 0 gets to write outsite the data/image block
	if(mf == true){

		//int file_error = MPI_File_open( MCW, fname, MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
		int file_error = MPI_File_open( MPI_COMM_SELF , filename, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
        // allow overwriting of existing file
		if(file_error != MPI_SUCCESS){
			printf("Error opening file %s.\n",filename);
			printf("Failed to create or open an existing file for writing in process 0.\n");
			MPI_Abort(MCW,22);
		}
		//nextAvailable is the offset of nextAvailable unallocated area in the output file
		//initially set it to the offset of the beginning of the 0th (first) IFD
		//BT unsigned long long nextAvailable = dataOffset + (totalX * totalY * dataSize);
		uint32_t nextAvailable = dataOffset + (totalX * totalY * dataSizeObj);  //DGT made uint32_t, was long
		//printf("DataOffset: %lu, totalX: %lu, totalY: %lu, dataSize: %d\n",dataOffset, totalX, totalY, dataSizeObj);
		//printf("Next available: %lu\n",nextAvailable);
		//fflush(stdout);
	
		//Write file header information
		//Write the byte-order value
		short endian = LITTLEENDIAN;
		MPI_File_write( fh, &endian, 2, MPI_BYTE, &status);
	
		//Write the tiff file identifier 
		short version = TIFF;
		MPI_File_write( fh, &version,2, MPI_BYTE, &status);

		//Write the offset of 0th (first and only) Image File Directory (IFD)
		//BT unsigned long long tableOffset = nextAvailable;
		uint32_t tableOffset = nextAvailable;  //DGT made uint32_t, was long
		nextAvailable += (numEntries * 12)+6; //update to offset of first Tag data value block by adding 12 bytes for each tag and 6 bytes for the IFD header and footer
		MPI_File_write( fh, &tableOffset, 4, MPI_BYTE, &status);

		//Go to beginning of 0th (first and only) IFD
		//Write Number of Directory Entries for 0th IFD
		MPI_File_seek( fh, tableOffset, MPI_SEEK_SET );
		MPI_File_write( fh, &numEntries, 2, MPI_BYTE, &status);
		
		ifd obj;

		//Write entries for Oth (first and only) IFD
		//Entry 0 - Image Width (columns)
		obj.tag = 256;
		obj.type = 4;  //DGT 10/19/11 changed from obj.type = 3;  Per email with Alan Richardson alan_r@mit.edu and reading http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf pages 13-15
		obj.count = 1;
		obj.offset = totalX;
		writeIfd( obj);
	
		//Entry 1 - Image Length (rows)
		obj.tag = 257;
		obj.type = 4;  //DGT 10/19/11 changed from obj.type = 3; Same as above
		obj.count = 1;
		obj.offset = totalY;
		writeIfd( obj);
	
		//Entry 2 - Bits (data size) per Sample
		//printf("dataSizeObj: %d.\n", dataSizeObj);
		obj.tag = 258;
		obj.type = 3;
		obj.count = 1;
		obj.offset = dataSizeObj * 8;
		writeIfd( obj);
	
		//Entry 3 - Compression (1=No Compression)
		obj.tag = 259;
		obj.type = 3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 4 - Photometric Interpretation (1=Black Is Zero)
		obj.tag = 262;
		obj.type = 3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 5 - Strip Offsets (pointers to beginning of strips)
		//DGT  This does not properly handle the case for only one strip
		obj.tag = 273;
		obj.type = 4;
		obj.count = numOffsets;
		//BT unsigned long long WRITEOFFSETS;  //  DGT insert to handle case for only one strip
		unsigned long WRITEOFFSETS;  //  DGT insert to handle case for only one strip
		if (numOffsets == 1)  
		{
			obj.offset=dataOffsets[0];
		}
		else
		{
			obj.offset = nextAvailable;
			WRITEOFFSETS = nextAvailable;
			nextAvailable += numOffsets*4;
		}
		writeIfd( obj);
			
		//Entry 6 - Samples per Pixel (always 1)
		obj.tag = 277;
		obj.type =3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 7 - Rows per Strip
		obj.tag = 278;
		obj.type =3;
		obj.count = 1;
		obj.offset = tileLength;
		writeIfd( obj);
	
		//Entry 8 - Strip Byte Counts
		//DGT  This does not properly handle the case for only one strip
		obj.tag = 279;
		obj.type = 4;
		obj.count = numOffsets;
		//BT unsigned long long WRITESIZES;   //  DGT changes to handle case for only one strip
		unsigned long WRITESIZES;   //  DGT changes to handle case for only one strip
		if(numOffsets == 1)
		{
			obj.offset=sizeOffsets[0];
		}
		else
		{
			obj.offset = nextAvailable;
			WRITESIZES = nextAvailable;
			nextAvailable += numOffsets*4;
		}
		writeIfd( obj);		
	
		//Entry 9 - Planar Configuration (always 1)
		obj.tag = 284;
		obj.type =3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 10 - Sample format ( 1=unsigned integer, 2=signed integer, 3=IEEE floating point )
		//Note that size/length of pixel is defined by the BitsPerSample field
		obj.tag = 339;
		obj.type =3;
		obj.count = 1;
		if( datatype == FLOAT_TYPE )
			obj.offset = 3;
		else obj.offset = 2;  // This assumes that all integer grids being written are signed
		writeIfd( obj);
		
		//Entry 11 - Scale Tag
		obj.tag = 33550;
		obj.type = 12;
		obj.count = 3;
		obj.offset = nextAvailable;
		writeIfd( obj);
		//BT unsigned long long WRITESCALETAG = nextAvailable;
		unsigned long WRITESCALETAG = nextAvailable;
		nextAvailable += 24;
	
		//Entry 12 - Model Tie Point Tag
		obj.tag = 33922;
		obj.type = 12;
		obj.count = 6;
		obj.offset = nextAvailable;
		writeIfd( obj);
		//BT unsigned long long WRITEMODEL = nextAvailable;
		unsigned long WRITEMODEL = nextAvailable;
		nextAvailable += 48;

		//GeoTIFF-GeoKeyDirectoryTag (GeoKey index w/data when small)
		unsigned long WRITEGEOKEY;
		if(filedata.geoKeySize > 0)
		{
			obj.tag=34735;
			obj.type=3;
			obj.offset=nextAvailable;
			obj.count=filedata.geoKeySize;  
			WRITEGEOKEY = nextAvailable;
			nextAvailable += obj.count * 2;  // 2 bytes per count since each is a short
			writeIfd(obj);
		}

		//GeoTiff-GeoDoubleParamsTag (GeoKey double data values)
		unsigned long WRITEGEODOUBLE;
		if(filedata.geoDoubleSize > 0)
		{
			obj.tag=34736;
			obj.type=12;
			obj.offset=nextAvailable;
			obj.count=filedata.geoDoubleSize;  
			WRITEGEODOUBLE = nextAvailable;
			nextAvailable += obj.count * 8; //8 bytes per count since each is a double
			writeIfd(obj);
		}

		//GeoTiff-GeoAsciiParamsTag (GeoKey ASCII data values)
		unsigned long WRITEGEOASCII;
		if(filedata.geoAsciiSize > 0)
		{
			obj.tag=34737;
			obj.type=2;
			obj.offset=nextAvailable;
			obj.count=filedata.geoAsciiSize;  
			WRITEGEOASCII = nextAvailable;
			nextAvailable += obj.count;
			writeIfd(obj);
		}

		//GDAL_ASCII Tag
		unsigned long WRITEGDALASCII;
		if(filedata.gdalAsciiSize > 0)
		{
			obj.tag=42112;
			obj.type=2;
			obj.offset=nextAvailable;
			obj.count=filedata.gdalAsciiSize;  
			WRITEGDALASCII = nextAvailable;
			nextAvailable += obj.count;
			writeIfd(obj);
		}

		//Entry 13 - GDAL_NODATA Tag
		obj.tag = 42113;
		obj.type = 2;
		//  DGT additions to handle datatypes
		if( datatype == SHORT_TYPE ) obj.count = 7;
		else if( datatype == LONG_TYPE ) obj.count = 12;
		else obj.count = 25;
		obj.offset = nextAvailable;
		writeIfd( obj);
		//BT unsigned long long WRITENODATA = nextAvailable;
		unsigned long WRITENODATA = nextAvailable;
		nextAvailable += obj.count;  // DGT modified from 25

		//Write Footer for this IFD
		//Footer consists of the offset for the next IFD, but since this is the only and last IFD, write 4 or 8 bytes of 0's
		if ( version = TIFF ) {
			//BT MPI_File_write( fh, (unsigned long long)0, 4, MPI_BYTE, &status);
			MPI_File_write( fh, (uint32_t)0, 4, MPI_BYTE, &status);  //DGT
		}
		else {
			MPI_File_write( fh, (uint64_t)0, 8, MPI_BYTE, &status);   //DGT
		}

		//Write Tag Data Blocks for Tags with more data than will fit in the tag (4 or 8 bytes depending on TIFF/BIGTIFF)
		//Write Data for Strip Offsets 		
		//DGT  Only do this for numOffsets > 1
		if(numOffsets > 1)
		{
			mpiOffset = WRITEOFFSETS;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET); 
			MPI_File_write( fh, dataOffsets, 4*numOffsets, MPI_BYTE, &status);
		
			//Write Data for Strip Byte Counts
			mpiOffset = WRITESIZES;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write( fh, sizeOffsets, 4*numOffsets, MPI_BYTE, &status);
		}
	
		//Write Data for Scale Tag
		mpiOffset = WRITESCALETAG;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		MPI_File_write( fh, &dx, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dy, 8, MPI_BYTE, &status);
		double dummy = 0;
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
	
		//Write Data for Model Tie Point Tag
		mpiOffset = WRITEMODEL;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &xleftedge, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &ytopedge, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);

		// Write Data for the GeoTIFF GeoKeyDirectoryTag tag
		if(filedata.geoKeySize > 0)
		{
			mpiOffset = WRITEGEOKEY;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		//	MPI_File_write( fh, &filedata.geoKeyDir, 2*filedata.geoKeySize, MPI_BYTE, &status);
			for( long i=0; i<filedata.geoKeySize; ++i) {
				MPI_File_write( fh, &filedata.geoKeyDir[i], 2, MPI_BYTE, &status);
				//printf("%d,",filedata.geoKeyDir[i]);
			}
			//printf("\n");
		}

		//  Write Data for the GeoTIFF GeoDoubleParamsTag tag
		if(filedata.geoDoubleSize > 0)
		{
			mpiOffset = WRITEGEODOUBLE;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write (fh, filedata.geoDoubleParams,(int)filedata.geoDoubleSize*8, MPI_BYTE, &status);
		}

		//  Write Data for the GeoTIFF GeoAsciiParamsTag tag
		if(filedata.geoAsciiSize > 0)
		{
			mpiOffset = WRITEGEOASCII;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write (fh, filedata.geoAscii,(int)filedata.geoAsciiSize, MPI_BYTE, &status);
		}

		//  Write GDALASCII tag
		if(filedata.gdalAsciiSize > 0)
		{
			mpiOffset = WRITEGDALASCII;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write (fh, filedata.gdalAscii,(int)filedata.gdalAsciiSize, MPI_BYTE, &status);
		}

		//Write Data for GDAL_NODATA Tag
		mpiOffset = WRITENODATA;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		if( datatype == SHORT_TYPE ) {
			char cnodata[7];
			sprintf(cnodata,"%06d\x0",*(short*)nodata); 
			MPI_File_write( fh, &cnodata, 7, MPI_BYTE, &status);
		}
		else if( datatype == LONG_TYPE) {
			char cnodata[12];
			sprintf(cnodata,"%011d\x0",*(long*)nodata); 
			MPI_File_write( fh, &cnodata, 12, MPI_BYTE, &status);
		}
		else if( datatype == FLOAT_TYPE) {
			char cnodata[25];
			//CPLString().Printf( "%.18g", dfNoData ).c_str() ); GDAL Code
			//sprintf(cnodata,"%-24.16Le\x0",*(float*)nodata); Kim's Code
			sprintf(cnodata,"%-24.16e\x0",*(float*)nodata);
			MPI_File_write( fh, &cnodata, 25, MPI_BYTE, &status);
		}
		MPI_File_close( &fh);

	}else{
		if(rank==0) {
			//int file_error = MPI_File_open( MCW, fname, MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
			int file_error = MPI_File_open( MPI_COMM_SELF , filename, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
		        // allow overwriting of existing file
			if(file_error != MPI_SUCCESS){
				printf("Error opening file %s.\n",filename);
				printf("Failed to create or open an existing file for writing in process 0.\n");
				MPI_Abort(MCW,22);
			}
			//nextAvailable is the offset of nextAvailable unallocated area in the output file
			//initially set it to the offset of the beginning of the 0th (first) IFD
			//BT unsigned long long nextAvailable = dataOffset + (totalX * totalY * dataSize);
			uint32_t nextAvailable = dataOffset + (totalX * totalY * dataSizeObj);  //DGT made uint32_t, was long
			//printf("DataOffset: %lu, totalX: %lu, totalY: %lu, dataSize: %d\n",dataOffset, totalX, totalY, dataSizeObj);
			//printf("Next available: %lu\n",nextAvailable);
			//fflush(stdout);
		
			//Write file header information
			//Write the byte-order value
			short endian = LITTLEENDIAN;
			MPI_File_write( fh, &endian, 2, MPI_BYTE, &status);
		
			//Write the tiff file identifier 
			short version = TIFF;
			MPI_File_write( fh, &version,2, MPI_BYTE, &status);

			//Write the offset of 0th (first and only) Image File Directory (IFD)
			//BT unsigned long long tableOffset = nextAvailable;
			uint32_t tableOffset = nextAvailable;  //DGT made uint32_t, was long
			nextAvailable += (numEntries * 12)+6; //update to offset of first Tag data value block by adding 12 bytes for each tag and 6 bytes for the IFD header and footer
			MPI_File_write( fh, &tableOffset, 4, MPI_BYTE, &status);

			//Go to beginning of 0th (first and only) IFD
			//Write Number of Directory Entries for 0th IFD
			MPI_File_seek( fh, tableOffset, MPI_SEEK_SET );
			MPI_File_write( fh, &numEntries, 2, MPI_BYTE, &status);
			
			ifd obj;

			//Write entries for Oth (first and only) IFD
			//Entry 0 - Image Width (columns)
			obj.tag = 256;
			obj.type = 3;
			obj.count = 1;
			obj.offset = totalX;
			writeIfd( obj);
	
			//Entry 1 - Image Length (rows)
			obj.tag = 257;
			obj.type = 3;
			obj.count = 1;
			obj.offset = totalY;
			writeIfd( obj);
	
			//Entry 2 - Bits (data size) per Sample
			//printf("dataSizeObj: %d.\n", dataSizeObj);
			obj.tag = 258;
			obj.type = 3;
			obj.count = 1;
			obj.offset = dataSizeObj * 8;
			writeIfd( obj);
	
			//Entry 3 - Compression (1=No Compression)
			obj.tag = 259;
			obj.type = 3;
			obj.count = 1;
			obj.offset = 1;
			writeIfd( obj);
	
			//Entry 4 - Photometric Interpretation (1=Black Is Zero)
			obj.tag = 262;
			obj.type = 3;
			obj.count = 1;
			obj.offset = 1;
			writeIfd( obj);
	
			//Entry 5 - Strip Offsets (pointers to beginning of strips)
			//DGT  This does not properly handle the case for only one strip
			obj.tag = 273;
			obj.type = 4;
			obj.count = numOffsets;
			//BT unsigned long long WRITEOFFSETS;  //  DGT insert to handle case for only one strip
			unsigned long WRITEOFFSETS;  //  DGT insert to handle case for only one strip
			if (numOffsets == 1)  
			{
				obj.offset=dataOffsets[0];
			}
			else
			{
				obj.offset = nextAvailable;
				WRITEOFFSETS = nextAvailable;
				nextAvailable += numOffsets*4;
			}
			writeIfd( obj);
			
			//Entry 6 - Samples per Pixel (always 1)
			obj.tag = 277;
			obj.type =3;
			obj.count = 1;
			obj.offset = 1;
			writeIfd( obj);
	
			//Entry 7 - Rows per Strip
			obj.tag = 278;
			obj.type =3;
			obj.count = 1;
			obj.offset = tileLength;
			writeIfd( obj);
	
			//Entry 8 - Strip Byte Counts
			//DGT  This does not properly handle the case for only one strip
			obj.tag = 279;
			obj.type = 4;
			obj.count = numOffsets;
			//BT unsigned long long WRITESIZES;   //  DGT changes to handle case for only one strip
			unsigned long WRITESIZES;   //  DGT changes to handle case for only one strip
			if(numOffsets == 1)
			{
				obj.offset=sizeOffsets[0];
			}
			else
			{
				obj.offset = nextAvailable;
				WRITESIZES = nextAvailable;
				nextAvailable += numOffsets*4;
			}
			writeIfd( obj);		
	
			//Entry 9 - Planar Configuration (always 1)
			obj.tag = 284;
			obj.type =3;
			obj.count = 1;
			obj.offset = 1;
			writeIfd( obj);
	
			//Entry 10 - Sample format ( 1=unsigned integer, 2=signed integer, 3=IEEE floating point )
			//Note that size/length of pixel is defined by the BitsPerSample field
			obj.tag = 339;
			obj.type =3;
			obj.count = 1;
			if( datatype == FLOAT_TYPE )
				obj.offset = 3;
			else obj.offset = 2;  // This assumes that all integer grids being written are signed
			writeIfd( obj);
		
			//Entry 11 - Scale Tag
			obj.tag = 33550;
			obj.type = 12;
			obj.count = 3;
			obj.offset = nextAvailable;
			writeIfd( obj);
			//BT unsigned long long WRITESCALETAG = nextAvailable;
			unsigned long WRITESCALETAG = nextAvailable;
			nextAvailable += 24;
	
			//Entry 12 - Model Tie Point Tag
			obj.tag = 33922;
			obj.type = 12;
			obj.count = 6;
			obj.offset = nextAvailable;
			writeIfd( obj);
			//BT unsigned long long WRITEMODEL = nextAvailable;
			unsigned long WRITEMODEL = nextAvailable;
			nextAvailable += 48;

			//GeoTIFF-GeoKeyDirectoryTag (GeoKey index w/data when small)
			unsigned long WRITEGEOKEY;
			if(filedata.geoKeySize > 0)
			{
				obj.tag=34735;
				obj.type=3;
				obj.offset=nextAvailable;
				obj.count=filedata.geoKeySize;  
				WRITEGEOKEY = nextAvailable;
				nextAvailable += obj.count * 2;  // 2 bytes per count since each is a short
				writeIfd(obj);
			}

			//GeoTiff-GeoDoubleParamsTag (GeoKey double data values)
			unsigned long WRITEGEODOUBLE;
			if(filedata.geoDoubleSize > 0)
			{
				obj.tag=34736;
				obj.type=12;
				obj.offset=nextAvailable;
				obj.count=filedata.geoDoubleSize;  
				WRITEGEODOUBLE = nextAvailable;
				nextAvailable += obj.count * 8; //8 bytes per count since each is a double
				writeIfd(obj);
			}

			//GeoTiff-GeoAsciiParamsTag (GeoKey ASCII data values)
			unsigned long WRITEGEOASCII;
			if(filedata.geoAsciiSize > 0)
			{
				obj.tag=34737;
				obj.type=2;
				obj.offset=nextAvailable;
				obj.count=filedata.geoAsciiSize;  
				WRITEGEOASCII = nextAvailable;
				nextAvailable += obj.count;
				writeIfd(obj);
			}

			//GDAL_ASCII Tag
			unsigned long WRITEGDALASCII;
			if(filedata.gdalAsciiSize > 0)
			{
				obj.tag=42112;
				obj.type=2;
				obj.offset=nextAvailable;
				obj.count=filedata.gdalAsciiSize;  
				WRITEGDALASCII = nextAvailable;
				nextAvailable += obj.count;
				writeIfd(obj);
			}
	
			//Entry 13 - GDAL_NODATA Tag
			obj.tag = 42113;
			obj.type = 2;
			//  DGT additions to handle datatypes
			if( datatype == SHORT_TYPE ) obj.count = 7;
			else if( datatype == LONG_TYPE ) obj.count = 12;
			else obj.count = 25;
			obj.offset = nextAvailable;
			writeIfd( obj);
			//BT unsigned long long WRITENODATA = nextAvailable;
			unsigned long WRITENODATA = nextAvailable;
			nextAvailable += obj.count;  // DGT modified from 25

			//Write Footer for this IFD
			//Footer consists of the offset for the next IFD, but since this is the only and last IFD, write 4 or 8 bytes of 0's
			if ( version = TIFF ) {
				//BT MPI_File_write( fh, (unsigned long long)0, 4, MPI_BYTE, &status);
				MPI_File_write( fh, (uint32_t)0, 4, MPI_BYTE, &status);  //DGT
			}
			else {
				MPI_File_write( fh, (uint64_t)0, 8, MPI_BYTE, &status);   //DGT
			}

			//Write Tag Data Blocks for Tags with more data than will fit in the tag (4 or 8 bytes depending on TIFF/BIGTIFF)
			//Write Data for Strip Offsets 		
			//DGT  Only do this for numOffsets > 1
			if(numOffsets > 1)
			{
				mpiOffset = WRITEOFFSETS;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET); 
				MPI_File_write( fh, dataOffsets, 4*numOffsets, MPI_BYTE, &status);
			
				//Write Data for Strip Byte Counts
				mpiOffset = WRITESIZES;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				MPI_File_write( fh, sizeOffsets, 4*numOffsets, MPI_BYTE, &status);
			}
	
			//Write Data for Scale Tag
			mpiOffset = WRITESCALETAG;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write( fh, &dx, 8, MPI_BYTE, &status);
			MPI_File_write( fh, &dy, 8, MPI_BYTE, &status);
			double dummy = 0;
			MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
	
			//Write Data for Model Tie Point Tag
			mpiOffset = WRITEMODEL;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
			MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
			MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
			MPI_File_write( fh, &xleftedge, 8, MPI_BYTE, &status);
			MPI_File_write( fh, &ytopedge, 8, MPI_BYTE, &status);
			MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);

			// Write Data for the GeoTIFF GeoKeyDirectoryTag tag
			if(filedata.geoKeySize > 0)
			{
				mpiOffset = WRITEGEOKEY;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				//MPI_File_write( fh, &filedata.geoKeyDir, 2*filedata.geoKeySize, MPI_BYTE, &status);
				for( long i=0; i<filedata.geoKeySize; ++i) {
					MPI_File_write( fh, &filedata.geoKeyDir[i], 2, MPI_BYTE, &status);
					//printf("%d,",filedata.geoKeyDir[i]);
				}
				//printf("\n");
			}

			//  Write Data for the GeoTIFF GeoDoubleParamsTag tag
			if(filedata.geoDoubleSize > 0)
			{
				mpiOffset = WRITEGEODOUBLE;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				MPI_File_write (fh, filedata.geoDoubleParams,(int)filedata.geoDoubleSize*8, MPI_BYTE, &status);
			}

			//  Write Data for the GeoTIFF GeoAsciiParamsTag tag
			if(filedata.geoAsciiSize > 0)
			{
				mpiOffset = WRITEGEOASCII;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				MPI_File_write (fh, filedata.geoAscii,(int)filedata.geoAsciiSize, MPI_BYTE, &status);
			}

			//  Write GDALASCII tag
			if(filedata.gdalAsciiSize > 0)
			{
				mpiOffset = WRITEGDALASCII;
				MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
				MPI_File_write (fh, filedata.gdalAscii,(int)filedata.gdalAsciiSize, MPI_BYTE, &status);
			}

			//Write Data for GDAL_NODATA Tag
			mpiOffset = WRITENODATA;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			if( datatype == SHORT_TYPE ) {
				char cnodata[7];
				sprintf(cnodata,"%06d\x0",*(short*)nodata); 
				MPI_File_write( fh, &cnodata, 7, MPI_BYTE, &status);
			}
			else if( datatype == LONG_TYPE) {
				char cnodata[12];
				sprintf(cnodata,"%011d\x0",*(long*)nodata); 
				MPI_File_write( fh, &cnodata, 12, MPI_BYTE, &status);
			}
			else if( datatype == FLOAT_TYPE) {
				char cnodata[25];
				//CPLString().Printf( "%.18g", dfNoData ).c_str() ); GDAL Code
				//sprintf(cnodata,"%-24.16Le\x0",*(float*)nodata); Kim's Code
				sprintf(cnodata,"%-24.16e\x0",*(float*)nodata);
				MPI_File_write( fh, &cnodata, 25, MPI_BYTE, &status);
			}
			MPI_File_close( &fh);
		}
	}
	MPI_Barrier(MCW);
	//  All processes have to get here
	int	file_error = MPI_File_open( MCW, filename, MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
	if(file_error != MPI_SUCCESS){
		printf("Error opening file %s.\n",filename);
		printf("Failed to open file that should have been created by process 0.\n");
		MPI_Abort(MCW,22);
	}	

	//Write data/image block
	//next set to srcOffest.  This gives the proper starting position to read from the src for writting. JJN
	long next = srcOffset;

	if( datatype == SHORT_TYPE ) {
		mpiOffset = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));//chng
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);

		//BT for(unsigned long i=0; i<numRows; i++) {
		for(long i=0; i<numRows; i++) {
			MPI_File_write( fh, (void*)(((short*)source)+next), numCols, MPI_SHORT, &status);
			mpiOffset += (numCols*dataSizeObj);
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			next+=DomainX;
		}
	}
	else if( datatype == LONG_TYPE ) {
		mpiOffset = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
//		printf("dataSizeObj %d\n",dataSizeObj);
		
/*  DGT This is ugly.  Internally we are using a long partition grid which in some implementations
    is 4 bytes and other 8 bytes.  ArcGIS and GDAL apparrently do not read 8 byte tiff's.
	We therefore coerce to int32_t which is fixed at 4 bytes.  There is not a corresponding MPI type 
	to use for writing so we write using MPI_BYTE.  This assumes that the byte order is correct.
*/
		int32_t* tempDataRow = new int32_t[numCols];
		for(long i=0; i<numRows; i++) {
			for(long k = 0; k < numCols; k++)
			{
					tempDataRow[k]=(int32_t) (*((long*)source+next+k));
					//  Here we are ignoring the possibility of typecast changing the no data value
					//  if it was outside the range of int32_t
			}
			MPI_File_write( fh, (void*)tempDataRow, numCols*dataSizeObj, MPI_BYTE, &status);
			mpiOffset += (numCols*dataSizeObj);
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			next+=DomainX;
		}

		delete tempDataRow;
	}
	else if( datatype == FLOAT_TYPE ){//set the write position in the file
		mpiOffset = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		//write each row into the file
		for(long i=0; i<numRows; i++) {
			//write a row of numCols(usualy less than gTotalX/DomainX
			MPI_File_write( fh, (void*)(((float*)source)+next), numCols, MPI_FLOAT, &status);
			//move the file pointer to the next row of the file.
			mpiOffset += (numCols*dataSizeObj);
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			//incriment the souce read location to the beggining of the next row.
			next+=DomainX;//number of Columns in the partition;
		}
	}
//	cout << "releasing";
	free(dataOffsets);
	free(sizeOffsets);
}
void tifFile::write(long xstart, long ystart, long numRows, long numCols, void* source) {

	MPI_Status status;
	MPI_Offset mpiOffset;

	//Calculate parameters needed by multiple tags
	//Calculate strip data offsets and sizes
	numOffsets = (totalY + tileLength - 1)/tileLength;
	//BT unsigned long long *dataOffsets;
	//BT unsigned long long *sizeOffsets;
	//BT dataOffsets = (unsigned long long*) malloc( 8 * numOffsets );
	//BT sizeOffsets = (unsigned long long*) malloc( 8 * numOffsets );
	//BT numOffsets = (totalY + tileLength - 1)/tileLength;
	uint32_t *dataOffsets;  //DGT
	uint32_t *sizeOffsets;  //DGT
	//SizeOf dataOffsets = (long*) malloc( 4 * numOffsets );
	//SizeOf sizeOffsets = (long*) malloc( 4 * numOffsets );
	dataOffsets = (uint32_t*) malloc( sizeof(uint32_t) * numOffsets );  //DGT
	sizeOffsets = (uint32_t*) malloc( sizeof(uint32_t) * numOffsets );  //DGT

	//BT unsigned long long sizeofStrip = tileLength * tileWidth * dataSize;
	//BT unsigned long long dataOffset = 8; //offset to beginning of image data block. Put it right after file header
	//BT for( unsigned long long i = 0; i< numOffsets; i++ ) {
	//  The below is hard coded for strips that inherit the tileLength of the copied object
	uint32_t sizeofStrip = tileLength * totalX * dataSizeObj;  // DGT made uint32_t, was long
	uint32_t dataOffset = 8; //offset to beginning of image data block. Put it right after file header  //DGT made uint32_t, was long
	for( long i = 0; i< numOffsets; i++ ) {
		sizeOffsets[i] = tileLength * totalX * dataSizeObj;
		dataOffsets[i] = dataOffset + (i * sizeofStrip);
	}
	// Recalculate size of final, potentially partial strip
	//DGT  Deleted dataOffset from line below
	//sizeOffsets[numOffsets-1] = dataOffset + ((totalY - (tileLength * (numOffsets-1))) * totalX * dataSize);
	sizeOffsets[numOffsets-1] = ((totalY - (tileLength * (numOffsets-1))) * totalX * dataSizeObj);
	//numEntries is the number of tags in the Oth (first and only) IFD
	//Consider implementing additional tags in the future: 34735 - 34797 and 42112

	numEntries = 14;
	//  DGT implementing spatial reference tag
	if(filedata.geoKeySize > 0)
	{
		numEntries++;
	}
	if(filedata.geoDoubleSize > 0)
	{
		numEntries++;
	}
	if(filedata.geoAsciiSize > 0)
	{
		numEntries++;
	}
	//  GDAL tag seems needed for ArcGIS to read GDAL spatial reference
	if(filedata.gdalAsciiSize > 0)
	{
		numEntries++;
	}

	//Write all of the file except the data/image block
	//This includes the file header, and all of the metadata, including the IFD header, footer, and data value blocks.
	//Only process 0 gets to write outsite the data/image block
	//if(rank==0) {
		//nextAvailable is the offset of nextAvailable unallocated area in the output file
		//initially set it to the offset of the beginning of the 0th (first) IFD
		//BT unsigned long long nextAvailable = dataOffset + (totalX * totalY * dataSize);
		uint32_t nextAvailable = dataOffset + (totalX * totalY * dataSizeObj);  //DGT made uint32_t, was long
		//printf("DataOffset: %lu, totalX: %lu, totalY: %lu, dataSize: %d\n",dataOffset, totalX, totalY, dataSizeObj);
		//printf("Next available: %lu\n",nextAvailable);
		//fflush(stdout);
	
		//Write file header information
		//Write the byte-order value
		short endian = LITTLEENDIAN;
		MPI_File_write( fh, &endian, 2, MPI_BYTE, &status);
	
		//Write the tiff file identifier 
		short version = TIFF;
		MPI_File_write( fh, &version,2, MPI_BYTE, &status);

		//Write the offset of 0th (first and only) Image File Directory (IFD)
		//BT unsigned long long tableOffset = nextAvailable;
		uint32_t tableOffset = nextAvailable;  //DGT made uint32_t, was long
		nextAvailable += (numEntries * 12)+6; //update to offset of first Tag data value block by adding 12 bytes for each tag and 6 bytes for the IFD header and footer
		MPI_File_write( fh, &tableOffset, 4, MPI_BYTE, &status);

		//Go to beginning of 0th (first and only) IFD
		//Write Number of Directory Entries for 0th IFD
		MPI_File_seek( fh, tableOffset, MPI_SEEK_SET );
		MPI_File_write( fh, &numEntries, 2, MPI_BYTE, &status);
		
		ifd obj;

		//Write entries for Oth (first and only) IFD
		//Entry 0 - Image Width (columns)
		obj.tag = 256;
		obj.type = 3;
		obj.count = 1;
		obj.offset = totalX;
		writeIfd( obj);
	
		//Entry 1 - Image Length (rows)
		obj.tag = 257;
		obj.type = 3;
		obj.count = 1;
		obj.offset = totalY;
		writeIfd( obj);
	
		//Entry 2 - Bits (data size) per Sample
		//printf("dataSizeObj: %d.\n", dataSizeObj);
		obj.tag = 258;
		obj.type = 3;
		obj.count = 1;
		obj.offset = dataSizeObj * 8;
		writeIfd( obj);
	
		//Entry 3 - Compression (1=No Compression)
		obj.tag = 259;
		obj.type = 3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 4 - Photometric Interpretation (1=Black Is Zero)
		obj.tag = 262;
		obj.type = 3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 5 - Strip Offsets (pointers to beginning of strips)
		//DGT  This does not properly handle the case for only one strip
		obj.tag = 273;
		obj.type = 4;
		obj.count = numOffsets;
		//BT unsigned long long WRITEOFFSETS;  //  DGT insert to handle case for only one strip
		unsigned long WRITEOFFSETS;  //  DGT insert to handle case for only one strip
		if (numOffsets == 1)  
		{
			obj.offset=dataOffsets[0];
		}
		else
		{
			obj.offset = nextAvailable;
			WRITEOFFSETS = nextAvailable;
			nextAvailable += numOffsets*4;
		}
		writeIfd( obj);
			
		//Entry 6 - Samples per Pixel (always 1)
		obj.tag = 277;
		obj.type =3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 7 - Rows per Strip
		obj.tag = 278;
		obj.type =3;
		obj.count = 1;
		obj.offset = tileLength;
		writeIfd( obj);
	
		//Entry 8 - Strip Byte Counts
		//DGT  This does not properly handle the case for only one strip
		obj.tag = 279;
		obj.type = 4;
		obj.count = numOffsets;
		//BT unsigned long long WRITESIZES;   //  DGT changes to handle case for only one strip
		unsigned long WRITESIZES;   //  DGT changes to handle case for only one strip
		if(numOffsets == 1)
		{
			obj.offset=sizeOffsets[0];
		}
		else
		{
			obj.offset = nextAvailable;
			WRITESIZES = nextAvailable;
			nextAvailable += numOffsets*4;
		}
		writeIfd( obj);		
	
		//Entry 9 - Planar Configuration (always 1)
		obj.tag = 284;
		obj.type =3;
		obj.count = 1;
		obj.offset = 1;
		writeIfd( obj);
	
		//Entry 10 - Sample format ( 1=unsigned integer, 2=signed integer, 3=IEEE floating point )
		//Note that size/length of pixel is defined by the BitsPerSample field
		obj.tag = 339;
		obj.type =3;
		obj.count = 1;
		if( datatype == FLOAT_TYPE )
			obj.offset = 3;
		else obj.offset = 2;  // This assumes that all integer grids being written are signed
		writeIfd( obj);
		
		//Entry 11 - Scale Tag
		obj.tag = 33550;
		obj.type = 12;
		obj.count = 3;
		obj.offset = nextAvailable;
		writeIfd( obj);
		//BT unsigned long long WRITESCALETAG = nextAvailable;
		unsigned long WRITESCALETAG = nextAvailable;
		nextAvailable += 24;
	
		//Entry 12 - Model Tie Point Tag
		obj.tag = 33922;
		obj.type = 12;
		obj.count = 6;
		obj.offset = nextAvailable;
		writeIfd( obj);
		//BT unsigned long long WRITEMODEL = nextAvailable;
		unsigned long WRITEMODEL = nextAvailable;
		nextAvailable += 48;

		//GeoTIFF-GeoKeyDirectoryTag (GeoKey index w/data when small)
		unsigned long WRITEGEOKEY;
		if(filedata.geoKeySize > 0)
		{
			obj.tag=34735;
			obj.type=3;
			obj.offset=nextAvailable;
			obj.count=filedata.geoKeySize;  
			WRITEGEOKEY = nextAvailable;
			nextAvailable += obj.count * 2;  // 2 bytes per geokey
			writeIfd(obj);
		}

		//GeoTiff-GeoDoubleParamsTag (GeoKey double data values)
		unsigned long WRITEGEODOUBLE;
		if(filedata.geoDoubleSize > 0)
		{
			obj.tag=34736;
			obj.type=12;
			obj.offset=nextAvailable;
			obj.count=filedata.geoDoubleSize;  
			WRITEGEODOUBLE = nextAvailable;
			nextAvailable += obj.count * 8; //8 bytes per count since each is a double
			writeIfd(obj);
		}

		//GeoTiff-GeoAsciiParamsTag (GeoKey ASCII data values)
		unsigned long WRITEGEOASCII;
		if(filedata.geoAsciiSize > 0)
		{
			obj.tag=34737;
			obj.type=2;
			obj.offset=nextAvailable;
			obj.count=filedata.geoAsciiSize;  
			WRITEGEOASCII = nextAvailable;
			nextAvailable += obj.count;
			writeIfd(obj);
		}

		//GDAL_ASCII Tag
		unsigned long WRITEGDALASCII;
		if(filedata.gdalAsciiSize > 0)
		{
			obj.tag=42112;
			obj.type=2;
			obj.offset=nextAvailable;
			obj.count=filedata.gdalAsciiSize;  
			WRITEGDALASCII = nextAvailable;
			nextAvailable += obj.count;
			writeIfd(obj);
		}

		//Entry 13 - GDAL_NODATA Tag
		obj.tag = 42113;
		obj.type = 2;
		//  DGT additions to handle datatypes
		if( datatype == SHORT_TYPE ) obj.count = 7;
		else if( datatype == LONG_TYPE ) obj.count = 12;
		else obj.count = 25;
		obj.offset = nextAvailable;
		writeIfd( obj);
		//BT unsigned long long WRITENODATA = nextAvailable;
		unsigned long WRITENODATA = nextAvailable;
		nextAvailable += obj.count;  // DGT modified from 25

		//Write Footer for this IFD
		//Footer consists of the offset for the next IFD, but since this is the only and last IFD, write 4 or 8 bytes of 0's
		if ( version = TIFF ) {
			//BT MPI_File_write( fh, (unsigned long long)0, 4, MPI_BYTE, &status);
			MPI_File_write( fh, (uint32_t)0, 4, MPI_BYTE, &status);  //DGT
		}
		else {
			MPI_File_write( fh, (uint64_t)0, 8, MPI_BYTE, &status);   //DGT
		}

		//Write Tag Data Blocks for Tags with more data than will fit in the tag (4 or 8 bytes depending on TIFF/BIGTIFF)
		//Write Data for Strip Offsets 		
		//DGT  Only do this for numOffsets > 1
		if(numOffsets > 1)
		{
			mpiOffset = WRITEOFFSETS;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET); 
			MPI_File_write( fh, dataOffsets, 4*numOffsets, MPI_BYTE, &status);
		
			//Write Data for Strip Byte Counts
			mpiOffset = WRITESIZES;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write( fh, sizeOffsets, 4*numOffsets, MPI_BYTE, &status);
		}
	
		//Write Data for Scale Tag
		mpiOffset = WRITESCALETAG;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		MPI_File_write( fh, &dx, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dy, 8, MPI_BYTE, &status);
		double dummy = 0;
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
	
		//Write Data for Model Tie Point Tag
		mpiOffset = WRITEMODEL;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &xleftedge, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &ytopedge, 8, MPI_BYTE, &status);
		MPI_File_write( fh, &dummy, 8, MPI_BYTE, &status);
	
		//Write Data for GDAL_NODATA Tag
		mpiOffset = WRITENODATA;
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		if( datatype == SHORT_TYPE ) {
			char cnodata[7];
			sprintf(cnodata,"%06d\x0",*(short*)nodata); 
			MPI_File_write( fh, &cnodata, 7, MPI_BYTE, &status);
		}
		else if( datatype == LONG_TYPE) {
			char cnodata[12];
			sprintf(cnodata,"%011d\x0",*(long*)nodata); 
			MPI_File_write( fh, &cnodata, 12, MPI_BYTE, &status);
		}
		else if( datatype == FLOAT_TYPE) {
			char cnodata[25];
			//CPLString().Printf( "%.18g", dfNoData ).c_str() ); GDAL Code
			//sprintf(cnodata,"%-24.16Le\x0",*(float*)nodata); Kim's Code
			sprintf(cnodata,"%-24.16e\x0",*(float*)nodata);
			MPI_File_write( fh, &cnodata, 25, MPI_BYTE, &status);
		}

		// Write GeoTIFF-GeoKeyDirectoryTag
		if(filedata.geoKeySize > 0)
		{
			mpiOffset = WRITEGEOKEY;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		//	MPI_File_write( fh, &filedata.geoKeyDir, 2*filedata.geoKeySize, MPI_BYTE, &status);
			for( long i=0; i<filedata.geoKeySize; ++i) {
				MPI_File_write( fh, &filedata.geoKeyDir[i], 2, MPI_BYTE, &status);
				//printf("%d,",filedata.geoKeyDir[i]);
			}
			//printf("\n");
		}

		//  Write GEOASCII tag
		if(filedata.geoAsciiSize > 0)
		{
			mpiOffset = WRITEGEOASCII;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write (fh, filedata.geoAscii,(int)filedata.geoAsciiSize, MPI_BYTE, &status);
		}
		//  Write GDALASCII tag
		if(filedata.gdalAsciiSize > 0)
		{
			mpiOffset = WRITEGDALASCII;
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			MPI_File_write (fh, filedata.gdalAscii,(int)filedata.gdalAsciiSize, MPI_BYTE, &status);
		}

//	}
	

	//Write data/image block
	//BT unsigned long long next =0;
	long next =0;

	if( datatype == SHORT_TYPE ) {
		mpiOffset = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);

		//BT for(unsigned long i=0; i<numRows; i++) {
		for(long i=0; i<numRows; i++) {
			MPI_File_write( fh, (void*)(((short*)source)+next), numCols, MPI_SHORT, &status);
			mpiOffset += (numCols*dataSizeObj);
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			next+=numCols;
		}
	}
	else if( datatype == LONG_TYPE ) {
		mpiOffset = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
//		printf("dataSizeObj %d\n",dataSizeObj);
		
/*  DGT This is ugly.  Internally we are using a long partition grid which in some implementations
    is 4 bytes and other 8 bytes.  ArcGIS and GDAL apparrently do not read 8 byte tiff's.
	We therefore coerce to int32_t which is fixed at 4 bytes.  There is not a corresponding MPI type 
	to use for writing so we write using MPI_BYTE.  This assumes that the byte order is correct.
*/
		int32_t* tempDataRow = new int32_t[numCols];
		for(long i=0; i<numRows; i++) {
			for(long k = 0; k < numCols; k++)
			{
					tempDataRow[k]=(int32_t) (*((long*)source+next+k));
					//  Here we are ignoring the possibility of typecast changing the no data value
					//  if it was outside the range of int32_t
			}
			MPI_File_write( fh, (void*)tempDataRow, numCols*dataSizeObj, MPI_BYTE, &status);
			mpiOffset += (numCols*dataSizeObj);
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			next+=numCols;
		}

		delete tempDataRow;
	}
	else if( datatype == FLOAT_TYPE ) {
		mpiOffset = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));
		//uint32_t check = dataOffset + (dataSizeObj*((totalX*ystart) + xstart));
		//if (check != mpiOffset)
		//{
		//	printf("TifFile offset overflow error: %ld, %ld\n",mpiOffset,check);
		//	fflush(stdout);
		//}
		MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		
		//BT for(unsigned long i=0; i<numRows; i++) {
		for(long i=0; i<numRows; i++) {
			MPI_File_write( fh, (void*)(((float*)source)+next), numCols, MPI_FLOAT, &status);
			mpiOffset += (numCols*dataSizeObj);
			//check += (numCols*dataSizeObj);
			//if (check != mpiOffset)
			//{
			//	printf("Offset overflow error 2nd posn: %ld, %ld\n",mpiOffset,check);
			//	fflush(stdout);
			//}
			MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
			next+=numCols;
		}

		//mpiOffset = dataOffset + (tiff->dataSizeObj*((totalCols*ystart) + xstart));
		//MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		//
		//for(int i=0; i<dy_grid; i++) {
		//	MPI_File_write( fh, &farr.d[next], dx_grid, MPI_FLOAT, &status);
		//	mpiOffset += (dx_grid*tiff->dataSizeObj);
		//	MPI_File_seek( fh, mpiOffset, MPI_SEEK_SET);
		//	next+=dx_grid;
		//}

	}
//	cout <<"releasing";
	free(dataOffsets);
	free(sizeOffsets);
}

bool tifFile::compareTiff(const tifFile &comp){
	double tol=0.0001;
	if(totalX != comp.totalX)
	{
		printf("Columns do not match: %d %d\n",totalX,comp.totalX);  
		return false;
	}
	if(totalY != comp.totalY)
	{
		printf("Rows do not match: %d %d\n",totalY,comp.totalY);  
		return false;
	}
	if(abs(dx-comp.dx) > tol) 
	{
		printf("dx does not match: %lf %lf\n",dx,comp.dx);  
		return false;
	}
	if(abs(dy-comp.dy) > tol) 
	{
		printf("dy does not match: %lf %lf\n",dy,comp.dy);  
		return false;
	}	
	if(abs(xleftedge - comp.xleftedge) > 0.0) 	
	{
		if(rank==0)
		{
			printf("Warning! Left edge does not match exactly:\n");
			printf(" %lf in file %s\n",xleftedge,filename);  
			printf(" %lf in file %s\n",comp.xleftedge,comp.filename);
		} 
		//return false;  //DGT decided to relax this to a warning as some TIFF files seem to use center or corner to reference edges in a way not understood 
	}
	if(abs(ytopedge - comp.ytopedge) > 0.0) 	
	{
		if(rank==0)
		{
			printf("Warning! Top edge does not match exactly:\n");
			printf(" %lf in file %s\n",ytopedge,filename);  
			printf(" %lf in file %s\n",comp.ytopedge,comp.filename);
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
void tifFile::setTotalXYtopYleftX(uint32_t x,uint32_t y, double xleft,double ytop){
	xllcenter = xleft + .5*dx;               
        yllcenter = ytop + y*dy - .5*dy;        
        xleftedge = xleft;
        ytopedge = ytop; 
	totalX = x;
	totalY = y;
	return;
}
void tifFile::readIfd(ifd &obj ) {
	MPI_Status status;
	MPI_File_read( fh, &obj.tag, 2, MPI_BYTE,&status);
	MPI_File_read( fh, &obj.type, 2, MPI_BYTE,&status);
	MPI_File_read( fh, &obj.count, 4, MPI_BYTE,&status);
	MPI_File_read( fh, &obj.offset, 4, MPI_BYTE,&status);
}

void tifFile::writeIfd(ifd &obj) {
	MPI_Status status;
	MPI_File_write(fh, &obj.tag, 2, MPI_BYTE, &status);
	MPI_File_write(fh, &obj.type, 2, MPI_BYTE, &status);
	MPI_File_write(fh, &obj.count, 4, MPI_BYTE, &status);
	MPI_File_write(fh, &obj.offset, 4, MPI_BYTE, &status);
}

void tifFile::printIfd(ifd obj) {
	printf("Tag: %hu\n",obj.tag);
	printf("Type: %hu\n",obj.type);
	printf("Value: %u\n",obj.count);
	printf("offset: %u\n",obj.offset);
}

void tifFile::geoToGlobalXY(double geoX, double geoY, int &globalX, int &globalY){
	//Original version had dx's intead of dy's - in case of future problems
//	int x0 = (int)(xllcenter - dx/2);
//	int y0 = (int)(yllcenter + dy/2);  // yllcenter actually is the y coordinate of the center of the upper left grid cell 
	globalX = (int)((geoX - xleftedge) / dx);
	globalY = (int)((ytopedge - geoY) / dy);
}
void tifFile::globalXYToGeo(long globalX, long globalY, double &geoX, double &geoY){
	geoX = xleftedge + dx/2. + globalX*dx;
	geoY = ytopedge - dy/2. - globalY*dy;  
}
void tifFile::printInfo(){

	printf("\t%s\n", filename);
	printf("tileLength: %d\n", tileLength);
	printf("tileWidth: %d\n", tileWidth);
	printf("numEntries: %d\n", numEntries);
	printf("totalX: %d\n", totalX);
	printf("totalY: %d\n", totalY);
	printf("dx: %lf\n", dx);
	printf("dy: %lf\n", dy);
	printf("xllcenter: %lf\n", xllcenter);
	printf("yllcenter: %lf\n", yllcenter);
	printf("xleftedge: %lf\n", xleftedge);
	printf("ytopedge: %lf\n", ytopedge);
	return;
}
