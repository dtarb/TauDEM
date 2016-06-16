/*  Taudem common function library header

  David Tarboton, Dan Watson
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
#ifndef COMMON_H
#define COMMON_H

#include <queue>

#include <mpi.h>
#include <gdal.h>
#include <ogr_api.h>

#include "const.h"
#include "linearpart.h"

//  TODO adjust this for different dx and dy
//const double aref[10] = { -atan2((double)1,(double)1), 0., -aref[0],(double)(0.5*PI),PI-aref[2],(double)PI,
                       // PI+aref[2],(double)(1.5*PI),2.*PI-aref[2],(double)(2.*PI) };   // DGT is concerned that this assumes square grids.  For different dx and dx needs adjustment

int nameadd(char* full, char* arg, const char* suff);
double prop(float a, int k, double dx1 , double dy1);
std::string getLayername(std::string input_path);

const char *getOGRdrivername(char *datasrcnew);
void getlayerfail(OGRDataSourceH hDS1,char * outletsds, int outletslyr);
int readoutlets(char *outletsds,char *lyrname,int uselayername, int outletslyr, OGRSpatialReferenceH hSRSRaster, int *noutlets, double*& x, double*& y);
int readoutlets(char *outletsds,char *lyrname,int uselayername, int outletslyr, OGRSpatialReferenceH  hSRSraster,int *noutlets, double*& x, double*& y, int*& id);

bool pointsToMe(long col, long row, long ncol, long nrow, tdpartition *dirData);

void initNeighborDinfup(tdpartition* neighbor,tdpartition* flowData, std::queue<node> *que,
					  int nx,int ny,int useOutlets, int *outletsX,int *outletsY,long numOutlets);
void initNeighborD8up(tdpartition* neighbor,tdpartition* flowData, std::queue<node> *que,
					  int nx,int ny,int useOutlets, int *outletsX,int *outletsY,long numOutlets); 

std::string humanReadableSize(uint64_t size);
#endif

