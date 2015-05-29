/*  Taudem tdpartition header 

  David Tarboton, Kim Schreuders, Dan Watson
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

//#include "commonLib.h"
#include <stdio.h>
#ifndef PARTITION_H
#define PARTITION_H
#include "tiffIO.h"
class tiffIO;
class tdpartition{
	protected:
		long totalx, totaly;
		long nx, ny;
		double dxA, dyA, *dxc,*dyc;
		

	public:
		tdpartition(){}
		virtual ~tdpartition(){}

		
		virtual bool isInPartition(int, int) = 0;
 		virtual bool hasAccess(int, int) = 0;
		virtual bool isNodata(long x, long y) = 0;
	    
		virtual void share() = 0;
		virtual void passBorders() = 0;
		virtual void addBorders() = 0;
		virtual void clearBorders() = 0;
		virtual int ringTerm(int isFinished) = 0;

		virtual bool globalToLocal(int globalX, int globalY, int &localX, int &localY) = 0;
		virtual void localToGlobal(int localX, int localY, int &globalX, int &globalY) = 0;

		virtual int getGridXY(int x, int y, int *i, int *j) = 0;
		virtual void transferPack(int*, int*, int*, int*) = 0;

		int getnx(){return nx;}
		int getny(){return ny;}
		int gettotalx(){return totalx;}
		int gettotaly(){return totaly;}
		double getdxA(){return dxA;}
		double getdyA(){return dyA;}

		int *before1;
		int *before2;
		int *after1;
		int *after2;
		
		//There are multiple copies of these functions so that classes that inherit
		//from tdpartition can be template classes.  These classes MUST declare as
		//their template type one of the types declared for these functions.
		virtual void* getGridPointer(){return (void*)NULL;}
		virtual void setToNodata(long x, long y) = 0;

		virtual void init(long totalx, long totaly, double dx_in, double dy_in, MPI_Datatype MPIt, short nd){}
		virtual void init(long totalx, long totaly, double dx_in, double dy_in ,MPI_Datatype MPIt, int32_t nd){}
		virtual void init(long totalx, long totaly, double dx_in, double dy_in, MPI_Datatype MPIt, float nd){}


		virtual short getData(long, long, short&){
			printf("Attempt to access short grid with incorrect data type\n");
			MPI_Abort(MCW,41);return 0;
		}
		virtual int32_t getData(long, long, int32_t&){
			printf("Attempt to access int32_t grid with incorrect data type\n");
			MPI_Abort(MCW,42);return 0;
		}
		virtual float getData(long, long, float&){
			printf("Attempt to access float grid with incorrect data type\n");
			MPI_Abort(MCW,43);return 0;
		}
	   

		virtual void savedxdyc(tiffIO &obj ){}
		virtual void getdxdyc(long, double&, double&){}
	    virtual void setData(long, long, short){}
		virtual void setData(long, long, int32_t){}
		virtual void setData(long, long, float){}

		virtual void addToData(long, long, short){}
		virtual void addToData(long, long, int32_t){}
		virtual void addToData(long, long, float){}
};
#endif


