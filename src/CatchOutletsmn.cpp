/*  CatchOutlets main program to derive catchment outlets at the downstream end of each stream reach.
     
  David Tarboton
  Utah State University  
  October 22, 2016
  
This function to take as input tree.dat, coord.dat, D8 flow directions *p.tif and a distance threshold.
To write an outlets shapefile that is the downstream end of each stream segment to be used in rapid watershed delineation functions.

Additional things it should do

1. Where the outlet is a downstream end move one cell down to have the catchment extend into downstream catchments to facilitate internally draining sinks
2. Where an outlet is within the threshold distance of the downstream end it is not written to thin the number of outlets and partitions that will be created.

*/

/*  Copyright (C) 2016  David Tarboton, Utah State University

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
//#include "shapelib/shapefil.h"
#include "CatchOutlets.h"

int main(int argc,char **argv)
{
   char pfile[MAXLN],streamnetsrc[MAXLN], streamnetlyr[MAXLN]="",outletsdatasrc[MAXLN],outletslayer[MAXLN]="";
   double mindist = 0.0;
   double minarea = 0.0;
   int err,i,uselyrname=0,lyrno=0,gwstartno=1;
   
   if(argc < 7)
    {  
       printf("To few input arguments.\n");
	   goto errexit;
    }
		i = 1;
//		printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	while(argc > i)
	{
		if(strcmp(argv[i],"-p")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(pfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-net") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(streamnetsrc, argv[i]);
				i++;
			}
			else goto errexit;
		}

/*		else if (strcmp(argv[i], "-netlyr") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(streamnetlyr, argv[i]);
				i++;
			}
			else goto errexit;
		} */  //  For initial work no layers
		else if(strcmp(argv[i],"-o")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletsdatasrc,argv[i]);
				i++;											
			}
			else goto errexit;
		}
/*		else if (strcmp(argv[i], "-lyrno") == 0)
		{
			i++;
			if (argc > i)
			{
				sscanf(argv[i], "%d", &lyrno);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-lyrname")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletslayer,argv[i]);
		        uselyrname = 1;
				i++;											
			}
			else goto errexit;
		}  */
		else if (strcmp(argv[i], "-gwstartno") == 0)
		{
			i++;
			if (argc > i)
			{
				sscanf(argv[i], "%d", &gwstartno);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-mindist")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%lf",&mindist);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-minarea") == 0)
		{
			i++;
			if (argc > i)
			{
				sscanf(argv[i], "%lf", &minarea);
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}
    if(err=catchoutlets(pfile,streamnetsrc,outletsdatasrc,mindist,gwstartno,minarea) != 0)
       printf("Catchment Outlets error %d\n",err);

	return 0;
errexit:
	   printf("Incorrect input.\n");
	   printf("Use with specific file names:\n %s -p <pfile>\n",argv[0]);
       printf("-net <streamnet source> [-netlyr <stream net layer>] -o <outletsshapefile>\n");
       printf("[-lyrno <layer number for outlet> -lyrname <layer name for outlet> -mindist <Min distance between outlets>]\n");
	   printf("<pfile> is the name of the D8 flow direction file (input).\n");
	   printf("<stream net source> is the name of the stream network shapefile data source (input).\n");
	   printf("<stream net layer> is the name of the stream network shapefile data layer (optional input).\n");
	   printf("<outletsshapefile> is the name of the outlet shape file (input).\n");
	   printf("<layer number> is the layer number for the outlets layer in the shapefile as a data source (output).\n");
	   printf("<layer name> is the layer name for the outlets layer in the shapefile as a data source (output).\n");
	   printf("<mindist> is the minimum distance between outlets (input).\n");
	   printf("Default <mindist> is 0 if not input.\n");
       exit(0);
} 

