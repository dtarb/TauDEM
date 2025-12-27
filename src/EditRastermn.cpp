/*  Edit Raster main program to edit grid cell values and make minor changes where necessary.
     
  David Tarboton
  Utah State University  
  November 24 2016 

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
#include "tardemlib.h"

int editraster(char *rasterfile, char *newfile, char *changefile);

int main(int argc,char **argv)  
{
   char rasterfile[MAXLN],changefile[MAXLN],newfile[MAXLN];
   int i = 1;
   if(argc <= 2) goto errexit;	
	while (argc > i)
	{
		if (strcmp(argv[i], "-in") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(rasterfile, argv[i]);
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
		else if (strcmp(argv[i], "-changes") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(changefile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else goto errexit;

	}
	int err;
    if( (err=editraster(rasterfile, newfile, changefile)) != 0)
        printf("Threshold Error %d\n",err);

	return 0;
   errexit:
     printf("Editraster use:\n %s -in <filetochange>\n",argv[0]);
     printf("-out <filetowrite> \n");
     printf("-changes <file with change values (x,y,newvalue)> \n");
     return 0; 
} 
   