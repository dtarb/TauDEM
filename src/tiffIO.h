/*  Taudem tiffio header

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

#ifndef TIFFIO_H
#define TIFFIO_H

#include <mpi.h>
#include <stdint.h>  //  see http://code.google.com/p/b-tk/issues/detail?id=30.  
//As of VS 2010 MS provides stdint.h so can now use <> rather than "" and use system stdint.h 
// See http://en.wikipedia.org/wiki/Stdint.h for details.
//  See http://msdn.microsoft.com/en-us/library/36k2cdd4(v=vs.71).aspx for explanation of difference
#include "commonLib.h"
#include <gdal.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>


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
	long xresNum;              //??? unsigned long BT - numerator for horizontal resolution 
	long xresDen;              //??? unsigned long BT - denominator for horizontal resolution
	long yresNum;              //??? unsigned long BT - numerator for vertical resolution
	long yresDen;              //??? unsigned long BT - denominator for horizontal resolution
	short planarConfig;        //??? unsigned short BT - irrelevant when SamplesPerPixel=1

	uint32_t geoKeySize;      	//  DGT made uint32_t	//??? unsigned long long BT
	uint32_t geoDoubleSize;   	//  DGT made uint32_t	//??? unsigned long long BT
	uint32_t geoAsciiSize;    	//  DGT made uint32_t	//??? unsigned long long BT
	uint32_t gdalAsciiSize;   	//  DGT made uint32_t	//??? unsigned long long BT

	uint16_t *geoKeyDir;        //  DGT made uint16_t  	// unsigned short BT - pointer to extended geoKeyDir metadata
	double *geoDoubleParams;    //pointer to extended geoDoubleParams metadata
	char *geoAscii;             //pointer to extended geoAscii metadata
	char *gdalAscii;            //pointer to extended gdalAscii metadata

};

//Image File Directory (IFD) - TIFF Tag Structure
struct ifd {
	unsigned short tag;			//Tag ID#
	unsigned short type;		//Datatype of Values (TIFF datatypes, not C++)
	uint32_t count;	  //DGT	//Count of Values
	uint32_t offset;	//DGT	// unsigned long long BT - Values (if fits in 4 bytes for TIFF or 8 for BIGTIFF else Offset to Values)
};

//  Parameters for WGS84 assumed for all geographic coordinates
const double elipa=6378137.000;
const double elipb=6356752.314;
const double boa=elipb/elipa;
class tiffIO{
	private:
		GDALDatasetH fh;			//gdal file handle
                GDALDatasetH copyfh;
                int isFileInititialized;
                GDALDriverH hDriver;
                GDALRasterBandH bandh;    
				// gdal band
		int rank, size;			//MPI rank & size, rank=number for this process, size=number of processes
		uint32_t totalX;		//DGT	// unsigned long BT - ??width of entire grid in number of cells (all partitions)
		uint32_t totalY;		//DGT	// unsigned long BT - ??length of entire grid in number of cells (all partitions)
		//double dx;				//??width of each cell
		//double dy;				//??length of each cell
                double xllcenter;		//horizontal center point of lower left grid cell in grographic coordinates, not grid coordinates
		double yllcenter;		//vertical center point of lower left grid cell in geographic coordinates, not grid coordinates
		double xleftedge;		//horizontal coordinate of left edge of grid in geographic coordinates, not grid coordinates
		double ytopedge;		//vertical coordinate of top edge of grid in geographic coordinates, not grid coordinates
		DATA_TYPE datatype;		//datatype of the grid values and the nodata value: short, long, or float
		void *nodata;			//pointer to the nodata value, the nodata value type is indicated by datatype
		void *filenodata;       //pointer to no data value from the file.  This may be different from nodata because filedatatype and datatype are not equivalent 
		char filename[MAXLN];  //  Save filename for error or warning writes
		const char *valueUnit; //value units
		double *Yp;
		double *dxc,*dyc;
	    double dxA,dyA,dlat,dlon,xleftedge_g,ytopedge_g,xllcenter_g,yllcenter_g;
	    int IsGeographic;
		OGRSpatialReferenceH  hSRS;
		
//  Mappings


	public:
		tiffIO(char *fname, DATA_TYPE newtype);
		tiffIO(char *fname, DATA_TYPE newtype, void* nd, const tiffIO &copy);
		~tiffIO();

		//BT void read(unsigned long long xstart, unsigned long long ystart, unsigned long long numRows, unsigned long long numCols, void* dest);
		//BT void write(unsigned long long xstart, unsigned long long ystart, unsigned long long numRows, unsigned long long numCols, void* source);
		void read(long xstart, long ystart, long numRows, long numCols, void* dest);
		void write(long xstart, long ystart, long numRows, long numCols, void* source);

		bool compareTiff(const tiffIO &comp);
				
		//void geoToGlobalXY(double geoX, double geoY, unsigned long long &globalX, unsigned long long &globalY);
		//void globalXYToGeo(unsigned long long globalX, unsigned long long globalY, double &geoX, double &geoY);
		// geoToGlobalXY_real(double geoX, double geoY, int &globalX, int &globalY);
		void geoToGlobalXY(double geoX, double geoY, int &globalX, int &globalY);
		void globalXYToGeo(long globalX, long globalY, double &geoX, double &geoY);
	
		void geotoLength(double dlon,double dlat, double lat, double *xyc);
		
	
		uint32_t getTotalX(){return totalX;}  // DGT made uint32_t, was long
		uint32_t getTotalY(){return totalY;}  // DGT made uint32_t, was long
	

		double getdyc(int index) {
			if (index < 0 || index >= totalY)
				return -1;

			return dyc[index];
		}

		double getdxc(int index) {
			if (index < 0 || index >= totalY)
				return -1;

			return dxc[index];
		}

		double getdxA() { return fabs(dxc[totalY/2]); }
		double getdyA() { return fabs(dyc[totalY/2]); }
		double getdlon() {return dlon;}
		double getdlat() {return dlat;}
		int getproj() {return IsGeographic;}
	
		
        double getXllcenter(){return xllcenter;}
		double getYllcenter(){return yllcenter;}
	
		OGRSpatialReferenceH getspatialref(){return hSRS;} // return projection information for raster file
		double getXLeftEdge(){return xleftedge;}
		double getYTopEdge(){return ytopedge;}
	    DATA_TYPE getDatatype(){return datatype;}
		void* getNodata(){return nodata;}
};

#endif
