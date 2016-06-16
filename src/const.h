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
#ifndef CONST_H
#define CONST_H

#include <cfloat>
#include <climits>
#include <cmath>

#define MCW MPI_COMM_WORLD
#define MAX_STRING_LENGTH 255
#define MAXLN 4096

#define TDVERSION "5.3.5"

enum DATA_TYPE
	{ SHORT_TYPE,
	  LONG_TYPE,
	  FLOAT_TYPE,
	  DOUBLE_TYPE,
	  UNKNOWN_TYPE,
	  INVALID_DATA_TYPE = -1
	};

//TODO: revisit this structure to see where it is used
struct node {
	int x;
	int y;

    node() {}
	node(int x_, int y_): x(x_), y(y_) {}

	bool operator==(const node &n) const {
        return x == n.x && y == n.y;
    }

    bool operator<(const node &n) const {
        return y < n.y || (y == n.y && x < n.x);
    }
};

const double PI = 3.14159265359;
const short MISSINGSHORT = SHRT_MIN;
const long MISSINGLONG = LONG_MIN;
const float MISSINGFLOAT = -FLT_MAX;
const float MINEPS = 1E-5f;

const int d1[9] = { 0,1, 1, 0,-1,-1,-1,0,1};
const int d2[9] = { 0,0,-1,-1,-1, 0, 1,1,1};

//  TODO adjust this for different dx and dy
const double aref[10] = { -atan2(1.0, 1.0), 0., -aref[0], 0.5*PI, PI-aref[2], PI,
    PI+aref[2], 1.5*PI,2.0*PI-aref[2],2.*PI };   // DGT is concerned that this assumes square grids.  For different dx and dx needs adjustment
#endif
