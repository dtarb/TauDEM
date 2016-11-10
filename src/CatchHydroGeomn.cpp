/*  CatchHydroGeomn main program for function that evaluates channel 
  hydraulic properties based on HAND(Height Above Nearest Drainage), 
  D-inf slope, and catchment grid inputs, a stage height text file, 
  and a parameter nmax.

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
   char handfile[MAXLN], catchfile[MAXLN], catchlistfile[MAXLN], slpfile[MAXLN], hfile[MAXLN], hpfile[MAXLN];
   int err, nmax;
      
   if(argc < 2) goto errexit;
/*   if(argc == 2)
	{
//		printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(plenfile,argv[1],"plen");
		nameadd(ad8file,argv[1],"ad8");
		nameadd(ssfile,argv[1],"ss");
    }*/
   if(argc > 2)
   {
//		printf("You are running %s with the specific file names option.\n", argv[0]);
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
/*
			else if (strcmp(argv[i], "-nmax") == 0)
			{
				i++;
				if (argc > i)
				{
					sscanf(argv[i], "%d", &nmax);
					i++;
				}
				else goto errexit;
			}
*/
		    else goto errexit;
		}
   }
    if((err=catchhydrogeo(handfile, catchfile, catchlistfile, slpfile, hfile, hpfile)) != 0)
        printf("Catchment Hydraulic Property Error %d\n",err);

	return 0;
errexit:
//   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -hand <handfile>\n",argv[0]);
   printf("-catch <catchfile> -catchlist <catchidlistfile> -slp <slpfile> -h <hfile> -table <hpfile> \n");
//   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'plen', 'ad8' and 'ss' will be appended. \n");
   printf("<handfile> is the name of the input hand raster file.\n");
   printf("<catchfile> is the name of the input catchment raster file.\n");
   printf("<slpfile> is the name of the input D-inf slope file.\n");
   printf("<hfile> is the name of the input stage table.\n");
   printf("<hpfile> is the name of the output hydraulic property file.\n");
   return 0; 
} 
   
