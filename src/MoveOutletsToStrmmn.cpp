/*  MoveOutletsToStrmmn main program to move outlets to a stream.
     
  David Tarboton, Teklu Tesfa, Dan Watson
  Utah State University  
  May 23, 2010 
  

  This function moves outlet point that are off a stream raster grid down D8 flow directions 
  until a stream raster grid is encountered.  Input is a flow direction grid, stream raster grid 
  and outlets shapefile.  Output is a new outlets shapefile where each point has been moved to 
  coincide with the stream raster grid if possible.  A field 'dist_moved' is added to the new 
  outlets shapefile to indicate the changes made to each point.  Points that are already on the 
  stream raster (src) grid are not moved and their 'dist_moved' field is assigned a value 0.  
  Points that are initially not on the stream raster grid are moved by sliding them along D8 
  flow directions until one of the following occurs:
  a.	A stream raster grid cell is encountered before traversing the max_dist number of grid cells.  
   The point is moved and 'dist_moved' field is assigned a value indicating how many grid cells the 
   point was moved.
  b.	More thanthe max_number of grid cells are traversed, or the traversal ends up going out of 
  the domain (encountering a no data D8 flow direction value).  The point is not moved and the 
  'dist_moved' field is assigned a value of -1.

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
//#include "shapelib/shapefil.h"
#include "MoveOutletsToStrm.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;

int main(int argc,char **argv)
{
   char pfile[MAXLN],srcfile[MAXLN],outletsdatasrc[MAXLN],outletslayer[MAXLN],outletmoveddatasrc[MAXLN],outletmovedlayer[MAXLN]="";
   int err,i,maxdist=50,uselyrname=0,lyrno=0;
   
   if(argc < 9)
    {  
       printf("No simple use case for this function.\n");
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
		else if(strcmp(argv[i],"-src")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(srcfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
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
                else if(strcmp(argv[i],"-ddm")==0)
		{
			i++;
			if(argc > i)
			{
				if(strcmp(argv[i],"row")==0) {
                                    tdpartition::decompType = DECOMP_ROW;
                                } else if (strcmp(argv[i],"column")==0) {
                                    tdpartition::decompType = DECOMP_COLUMN;
                                } else if (strcmp(argv[i],"block")==0) {
                                    tdpartition::decompType = DECOMP_BLOCK;
                                } else {
                                    goto errexit;
                                }
				i++;
			}
			else goto errexit;
		}
                else if(strcmp(argv[i],"-lyrno")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%d",&lyrno);
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
		}


		else if(strcmp(argv[i],"-om")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletmoveddatasrc,argv[i]);
				i++;
			}
			else goto errexit;
		}


		else if(strcmp(argv[i],"-omlyr")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletmovedlayer,argv[i]);
				i++;
			}
			else goto errexit;
		}

		else if(strcmp(argv[i],"-md")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%d",&maxdist);
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}
    if(err=outletstosrc(pfile,srcfile,outletsdatasrc,outletslayer,uselyrname,lyrno,outletmoveddatasrc,outletmovedlayer,maxdist) != 0)
       printf("Move outlets to stream error %d\n",err);

	return 0;
errexit:
	   printf("Incorrect input.\n");
	   printf("Use with specific file names:\n %s -p <pfile>\n",argv[0]);
       printf("-src <srcfile> -o <outletsshapefile> -om <outletsmoved>\n");
       printf("[-md <max dist>] [-ddm <ddm>]\n");
	   printf("<pfile> is the name of the D8 flow direction file (input).\n");
	   printf("<srcfile> is the name of the stream raster file (input).\n");
	   printf("<outletsshapefile> is the name of the outlet shape file (input).\n");
	   printf("<outletsmoved> is the name of the moved outlet shape file (output).\n");
	   printf("<max dist> is the maximum number of grid cells to move an outlet (input).\n");
	   printf("Default <max dist> is 50 if not input.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
       exit(0);
} 

