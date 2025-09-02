/*  InunDepth function takes HAND raster, computes inundation depths
 *  using flow forecast CSV input and hydro property flow input,
 *  then creates the inundation map/depth raster file. Optionally,
 *  it can also output the inundation depth text/CSV file.

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
#include "InunDepth.h"

int main(int argc,char **argv)
{
   char handfile[MAXLN], catchfile[MAXLN], maskfile[MAXLN], fcfile[MAXLN], hpfile[MAXLN], fcmapfile[MAXLN], depthfile[MAXLN];
   int err;
   int hasMask = 0;
   int hasDepth = 0;
   int minRequiredArgs = 5; // maksfile and depthfile are optional
      
   if(argc < minRequiredArgs) goto errexit;
/*   if(argc == 2)
	{
//		printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(plenfile,argv[1],"plen");
		nameadd(ad8file,argv[1],"ad8");
		nameadd(ssfile,argv[1],"ss");
    }*/
   if(argc >= minRequiredArgs)
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
					strcpy(maskfile,argv[i]);
					hasMask=1;
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-fc")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(fcfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-hp")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(hpfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if (strcmp(argv[i], "-inun") == 0)
			{
				i++;
				if (argc > i)
				{
					strcpy(fcmapfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if (strcmp(argv[i], "-depth") == 0)
			{
				i++;
				if (argc > i)
				{
					strcpy(depthfile, argv[i]);
					hasDepth = 1;
					i++;
				}
				else goto errexit;
			}
		    else goto errexit;
		}
   }
   	if(handfile == NULL || catchfile == NULL || fcfile == NULL || hpfile == NULL || fcmapfile == NULL) goto errexit;
	if (hasMask && hasDepth)
		err=inundepth(handfile, catchfile, maskfile, fcfile, hpfile, fcmapfile, depthfile);
	else if (hasMask)
		err=inundepth(handfile, catchfile, maskfile, fcfile, hpfile, fcmapfile, NULL);
	else if (hasDepth)
		err=inundepth(handfile, catchfile, NULL, fcfile, hpfile, fcmapfile, depthfile);
	else
		err=inundepth(handfile, catchfile, NULL, fcfile, hpfile, fcmapfile, NULL);

	if(err != 0)
		printf("Inundation Depth Generation Error %d\n",err);

	return 0;
errexit:
   printf("Use with specific file names:\n %s -hand <handfile>\n",argv[0]);
   printf("-catch <catchfile> -mask <maskfile> \n");
   printf("-fc <forecastfile> -hp <hydropropertyfile> \n");
   printf("-inun <outputinundationfile> \n");
   printf("-depth <outputdepthfile> \n");
   printf("<handfile> is the name of the input hand raster file - required file.\n");
   printf("<catchfile> is the name of the input catchment COMID raster file - required file.\n");
   printf("<maskfile> is the name of the mask raster file - optional file, e.g. waterbody.\n");
   printf("<forecastfile> is the name of the inundation forecast CSV file - required file, with columns: id, flow.\n");
   printf("<hydropropertyfile> is the name of the hydro property text file - required file.\n");
   printf("<outputinundationfile> is the name of the output inundation raster file - required file.\n");
   printf("<outputdepthfile> is the name of the output inundation depth text/CSV file - optional file.\n");
   return 0; 
} 
