/*  D8HDistToStrmmn

  The mani program to compute horizontal distance to stream based on d8 flow model.
  
  David G Tarboton, Teklu K Tesfa
  Utah State University     
  May 23, 2010   
  
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

DecompType tdpartition::decompType = DECOMP_BLOCK;

int distgrid(char *pfile, char *srcfile, char *distfile, int thresh);

int main(int argc,char **argv)
{
   char pfile[MAXLN],srcfile[MAXLN],distfile[MAXLN];
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
		nameadd(srcfile,argv[1],"src");
		nameadd(distfile,argv[1],"dist");
	}

    if(err=distgrid(pfile,srcfile,distfile,thresh) != 0)
        printf("D8 distance error %d\n",err);


	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
       printf("Usage with specific file names:\n %s -p <pfile>\n",argv[0]);
	   printf("-src <srcfile> -dist <distfile> [-thresh <thresh>] [-ddm <ddm>]\n");
  	   printf("<basefilename> is the name of the base digital elevation model\n");
	   printf("<pfile> is the d8 flow direction input file.\n");
       printf("<srcfile> is the stream raster input file.\n");
       printf("<distfile> is the distance to stream output file.\n");
       printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
	   printf("The optional <thresh> is the user input threshold number.\n");
       printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("p      D8 flow directions (input)\n");
       printf("src    stream raster file (Input)\n");
       printf("dist   distance to stream file(output)\n");
       exit(0);
} 
