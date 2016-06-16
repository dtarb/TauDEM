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
#ifndef PARTITION_H
#define PARTITION_H

#include <mpi.h>
#include <cstdio>

#include "tiffIO.h"

class tiffIO;

enum DecompType {
    DECOMP_BLOCK,
    DECOMP_ROW,
    DECOMP_COLUMN
};

class tdpartition{
    protected:
        int totalx, totaly;
        int nx, ny;
        int cx, cy;
        double dxA, dyA, *dxc, *dyc;

    public:
        static DecompType decompType;

        tdpartition(){}
        virtual ~tdpartition(){}

        virtual bool isInPartition(int x, int y) const = 0;
        virtual bool hasAccess(int x, int y) const = 0;
        virtual bool isNodata(int x, int y) const = 0;

        virtual void share() = 0;
        virtual void passBorders() = 0;
        virtual void addBorders() = 0;
        virtual void clearBorders() = 0;
        virtual int ringTerm(int isFinished) = 0;

        virtual bool globalToLocal(int globalX, int globalY, int &localX, int &localY) = 0;
        virtual void localToGlobal(int localX, int localY, int &globalX, int &globalY) = 0;

        virtual bool hasLeftNeighbour() = 0;
        virtual bool hasRightNeighbour() = 0;
        virtual bool hasTopNeighbour() = 0;
        virtual bool hasBottomNeighbour() = 0;
        virtual bool hasTopLeftNeighbour() = 0;
        virtual bool hasTopRightNeighbour() = 0;
        virtual bool hasBottomLeftNeighbour() = 0;
        virtual bool hasBottomRightNeighbour() = 0;

        virtual int getNeighbourCount() = 0;

        virtual void transferPack(int**, int**) = 0;

        int getnx() const {return nx;}
        int getny() const {return ny;}
        int gettotalx() const {return totalx;}
        int gettotaly() const {return totaly;}
        double getdxA() const {return dxA;}
        double getdyA() const {return dyA;}

        //There are multiple copies of these functions so that classes that inherit
        //from tdpartition can be template classes.  These classes MUST declare as
        //their template type one of the types declared for these functions.
        virtual void* getGridPointer() const {return (void*)NULL;}
        virtual void setToNodata(int x, int y) = 0;

        virtual void init(long totalx, long totaly, double dx_in, double dy_in, MPI_Datatype MPIt, short nd){}
        virtual void init(long totalx, long totaly, double dx_in, double dy_in ,MPI_Datatype MPIt, int32_t nd){}
        virtual void init(long totalx, long totaly, double dx_in, double dy_in, MPI_Datatype MPIt, float nd){}

        virtual short getData(int, int, short&) const {
            printf("Attempt to access short grid with incorrect data type\n");
            MPI_Abort(MPI_COMM_WORLD,41);
            return 0;
        }

        virtual int32_t getData(int, int, int32_t&) const {
             printf("Attempt to access int32_t grid with incorrect data type\n");
            MPI_Abort(MPI_COMM_WORLD,42);return 0;
        }

        virtual float getData(int, int, float&) const {
            printf("Attempt to access float grid with incorrect data type\n");
            MPI_Abort(MPI_COMM_WORLD,43);
            return 0;
        }

        virtual void savedxdyc(tiffIO &obj){}
        virtual void getdxdyc(int, double&, double&){}
        virtual void setData(int, int, short){}
        virtual void setData(int, int, int32_t){}
        virtual void setData(int, int, float){}

        virtual void addToData(int, int, short){}
        virtual void addToData(int, int, int32_t){}
        virtual void addToData(int, int, float){}
};

#endif

