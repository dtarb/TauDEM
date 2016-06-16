/*  SlopeAreamn main program for function that evaluates S^m A^n based on slope and specific catchment area 
  grid inputs, and parameters m and n.  This is intended for use with the slope-area stream 
  raster delineation method.
     
  David Tarboton
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

int main(int argc,char **argv)  
{
   char slopefile[MAXLN],scafile[MAXLN], safile[MAXLN];
   float p[2];
   int err;
      
   if(argc < 2) goto errexit;
   // Set defaults
   p[0]=2;  //  m exponent
   p[1]=1;  // n exponent
   if(argc == 2)
	{
//		printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(slopefile,argv[1],"slp");
		nameadd(scafile,argv[1],"sca");
		nameadd(safile,argv[1],"sa");
    }
   if(argc > 2)
   {
//		printf("You are running %s with the specific file names option.\n", argv[0]);
        int i=1;	
		while(argc > i)
		{
			if(strcmp(argv[i],"-slp")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(slopefile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-sca")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(scafile,argv[i]);
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
			else if(strcmp(argv[i],"-par")==0)
			{
				i++;
				if(argc > i+1)
				{
					sscanf(argv[i],"%f",&p[0]);
					i++;
					sscanf(argv[i],"%f",&p[1]);
					i++;
				}
				else goto errexit;
			}
		    else goto errexit;
		}
   }
    if((err=slopearea(slopefile,scafile, safile,p)) != 0)
        printf("SlopeArea Error %d\n",err);

	return 0;
errexit:
   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -slp <slopefile>\n",argv[0]);
   printf("-sca <scafile> -sa <safile> [-par <m> <n>] [-ddm <ddm>]\n");
   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'slp', 'sca' and 'sa' will be appended. \n");
   printf("<slopefile> is the name of the input slope file.\n");
   printf("<scafile> is the name of input contributing area file.\n");
   printf("<safile> is the name of the output file with the result slope^m x (contributing area)^n.\n");
   printf("<m> is the exponent on slope, default value 2 if not specified.\n");
   printf("<n> is the exponent on contributing area, default value 1 if not specified.\n");
   printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
   return 0; 
} 
   