/*  PitRemovemn main program to compute pit filled DEM.
     
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
#include "flood.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;

int main(int argc,char **argv)
{
   char demfile[MAXLN],newfile[MAXLN],flowfile[MAXLN];
   int err,order,subbno,i;
   short useflowfile=0;
   bool verbose=false;  //  Initialize verbose flag
   bool is_4p = false; // four-point flow method versus eight-point, arb 5/31/11
   char maskfile[MAXLN]; // mask out actual depressions, arb 5/31/11
   bool use_mask = false; // flag to specify the optional mask file, arb 5/31/11
   
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
		else if(strcmp(argv[i],"-fel")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(newfile,argv[i]);
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
		else if(strcmp(argv[i],"-sfdr")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(flowfile,argv[i]);
				useflowfile=1;
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-v")==0)
		{
			i++;
			verbose=true;
		}
		else if(strcmp(argv[i],"-4way")==0)   // arb added, 5/31/11. This is the flag for 4-point pour method
		{
			i++;
			is_4p=true;
		}
		else if(strcmp(argv[i],"-depmask")==0) // arb added, 5/31/11. This is to input the optional depression mask file
		{
			i++;
			if(argc > i)
			{
				strcpy(maskfile,argv[i]);
				use_mask=true;
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}
	if( argc == 2) {
		strcpy(demfile,argv[1]);
		//printf("File %s\n",demfile);
		nameadd(newfile,argv[1],"fel");
	} 
	//  Debugging prints
	if(verbose)
	{
		printf("On input demfile: %s\n",demfile);
		printf("On input newfile: %s\n",newfile);
    printf("%ssing mask file: %s\n",use_mask?"U":"Not U", use_mask?maskfile:"N/A");
		fflush(stdout);
	}
	useflowfile=0;  //  useflowfile not implemented

	if((err=flood(demfile,newfile,flowfile,useflowfile,verbose,is_4p,use_mask,maskfile)) != 0)
        printf("PitRemove error %d\n",err);

	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -z <demfile>\n",argv[0]);
           printf("-fel <newfile> [-sfdr <flowfile>] [-ddm <ddm>]\n");
	   printf("<basefilename> is the name of the raw digital elevation model.\n");
	   printf("<demfile> is the name of the input elevation grid file.\n");
	   printf("<newfile> is the output elevation grid with pits filled.\n");
	   printf("<flowfile> is the input grid of flow directions to be imposed.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
	   printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("fel    output elevation grid with pits filled.\n\n");
       exit(0);
}

