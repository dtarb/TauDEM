/*  DinfAvalanchemn main program to compute avalanche runout zones.
  
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
#include "tardemlib.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;

//-ang demang.tif -fel demfel.tif -ass demass.tif -rz demrz.tif -dfs demdfs.tif [-thresh 0.2] [-alpha 20] [-direct]

int main(int argc,char **argv)
{
   char angfile[MAXLN],felfile[MAXLN],assfile[MAXLN],rzfile[MAXLN],dmfile[MAXLN];
   int err,i;
   int path=1;
   float thresh=0.2, alpha=18.0;

   if(argc < 2)
    {  
       printf("Error: To run this program, use either the Simple Usage option or\n");
	   printf("the Usage with Specific file names option\n");
	   goto errexit;
    }

   else if(argc > 2)
	{
		i = 1;
//		printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	}
	else {
		i = 2;
//		printf("You are running %s with the Simple Usage option.\n", argv[0]);
	}
	while(argc > i)
	{
		if(strcmp(argv[i],"-fel")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(felfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-ang")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(angfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-ass")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(assfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-rz")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(rzfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-dfs")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(dmfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-thresh")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%f",&thresh);
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
		else if(strcmp(argv[i],"-alpha")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%f",&alpha);
				i++;
			}
			else goto errexit;
		}
	   else if(strcmp(argv[i],"-direct")==0)
		{
			i++;
			path=0;
		}
		else 
		{
			goto errexit;
		}
	}

	if( argc == 2) {
		nameadd(felfile,argv[1],"fel");
		nameadd(angfile,argv[1],"ang");
		nameadd(assfile,argv[1],"ass");
		nameadd(rzfile,argv[1],"rz");
		nameadd(dmfile,argv[1],"dfs");
	}   

   if(err=avalancherunoutgrd(angfile,felfile,assfile,rzfile,dmfile,thresh,alpha,path) != 0)
         printf("area error %d\n",err);


	return 0;
	
errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
       printf("Usage with specific file names:\n %s -fel <felfile>\n",argv[0]);
	   printf("-ang <angfile> -ass <assfile> -ass <assfile> -rz <rzfile>\n");
	   printf("[-thresh <thresh>] [-alpha <alpha>] [-ddm <ddm>] [<path>]\n");
  	   printf("<basefilename> is the name of the base digital elevation model\n");
	   printf("<felfile> is the pit filled or carved elevation input file.\n");
       printf("<angfile> is the d-infinity flow direction input file.\n");
       printf("<assfile> is the avalanche source site input grid file.\n");
	   printf("<rzfile> is the avalanche runout zone output grid file.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
	   printf("The optional <thresh> is the input proportion threshold number.\n");
	   printf("The optional <alpha> is the user input angle threshold number.\n");
	   printf("The optional <path> is the flag to indicate whether distance is measured along\n");
	   printf("flow path (path=1) or as a straight line from source to grid cell (path=0).\n");
       printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("fel   pit filled or carved elevation grid (input)\n");
       printf("ang   D-infinity flow direction grid (Input)\n");
       printf("ass   avalanche source site grod (input)\n");
	   printf("rz    avalanche runout zone grid (output)\n");
	   printf("dm    avalanche runout zone grid (output)\n");
       exit(0); 
} 
