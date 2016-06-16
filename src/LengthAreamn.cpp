/*  LengthAreamnmn main program for function that evaluates A >= M L^y ? 1:0 based on upslope path length 
  and D8 contributing area grid inputs, and parameters M and y.  This is intended for use 
  with the length-area stream raster delineation method.  

  David Tarboton, Teklu Tesfa
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
   char plenfile[MAXLN],ad8file[MAXLN], ssfile[MAXLN];
   float p[2];
   int err;
      
   if(argc < 2) goto errexit;
   // Set defaults
   p[0]=0.03;  //  M coefficient
   p[1]=1.3;  // y exponent
   if(argc == 2)
	{
//		printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(plenfile,argv[1],"plen");
		nameadd(ad8file,argv[1],"ad8");
		nameadd(ssfile,argv[1],"ss");
    }
   if(argc > 2)
   {
//		printf("You are running %s with the specific file names option.\n", argv[0]);
        int i=1;	
		while(argc > i)
		{
			if(strcmp(argv[i],"-plen")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(plenfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-ad8")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(ad8file,argv[i]);
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
		    else goto errexit;
		}
   }
    if((err=lengtharea(plenfile,ad8file, ssfile,p)) != 0)
        printf("Length Area Error %d\n",err);

	return 0;
errexit:
   printf("Simple Use:\n %s <basefilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -plen <plenfile>\n",argv[0]);
   printf("-ad8 <ad8file> -ss <ssfile> [-par <M> <y>] [-ddm <ddm>]\n");
   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'plen', 'ad8' and 'ss' will be appended. \n");
   printf("<plenfile> is the name of the input upslope longest path length file.\n");
   printf("<ad8file> is the name of input contributing area file.\n");
   printf("<ssfile> is the name of the output file with the result A >= M L^y ? 1:0.\n");
   printf("<M> is the coefficient, default value 0.03 if not specified.\n");
   printf("<y> is the exponent on upslope length, default value 1.3 if not specified.\n");
   printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
   return 0; 
} 
   