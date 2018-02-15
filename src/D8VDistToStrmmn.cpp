/*  D8VDistToStrmmn

  The main program to compute vertical distance to stream based on d8 flow model.
  
  Xing Zheng, David G Tarboton, Teklu K Tesfa
  University of Texas at Austin, Utah State University     
  July 02, 2017   
  
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

int d8vdistdown(char *pfile, char *felfile, char *srcfile, char *distfile, int thresh);

int main(int argc,char **argv)
{
   char pfile[MAXLN],felfile[MAXLN],srcfile[MAXLN],distfile[MAXLN];
   int err,nmain, thresh=1,i;
   
   if(argc < 2)
    {  
       printf("Error: To run this program, use either the Simple Usage option or\n");
	   printf("the Usage with Specific file names option\n");
	   goto errexit;
    }
   
   else if(argc > 2)
	{
		i = 1;
	//	printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	}
	else {
		i = 2;
	//	printf("You are running %s with the Simple Usage option.\n", argv[0]);
	}

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
		else if (strcmp(argv[i], "-fel") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(felfile, argv[i]);
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
		else if(strcmp(argv[i],"-dist")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(distfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-thresh")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%d",&thresh);
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}   

	if(argc == 2)
	{
		nameadd(pfile,argv[1],"p");
		nameadd(felfile, argv[1], "fel");
		nameadd(srcfile,argv[1],"src");
		nameadd(distfile,argv[1],"dist");
	}

    if(err=d8vdistdown(pfile,felfile,srcfile,distfile,thresh) != 0)
        printf("D8 distance down error %d\n",err);


	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
       printf("Usage with specific file names:\n %s -p <pfile>\n",argv[0]);
	   printf("-fel <felfile> -src <srcfile> -dist <distfile> [-thresh <thresh>]\n");
  	   printf("<basefilename> is the name of the base digital elevation model\n");
	   printf("<pfile> is the d8 flow direction input file.\n");
	   printf("<felfile> is the pit filled or carved elevation input file.\n");
       printf("<srcfile> is the stream raster input file.\n");
       printf("<distfile> is the vertical distance to stream output file.\n");
	   printf("The optional <thresh> is the user input threshold number.\n");
       printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("p      D8 flow directions (input)\n");
	   printf("fel    pit filled or carved elevation file\n");
       printf("src    stream raster file (Input)\n");
       printf("dist   distance to stream file(output)\n");
       exit(0);
} 
