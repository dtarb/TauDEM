/*  PeukerDouglas main program for function that operates on an elevation grid and outputs an 
  indicator (1,0) grid of upward curved grid cells according to the Peuker and 
  Douglas algorithm.
     
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
//#include "gridCodes.h"
#include "commonLib.h"
//#include "tardemlib.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;

int peukerdouglas(char *felfile,char *ssfile,float *p);

int main(int argc,char **argv)  
{
   char felfile[MAXLN],ssfile[MAXLN];
   int err;
   float p[3];
      
   if(argc < 2) goto errexit;
   //  Specify default weights
   p[0]=0.4;  p[1]=0.1;  p[2]=0.05;
   if(argc == 2)
	{
//		printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(felfile,argv[1],"fel");
		nameadd(ssfile,argv[1],"ss");
    }
   if(argc > 2)
   {
//		printf("You are running %s with the specific file names option.\n", argv[0]);
        int i=1;	
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
			else if(strcmp(argv[i],"-ss")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(ssfile,argv[i]);
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
				if(argc > i+2)
				{
					sscanf(argv[i],"%f",&p[0]);
					i++;
					sscanf(argv[i],"%f",&p[1]);
					i++;
					sscanf(argv[i],"%f",&p[2]);
					i++;
				}
				else goto errexit;
			}
		   else goto errexit;
		}
   }
    if( (err=peukerdouglas(felfile,ssfile,p)) != 0)
        printf("Peuker Douglas Error %d\n",err);

	return 0;
errexit:
   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -fel <elevationfile>\n",argv[0]);
   printf("-ss <streamsource> [-par <weightMiddle> <weightSide> <weightDiagonal>] [-ddm <ddm>]\n");
   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. 'fel' will be appended. \n");
   printf("<elevationfile> is the name of the elevation input file.\n");
   printf("<streamsource> is the name of the stream source file output.\n");
   printf("The elevation input is smoothed by averaging using the center and eight surrounding grid cells.\n");
   printf("<weightMiddle> is the weight given to the center cell in the smoothing of the input elevations.\n");
   printf("<weightSide> is the weight given to the 4 side cells in the smoothing of the input elevations.\n");
   printf("<weightDiagonal> is the weight given to the 4 diagonal cells in the smoothing of the input elevations.\n");
   printf("Default weights are 0.4 0.1 0.05 if -par is not specified.\n");
   printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
   return 0; 
} 
   

