/*  Threshold main program to evaluate grid cells >= input threshold value.
     
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

int threshold(char *ssafile,char *srcfile,char *maskfile, float thresh, int usemask);

int main(int argc,char **argv)  
{
   char ssafile[MAXLN],srcfile[MAXLN], maskfile[MAXLN];
   int err, usemask;
   float thresh;
      
   if(argc < 2) goto errexit;
   usemask=0;  // Set defaults
   thresh=100.;
   if(argc == 2)
	{
		//printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(ssafile,argv[1],"ssa");
		nameadd(srcfile,argv[1],"src");
    }
   if(argc > 2)
   {
		//printf("You are running %s with the specific file names option.\n", argv[0]);
        int i=1;	
		while(argc > i)
		{
			if(strcmp(argv[i],"-ssa")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(ssafile,argv[i]);
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
			else if(strcmp(argv[i],"-mask")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(maskfile,argv[i]);
					i++;
					usemask=1;
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
					sscanf(argv[i],"%f",&thresh);
					i++;
				}
				else goto errexit;
			}
		   else goto errexit;
		}
   }
    if( (err=threshold(ssafile,srcfile,maskfile,thresh,usemask)) != 0)
        printf("Threshold Error %d\n",err);

	return 0;
errexit:
   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -fel <ssafile>\n",argv[0]);
   printf("-ss <srcfile> [-thresh <thresholdvalue>] [-mask <maskfile>] [-ddm <ddm>]\n");
   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'ssa' and 'src' will be appended. \n");
   printf("<ssafile> is the name of file to be thresholded.\n");
   printf("<srcfile> is the name of file with the thresholded output.\n");
   printf("<maskfile> is the name of a file that masks the domain.\n");
   printf("<thresholdvalue> is the value of the threshold.\n");
   printf("The threshold logic is src = ((ssa >= thresh) & (mask >=0)) ? 1:0.\n");
   printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
   return 0; 
} 
   