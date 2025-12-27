/*  Set Region main program to set grid cell values for a grid processing region as part of RWD.
     
  David Tarboton
  Utah State University  
  January 15, 2017 

*/

/*  Copyright (C) 2017  David Tarboton, Utah State University

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

int setregion(char *fdrfile, char *regiongwfile, char *newfile, int32_t regionID);

int main(int argc,char **argv)  
{
   char fdrfile[MAXLN],regiongwfile[MAXLN],newfile[MAXLN];
   int32_t regionID = 1;
   int i = 1;
   if(argc <= 2) goto errexit;	
	while (argc > i)
	{
		if (strcmp(argv[i], "-p") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(fdrfile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-gw") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(regiongwfile, argv[i]);
				i++;
			}
			else goto errexit;
		}

		else if (strcmp(argv[i], "-out") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(newfile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-id") == 0)
		{
			i++;
			if (argc > i)
			{
				sscanf(argv[i], "%d", &regionID);
				i++;
			}
			else goto errexit;
		}
		else goto errexit;

	}
	int err;
    if( (err= setregion(fdrfile, regiongwfile, newfile, regionID) != 0))
        printf("Set Region Error %d\n",err);

	return 0;
errexit:
   printf("SetRetion use:\n %s -p <flow direction file>\n",argv[0]);
   printf("-gw <region gage watershed file> \n");
   printf("-out <output region file> \n");
   printf("[-id <Region identifier> (if ommitted defaults to 1)] \n");
   return 0; 
} 
   