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
#include "shapelib/shapefil.h"
#include "commonLib.h"

//  Function to read outlets from a shapefile
int readoutlets(char *outletsfile, int *noutlets, double*& x, double*& y)
{
	SHPHandle shp;
	shp = SHPOpen(outletsfile, "rb");
	if (shp != NULL) {
		int nEntities = 0;
		int nShapeType = 0;
		SHPGetInfo(shp, &nEntities, &nShapeType, NULL, NULL );
		if (nShapeType != SHPT_POINT) {
			fprintf(stderr, "Outlets shapefile %s is not a point shapefile\n", outletsfile);
			fflush(stderr);
			return 1;
		}
		long p_size;
		long countPts = 0;
		for( int i=0; i<nEntities; i++) {
			SHPObject * shape = SHPReadObject(shp, i);
			countPts += shape->nVertices;
			SHPDestroyObject(shape);
		}
		x = new double[countPts];
		y = new double[countPts];
		int nxy=0;
		for( int i=0; i<nEntities; i++) {
			SHPObject * shape = SHPReadObject(shp, i);
			p_size = shape->nVertices;
			for( int j=0; j<p_size; j++) {
				x[nxy] = shape->padfX[j];
				y[nxy] = shape->padfY[j];
				nxy++;
			}
			SHPDestroyObject(shape);
		}
		*noutlets=nxy;
		SHPClose(shp);
		return 0;
	}
	else { 
		fprintf(stderr, "Error opening outlets shapefile %s.\n", outletsfile);
		fflush(stderr);
		return 1;
	}	
}

//  Function to read outlets from a shapefile - overloaded to return an array of integer ID's
int readoutlets(char *outletsfile, int *noutlets, double*& x, double*& y, int*& id)
{
	SHPHandle shp = SHPOpen(outletsfile, "rb");
	char dbffile[MAXLN];
	nameadd(dbffile, outletsfile, ".dbf");
	DBFHandle dbf = DBFOpen(dbffile, "rb");
	if ((shp != NULL) && (dbf != NULL)) {
		int nEntities = 0;
		int nShapeType = 0;
		SHPGetInfo(shp, &nEntities, &nShapeType, NULL, NULL );
		if (nShapeType != SHPT_POINT)
		{
			fprintf(stderr, "Outlets shapefile %s is not a point shapefile\n", outletsfile);
			fflush(stderr);
			return 1;
		}
	long p_size;
	long countPts = 0;

	int nfld = DBFGetFieldCount(dbf);
	int idfld = DBFGetFieldIndex(dbf, "id");
	for( int i=0; i<nEntities; i++) {
		SHPObject * shape = SHPReadObject(shp, i);
		countPts += shape->nVertices;
		SHPDestroyObject(shape);
	}
	x = new double[countPts];
	y = new double[countPts];
	if (idfld >= 0)
		id = new int[countPts];
    int nxy=0;
	for( int i=0; i<nEntities; i++) {
		SHPObject * shape = SHPReadObject(shp, i);
		p_size = shape->nVertices;
		for( int j=0; j<p_size; j++) {
			x[nxy] = shape->padfX[j];
			y[nxy] = shape->padfY[j];
			if (idfld >= 0)
			{
				id[nxy] = DBFReadIntegerAttribute(dbf, i, idfld);
			}
			nxy++;
		}
		SHPDestroyObject(shape);
	}
	*noutlets=nxy;
	SHPClose(shp);
	return 0;
	}
	else { 
		fprintf(stderr, "Error opening outlets shapefile: %s\n", outletsfile);
		fflush(stderr);
		return 1;
	}	
}
