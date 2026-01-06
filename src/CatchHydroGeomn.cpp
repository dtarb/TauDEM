/*  CatchHydroGeomn main program for function that evaluates channel 
  hydraulic properties based on HAND(Height Above Nearest Drainage), 
  D-inf slope, and catchment grid inputs, a stage height text file, 
  and a catchment id list file.

  David Tarboton, Xing Zheng
  Utah State University, University of Texas at Austin
  Nov 01, 2016 
  
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
#include "tardemlib.h"

int main(int argc,char **argv)
{
   char handfile[MAXLN] = "", catchfile[MAXLN] = "", catchlistfile[MAXLN] = "", slpfile[MAXLN] = "", hfile[MAXLN] = "", hpfile[MAXLN] = "";
   int err = 0;

   if(argc < 13) goto errexit;
   else
   {
        int i=1;
		while(argc > i)
		{
			if(strcmp(argv[i],"-hand")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(handfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-catch")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(catchfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-catchlist")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(catchlistfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-slp")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(slpfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if (strcmp(argv[i], "-h") == 0)
			{
				i++;
				if (argc > i)
				{
					strcpy(hfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if (strcmp(argv[i], "-table") == 0)
			{
				i++;
				if (argc > i)
				{
					strcpy(hpfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
		    else goto errexit;
		}
   }

   if (handfile[0] == '\0' || catchfile[0] == '\0' || catchlistfile[0] == '\0' || slpfile[0] == '\0' || hfile[0] == '\0' || hpfile[0] == '\0') goto errexit;

    if((err=catchhydrogeo(handfile, catchfile, catchlistfile, slpfile, hfile, hpfile)) != 0)
	{
        printf("Catchment Hydraulic Property Error %d\n",err);
	}
	if (err != 0) return err;
	return 0;
errexit:
	printf("Incorrect input.\n");
	printf("Use with specific file names:\n %s -hand <handfile>\n",argv[0]);
	printf("-catch <catchfile> -catchlist <catchidlistfile> -slp <slpfile> -h <hfile> -table <hpfile> \n");
	printf("<handfile> is the name of the input hand raster file.\n");
	printf("<catchfile> is the name of the input catchment raster file.\n");
	printf("<catchidlistfile> is the name of the input catchment id list CSV file. This file has at least 3 columns: catchment id, catchment slope, catchment length. Optionally, a 4th column for Manning's n can be provided.\n");
	printf("<slpfile> is the name of the input D-inf slope raster file.\n");
	printf("<hfile> is the name of the input stage table text file.\n");
	printf("<hpfile> is the name of the output hydraulic property text file.\n");

   return 1;
} 
