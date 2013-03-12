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

#include <stdio.h>
#include <string.h>
#include "shapefile.h"


void stolower(char *s)
{
    for(; *s; s++)
        if(('A' <= *s) && (*s <= 'Z'))
            *s = 'a' + (*s - 'A');
}


//  Function to read outlets from a shapefile
int readoutlets(char *outletsfile, int *noutlets, double*& x, double*& y)
{
	shapefile sh;
	if( sh.open(outletsfile) ) {
	shape *shp;
	api_point point;
	long size = sh.size();
	long p_size;
	long countPts = 0;
	for( int i=0; i<size; i++) {
		shp = sh.getShape(i);
		countPts += shp->size();
	}
	x = new double[countPts];
	y = new double[countPts];
    int nxy=0;
	for( int i=0; i<size; i++) {
		shp = sh.getShape(i);
		p_size = shp->size();
		for( int j=0; j<p_size; j++) {
			x[nxy] = shp->getPoint(j).getX();
			y[nxy] = shp->getPoint(j).getY();
			nxy++;
		}	
	}
	*noutlets=nxy;
	sh.close();
//	delete shp;
//	delete &sh;
	return 0;
	}
	else { 
		printf("Error opening shapefile.\n");
		return 1;
	}	
}

//  Function to read outlets from a shapefile - overloaded to return an array of integer ID's
int readoutlets(char *outletsfile, int *noutlets, double*& x, double*& y, int*& id)
{
	shapefile sh;
	if( sh.open(outletsfile) ) {
	shape *shp;
	api_point point;
	long size = sh.size();
	long p_size;
	long countPts = 0;
	int nfld=sh.numberFields();
	int idfld;
	field f;
	for(int k=0; k<nfld; k++)
	{
		f=sh.getField(k);
		char *name;
		name=f.getFieldName();
		stolower(name);
		if(strcmp(name,"id")==0)
		{
			idfld=k;
			break;
		}
	}
	for( int i=0; i<size; i++) {
		shp = sh.getShape(i);
		countPts += shp->size();
	}
	x = new double[countPts];
	y = new double[countPts];
	id=new int[countPts];
    int nxy=0;
	for( int i=0; i<size; i++) {
		shp = sh.getShape(i);
		p_size = shp->size();
		for( int j=0; j<p_size; j++) {
			x[nxy] = shp->getPoint(j).getX();
			y[nxy] = shp->getPoint(j).getY();
			cell v;
			v=shp->getCell(idfld);
			id[nxy]=v.IntegerValue();
			nxy++;
		}	
	}
	*noutlets=nxy;
	sh.close();
	return 0;
	}
	else { 
		printf("Error opening shapefile: %s\n", outletsfile);
		return 1;
	}	
}
