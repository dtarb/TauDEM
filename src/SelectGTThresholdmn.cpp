/*  Main program to select only data values from a grid that are greater than a threshold
  The rest are converted to no data.
     
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

int selgtthreshold(char *demfile,char *outfile, float thresh, int prow, int pcol);

int main(int argc,char **argv)  
{
   char demfile[MAXLN],outfile[MAXLN];
   float thresh=0.;
   int err,prow=0,pcol=0;
      
   if(argc < 2) goto errexit;
   if(argc == 2)
	{
		//printf("You are running %s with the simple use option.\n", argv[0]);
		nameadd(demfile,argv[1],"");
		nameadd(outfile,argv[1],"t");
    }
   if(argc > 2)
   {
		//printf("You are running %s with the specific file names option.\n", argv[0]);
        int i=1;	
		while(argc > i)
		{
			if(strcmp(argv[i],"-z")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(demfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i],"-t")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(outfile,argv[i]);
					i++;
				}
				else goto errexit;
			}
                else if(strcmp(argv[i],"-mf")==0)
                {
                        i++;
                        if(argc > i)
                        {
                                prow = atoi(argv[i]);
                                i++;
                                if(argc > i)
                                {
                                        pcol = atoi(argv[i]);
                                        i++;
                                }
                                else goto errexit;
                        }
                        else goto errexit;
                        if(prow <=0 || pcol <=0)
                                goto errexit;
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
    if( (err=selgtthreshold(demfile,outfile,thresh,prow,pcol)) != 0)
        printf("SelGTThreshold Error %d\n",err);

	return 0;
errexit:
   printf("Simple Use:\n %s <demfilename>\n",argv[0]);
   printf("Use with specific file names:\n %s -z <demfilename>\n",argv[0]);
   printf("-t <outfilename> [-thresh <thresholdvalue>]\n");
   printf("The threshold logic is if(dem <= thresh) dem=NODATA\n");
   return 0; 
} 
   
