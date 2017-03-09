/*  InunMap function takes HAND raster, gets inun depth info
 *  from the COMID mask raster and inundation forecast netcdf4 input,
 *  then creates the inundation map raster.

  Yan Liu, David Tarboton, Xing Zheng
  NCSA/UIUC, Utah State University, University of Texas at Austin
  Jan 03, 2017 
  
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
#include "InunMap.h"

int main(int argc,char **argv)  
{
   char handfile[MAXLN], catchfile[MAXLN], maskfile[MAXLN], fcfile[MAXLN], hpfile[MAXLN], fcmapfile[MAXLN];
   int maskpits = 0;
   int err, nmax;
   int hasMask = 0;
      
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
			else if(strcmp(argv[i],"-mask")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(maskfile,argv[i]); hasMask=1;
					i++;
				}
				else goto errexit;
			}

			else if(strcmp(argv[i],"-forecast")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(fcfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-hydrotable")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(hpfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-maskpits")==0)
			{
				i++;
				if(argc > i)
				{
					maskpits = 1;
				}
				else goto errexit;
			}

			else if (strcmp(argv[i], "-mapfile") == 0)
			{
				i++;
				if (argc > i)
				{
					strcpy(fcmapfile, argv[i]);
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
	if (hasMask)
		err=inunmap(handfile, catchfile, maskfile, fcfile, maskpits, hpfile, fcmapfile);
	else
		err=inunmap(handfile, catchfile, NULL, fcfile, maskpits, hpfile, fcmapfile);

    if(err != 0)
        printf("Inundation Map Generation Error %d\n",err);

	return 0;
errexit:
//   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -hand <handfile>\n",argv[0]);
   printf("-catch <catchfile> -mask <maskfile> \n");
   printf("-forecast <forecastfile> -mapfile <outputmapfile> \n");
   printf("-maskpits -hydrotable <hydropropertyfile> \n");
   printf("-mapfile <outputmapfile> \n");
//   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'plen', 'ad8' and 'ss' will be appended. \n");
   printf("<handfile> is the name of the input hand raster file.\n");
   printf("<catchfile> is the name of the input catchment COMID mask raster file.\n");
   printf("<maskfile> is the name of the mask raster file, e.g. waterbody.\n");
   printf("<forecastfile> is the name of the inundation forecast netCDF4 file.\n");
   printf("<maskpits> is the option to mask pits in inundation mapping if pit area >0.1.\n");
   printf("<hydropropertyfile> is the name of the hydro property netcdf file. Only when -maskpits is on\n");
   printf("<outputmapfile> is the name of the output inundation map file.\n");
   return 0; 
} 
   
