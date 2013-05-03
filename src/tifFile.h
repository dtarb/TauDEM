/*  Taudem tifFile header

  David Tarboton, Dan Watson, Kim Schreuders
  Utah State University  
  May 23, 2010
  
*/

/*  Copyright (C) 2009  David Tarboton, Utah State University

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

#ifndef TIFFILE_H
#define TIFFILE_H

#include <mpi.h>
#include <stdint.h>  //  see http://code.google.com/p/b-tk/issues/detail?id=30.  
//  As of VS 2010 MS provides stdint.h so can now use <> rather than "" and use system stdint.h 
// See http://en.wikipedia.org/wiki/Stdint.h for details.
//  See http://msdn.microsoft.com/en-us/library/36k2cdd4(v=vs.71).aspx for explanation of <> vs "" difference
#include "commonLib.h"

//Assumptions when using BIGTIFF - The BIGTIFF specification does not have these limitations, however this implementation does:
// - The width and the height of the grid will be no more than 2^32 (4G) cells in either dimension
// - No single TIFF strip or tile will contain more than 2^32 (4G) cells
// - No single linear partition strip (process) will contain more than 2^32 (4G) cells
// - No TIFF metadata tag may have more than 2^32 (4G) values

const short LITTLEENDIAN = 18761;
const short BIGENDIAN = 19789;
const short TIFF = 42;
const short BIGTIFF = 43;

struct geotiff{
	long xresNum;             //numerator for horizontal resolution 
	long xresDen;             //denominator for horizontal resolution
	long yresNum;             //numerator for vertical resolution
	long yresDen;             //denominator for horizontal resolution
	short planarConfig;       //irrelevant when SamplesPerPixel=1

	uint32_t geoKeySize;      
	uint32_t geoDoubleSize;  
	uint32_t geoAsciiSize;    
	uint32_t gdalAsciiSize;   

	uint16_t *geoKeyDir;      //pointer to extended geoKeyDir metadata
	double *geoDoubleParams;  //pointer to extended geoDoubleParams metadata
	char *geoAscii;           //pointer to extended geoAscii metadata
	char *gdalAscii;          //pointer to extended gdalAscii metadata

};

//Image File Directory (IFD) - TIFF Tag Structure
struct ifd {
	unsigned short tag;	//Tag ID#
	unsigned short type;	//Datatype of Values (TIFF datatypes, not C++)
	uint32_t count;	 	//Count of Values
	uint32_t offset;	//Values (if fits in 4 bytes for TIFF or 8 for BIGTIFF else Offset to Values)
};


class tifFile{
	protected:
		MPI_File fh;			//MPI file handle
		geotiff filedata;		//data from the GeoTIFF metadata tags
		ifd* ifds;			//pointer to the complete array of TIFF metadata tags
		short dataSizeFileIn;   	//#unsigned short BT - data value size of tiff file in bytes = BitsPerSample/8, not necessarily the same as the size of data in array
		short dataSizeObj;      	//#unsigned short BT - data value size of each element in the storage array and of the output tiff file in bytes
		short sampleFormat;     	//#unsigned short BT - data type of TIFF file: 1=unsigned interger, 2=signed interger, 3=float, 4=undefined
		uint32_t numOffsets;		//#DGT	//?? unsigned long BT - number of tiles/strips in the TIFF file, also the number of items in the numOffset and Bytes arrays
		uint32_t *offsets;		//DGT	//unsigned long long BT - pointer to the array of tile/strip offsets into the TIFF data
		short tileOrRow;       		//#unsigned short - NEED: TIFF 0=undefined, 1=tile, 2=row
		uint32_t tileLength;		//#DGT	// unsigned long - NEED: tile length
		uint32_t tileWidth;		//#DGT	// unsigned long - NEED: tile width
		int bytesize;
		uint32_t *bytes;		//DGT	//unsigned long BT - ??pointer into an array of the number of bytes in each TIFF tile or strip
		unsigned short numEntries;	//?? unsigned long - number of TIFF metadata tags in the Oth IFD (the only one we read/write)
		short version;			//TIFF Version Code, 42=TIFF/GeoTIFF, 43=BigTIFF
		int rank, size;			//MPI rank & size, rank=number for this process, size=number of processes
		uint32_t totalX;		//DGT	// unsigned long BT - ??width of entire grid in number of cells (all partitions)
		uint32_t totalY;		//DGT	// unsigned long BT - ??length of entire grid in number of cells (all partitions)
		double dx;			//??width of each cell
		double dy;			//??length of each cell
		double xllcenter;		//horizontal center point of lower left grid cell in grographic coordinates, not grid coordinates
		double yllcenter;		//vertical center point of lower left grid cell in geographic coordinates, not grid coordinates
		double xleftedge;		//horizontal coordinate of left edge of grid in geographic coordinates, not grid coordinates
		double ytopedge;		//vertical coordinate of top edge of grid in geographic coordinates, not grid coordinates
		DATA_TYPE datatype;		//datatype of the grid values and the nodata value: short, long, or float
		int nodatasize;
		int filenodatasize;
		void *nodata;			//pointer to the nodata value, the nodata value type is indicated by datatype
		void *filenodata;       	//pointer to no data value from the file.  This may be different from nodata because filedatatype and datatype are not equivalent 
		char filename[MAXLN];  		//  Save filename for error or warning writes
		double geoystart, geoyend, geoxstart, geoxend;//Used for multiple files.  Represents the Geographic X and Y values for this Tiff File. 
		uint32_t gystart, gyend, gxstart, gxend;//Used for multiple files.  Represents the Global/Domain X and Y values for this Tiff File. 
		bool nowrite;
//  Mappings


	public:
		tifFile(char *fname, DATA_TYPE newtype);
		tifFile(char *fname, DATA_TYPE newtype, void* nd, const tifFile &copy);
		tifFile(char *fname, DATA_TYPE newtype, void* nd, const tifFile &copy, bool nofile);
		~tifFile();

		void read(long xstart, long ystart, long numRows, long numCols, void* dest);
		void write(long xstart, long ystart, long numRows, long numCols, void* source);
		
		void read(long xstart, long ystart, long numRows, long numCols, void* dest, long partOffset, long DomainX);
		void write(long xstart, long ystart, long numRows, long numCols, void* source, long partOffset, long DomainX,bool mf);

		bool compareTiff(const tifFile &comp);
		void readIfd(ifd &obj);
		void writeIfd(ifd &obj);
		void printIfd(ifd obj);

		void geoToGlobalXY(double geoX, double geoY, int &globalX, int &globalY);
		void globalXYToGeo(long globalX, long globalY, double &geoX, double &geoY);

		double getgeoystart(){return geoystart;}
		double getgeoxstart(){return geoxstart;}
		double getgeoyend(){return geoyend;}
		double getgeoxend(){return geoxend;}
		void setgeoystart(double y){geoystart = y;}
		void setgeoxstart(double x){geoxstart = x;}
		void setgeoyend(double y){geoyend = y;}
		void setgeoxend(double x){geoxend = x;}

		uint32_t getgystart(){return gystart;}
		uint32_t getgxstart(){return gxstart;}
		uint32_t getgyend(){return gyend;}
		uint32_t getgxend(){return gxend;}
		void setgystart(uint32_t y){gystart = y;}
		void setgxstart(uint32_t x){gxstart = x;}
		void setgyend(uint32_t y){gyend = y;}
		void setgxend(uint32_t x){gxend = x;}

		void setTotalXYtopYleftX(uint32_t x,uint32_t y, double xleft,double ytop);

		uint32_t getTotalX(){return totalX;}
		uint32_t getTotalY(){return totalY;} 
		double getdx(){return dx;}
		double getdy(){return dy;}
		double getXllcenter(){return xllcenter;}
		double getYllcenter(){return yllcenter;}
		double getXLeftEdge(){return xleftedge;}
		double getYTopEdge(){return ytopedge;}
		DATA_TYPE getDatatype(){return datatype;}
		void* getNodata(){return nodata;}
		void printInfo();

};

#endif
