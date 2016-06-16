/*  D8FLowPathExtremeUpmn 
  Main program for function that evaluates the extreme (either maximum or minimum) upslope value 
  from an input grid based on the D8 flow directions.  It is similar to the minimum upslope function 
  used Tarolli and Tarboton (2006).  Numerically the extreme upslope value is computed recursively 
  as the extreme of the cell value itself and the result from the function applied at grid cells 
  immediately upslope, using D8 flow directions (Tarolli and Tarboton used Dinfinity).  This is 
  intended initially for use in stream raster generation to identify a threshold of slope x area 
  product that results in an optimum (according to drop analysis) stream network.  If an outlets 
  shapefile is provided the function should only output results for the area upslope of the outlets.

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

int d8flowpathextremeup(char *pfile, char*safile, char *ssafile, int usemax, char* datasrc,char* lyrname,int uselyrname,int lyrno,int useoutlets, int contcheck);

int main(int argc,char **argv)  
{
   char pfile[MAXLN],safile[MAXLN], ssafile[MAXLN],datasrc[MAXLN],lyrname[MAXLN];
   int err, useoutlets,contcheck,usemax,lyrno=0,uselyrname=0;
      
   if(argc < 2) goto errexit;
   usemax=1;  // Set defaults
   useoutlets=0;
   contcheck=1;
   if(argc == 2)
	{
//		printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(pfile,argv[1],"p");
		nameadd(safile,argv[1],"sa");
		nameadd(ssafile,argv[1],"ssa");
    }
   if(argc > 2)
   {
//		printf("You are running %s with the specific file names option.\n", argv[0]);
        int i=1;	
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
			else if(strcmp(argv[i],"-sa")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(safile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-ssa")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(ssafile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-o")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(datasrc,argv[i]);
					i++;
					useoutlets=1;
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
				strcpy(lyrname,argv[i]);
		        uselyrname = 1;
				i++;											
			}
			else goto errexit;
		}


		   else if(strcmp(argv[i],"-min")==0)
			{
				i++;
				usemax=0;
			}
			else if(strcmp(argv[i],"-nc")==0)
			{
				i++;
				contcheck=0;
			}
		   else goto errexit;
		}
   }
    if((err=d8flowpathextremeup(pfile, safile, ssafile, usemax, datasrc,lyrname,uselyrname,lyrno, useoutlets, contcheck)) != 0)
        printf("Flow Path Extreme Up Error %d\n",err);

	return 0;
errexit:
   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -p <pfile>\n",argv[0]);
   printf("-sa <safile> -ssa <ssafile> [-min] [-nc] [-o <outletsfile>] [-ddm <ddm>]\n");
   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'p', 'sa' and 'ssa' will be appended. \n");
   printf("<pfile> is the name of D8 flow directions file.\n");
   printf("<safile> is the name of input file with values from which extreme upslope is to be found.\n");
   printf("<ssa> is the name of the output file with extreme upslope values.\n");
   printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
   printf("-min indicates to search for a minimum (default is max)\n");
   printf("-nc indicates to override edge contamination checking (checking is on by default)\n");
   return 0; 
} 
   