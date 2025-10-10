/*
  The Flowdircond tool adjusts elevation values in a DEM to ensure that elevations along specified D8 flow directions
  do not increase (move uphill). It is designed to operate only on grid cells representing a stream network,
  where flow directions are known and provided. For each stream cell, the algorithm recursively traces downstream along
  the D8 flow directions, lowering elevations as needed to enforce a monotonic downhill path.
  Non-stream cells (marked as NoData or having flow directions outside the valid range 1:8 in the flow direction input)
  remain unchanged. This “soft burning” process etches hydrography into the DEM, improving hydrologic consistency
  and supporting applications such as flood inundation mapping using height above nearest drainage.

  David Tarboton, Nazmus Sazib
  Utah State University
  May 10, 2016
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

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "commonLib.h"
#include "d8.h"

int flowdircond( char *pfile, char *zfile, char *zfdcfile);
int main(int argc,char **argv)
{
  	char pfile[MAXLN], zfile[MAXLN], zfdcfile[MAXLN];
  	int err, i;

	if(argc < 2)
	{
		printf("Error: To run this program, use either the Simple Usage option or\n");
		printf("the Usage with Specific file names option\n");
		goto errexit; 
	}
	else if(argc > 2)
	{
		i = 1;
	}
	else {
		i = 2;
	}
	while(argc > i)
	{
		if(strcmp(argv[i],"-z")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(zfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-p")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(pfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-zfdc")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(zfdcfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}
	if( argc == 2) {
		nameadd(zfile,argv[1],"z");
		nameadd(pfile,argv[1],"p");
		nameadd(zfdcfile,argv[1],"zfdc");	
	}

	if((err=flowdircond(pfile, zfile, zfdcfile)) != 0)
		printf("flowdiircond error %d\n",err);

	return 0;

errexit:
	printf("Use with specific file names:\n %s -p <pfile>\n",argv[0]);
	printf("-z <zfile> -zfdc <zfdcfile> \n");
	printf("<pfile> is the name of the input D8 flow direction raster file.\n");
	printf("<zfile> is the name of the input elevation raster file.\n");
	printf("<zfdcfile> is the name of the output conditioned elevation raster file.\n");
	return 0;
}