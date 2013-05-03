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
#include <stdint.h>  //  see http://code.google.com/p/b-tk/issues/detail?id=30.  As of VS 2010 MS provides stdint.h so can now use <> 
// rather than "" and use system stdint.h // See http://en.wikipedia.org/wiki/Stdint.h for details.
#include "commonLib.h"
#ifdef  _MSC_VER  //  Microsoft compiler
#include "dirent.h"
#else
#include <dirent.h>
#endif

#include "tifFile.h"

class tiffIO{
	protected:
		int readwrite; 			// initialized to read only if = 0. if 1, initialized to write only.
		int rank, size;
		char dirn[NAME_MAX];
		DIR *dir;			// = NULL;
		struct dirent *drnt;		// = NULL;
		long numFiles;			// = 0;
		char **files;			//[numFiles][NAME_MAX];
		char file[NAME_MAX];		// = "./start/";
		tifFile **IOfiles;		//[numFiles];
		long gTotalX;			// = 0,								//Global Total X value;
		long gTotalY;			// Global Total Y value;
		long fileX;			// biggest files size number of columns
		long fileY;			// biggest files size number of rows
		double xleftedge;		//horizontal coordinate of left edge of grid in geographic coordinates, not grid coordinates
		double ytopedge;		//vertical coordinate of top edge of grid in geographic coordinates, not grid coordinates
		double xllcenter;
		double yllcenter;
		double xleft; 			// global xleft edge value
		double ytop;			//global ytop edge value
		double dx;
		double dy;						
		long xsize;			//x size of the output file
		long ysize;			//y size of the output file
		void* nodata;			//pointer to no data value from the file.  This may be different from nodata because filedatatype and datatype are not equivalent 
	//pointer to the nodata value, the nodata value type is indicated by datatype
		long PYS;			//Partition Y Start in global c counting
		long PYE;			//Partition Y End in global c counting
		long FYS;			//File Y Start in global c counting of file to be written
		long FYE;			//File Y End in global c counting of file to be written
		//PYS = Partition Y Start in golbal, PYE = Part Y End, FYS = File Y Start to be written, FYE = File Y End
		//char outName[NAME_MAX];		//the name of the output file.
		//char buff[33];									//a buffer to make the name of the output file with.
		char filename[MAXLN];  							//  Save filename for error or warning writes
		DATA_TYPE datatype;								//datatype of the grid values and the nodata value: short, long, or float

	public:
		tiffIO(char *dirname, DATA_TYPE newtype);
		tiffIO(char *dirname, DATA_TYPE newtype, void* nd, const tiffIO &copy);
		~tiffIO();
		//read and write functions
		void read(long xstart, long ystart, long numRows, long numCols, void* dest);
		void write(long xstart, long ystart, long numRows, long numCols, void* source,char prefix[],int prow, int pcol);
		
		//Analisys functions
		bool compareTiff(const tiffIO &comp);
		//Conversion functions
		void geoToGlobalXY(double geoX, double geoY, int &globalX, int &globalY);
		void globalXYToGeo(long globalX, long globalY, double &geoX, double &geoY);

		//Accessor functions
		uint32_t getTotalX(){return gTotalX;}  // DGT made uint32_t, was long
		uint32_t getTotalY(){return gTotalY;}  // DGT made uint32_t, was long
		double getdx(){return dx;}
		double getdy(){return dy;}
		DATA_TYPE getDatatype(){return datatype;}
		void* getNodata(){return nodata;}
		double getXllcenter(){return xllcenter;}
		double getYllcenter(){return yllcenter;}
};

#endif

