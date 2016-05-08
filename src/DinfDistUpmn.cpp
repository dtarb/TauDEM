/*  DinfDistUpmn main program to compute distance to ridge in DEM 
    based on D-infinity flow direction model.
     
  David Tarboton, Teklu K Tesfa
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
#include "DinfDistUp.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;

//========================

int main(int argc,char **argv)
{
char angfile[MAXLN],felfile[MAXLN],slpfile[MAXLN],wfile[MAXLN],rtrfile[MAXLN];
   int err,i,statmethod=0,typemethod=0,usew=0, concheck=1;
   float thresh=0.0;
      
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
		if(strcmp(argv[i],"-ang")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(angfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-fel")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(felfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-slp")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy( slpfile,argv[i]);
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
		else if(strcmp(argv[i],"-wg")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(wfile,argv[i]);
				usew=1;
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-du")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(rtrfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-m")==0)
		{
			i++;
			if(argc > i)
			{
				if(strcmp(argv[i],"h")==0)
				{
					typemethod=0;
				}
				else if(strcmp(argv[i],"v")==0)
				{
					typemethod=1;
				}
				else if(strcmp(argv[i],"p")==0)
				{
					typemethod=2;
				}
				else if(strcmp(argv[i],"s")==0)
				{
					typemethod=3;
				}
				else if(strcmp(argv[i],"ave")==0)
				{
					statmethod=0;
				}
				else if(strcmp(argv[i],"max")==0)
				{
					statmethod=1;
				}
				else if(strcmp(argv[i],"min")==0)
				{
					statmethod=2;
				}
				i++;
				if(strcmp(argv[i],"h")==0)
				{
					typemethod=0;
				}
				else if(strcmp(argv[i],"v")==0)
				{
					typemethod=1;
				}
				else if(strcmp(argv[i],"p")==0)
				{
					typemethod=2;
				}
				else if(strcmp(argv[i],"s")==0)
				{
					typemethod=3;
				}
				else if(strcmp(argv[i],"ave")==0)
				{
					statmethod=0;
				}
				else if(strcmp(argv[i],"max")==0)
				{
					statmethod=1;
				}
				else if(strcmp(argv[i],"min")==0)
				{
					statmethod=2;
				}
				i++;				
			}
			else goto errexit;
		}		
	   else if(strcmp(argv[i],"-nc")==0)
		{
			i++;
			concheck=0;
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
		else 
		{
			goto errexit;
		}
	}
	if( argc == 2) {
		nameadd(angfile,argv[1],"ang");
		nameadd(felfile,argv[1],"fel");
		nameadd(slpfile,argv[1],"slp");
		nameadd(wfile,argv[1],"wg");
		nameadd(rtrfile,argv[1],"du");
	} 
 
if((err=dinfdistup(angfile,felfile,slpfile,wfile,rtrfile,statmethod,
   typemethod,usew, concheck,thresh)) != 0)
        printf("area error %d\n",err);   

//////Calling function
////int dinfdistup(char *angfile,char *felfile,char *slpfile,char *wfile, char *rtrfile,
////			   int statmethod,int typemethod,int useweight, int concheck, float thresh)
//	int er;
//switch (typemethod)
//{
//case 0://horizontal distance to ridge
//	er=hdisttoridgegrd(angfile,wfile,rtrfile,statmethod, 
//		concheck,thresh,usew);
//break;
//case 1://vertical rize to ridge
//	er=vrisetoridgegrd(angfile,felfile,rtrfile, 
//		statmethod,concheck,thresh);
//break;
//case 2:// Pythagoras distance to ridge
//	er=pdisttoridgegrd(angfile,felfile,wfile,rtrfile, 
//					statmethod,usew,concheck,thresh);
//break;
//case 3://surface distance to ridge
//	er=sdisttoridgegrd(angfile,slpfile,wfile,rtrfile, 
//					statmethod,usew,concheck,thresh);
//break;
//}
////return (er);

	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -ang <angfile>\n",argv[0]);
       printf("-fel <felfile> -slp <slpfile> [-wg <wfile>] -du <rtrfile>\n");
  	   printf("[-m ave h] [-nc] [-ddm <ddm>]\n");
	   printf("<basefilename> is the name of the raw digital elevation model\n");
	   printf("<angfile> is the D-infinity flow direction input file.\n");
	   printf("<felfile> is the pit filled or carved elevation input file.\n");
	   printf("<slpfile> is the D-infinity slope input file.\n");
	   printf("<wgfile> is the D-infinity flow direction input file.\n");
	   printf("<rtrfile> is the D-infinity distance output file.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
	   printf("[-m ave h] is the optional method flag.\n");
	   printf("The flag -nc overrides edge contamination checking\n");
	   printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("ang   D-infinity contributing area file (output)\n");
	   printf("fel   pit filled or carved elevation file\n");
	   printf("slp   D-infinity slope input file file\n");
	   printf("wg   weight input file\n");
	   printf("du   distance to stream output file\n");
       exit(0);
} 
