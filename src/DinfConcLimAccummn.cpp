/*  DinfConcLimAccummn main program to compute concentration limitted accumulation based on 
    D-infinity flow model.

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

DecompType tdpartition::decompType = DECOMP_BLOCK;

int main(int argc,char **argv)
{
   char angfile[MAXLN],ctptfile[MAXLN],dmfile[MAXLN],qfile[MAXLN],dgfile[MAXLN],datasrc[MAXLN],lyrname[MAXLN];
   int err,useOutlets=0,uselyrname=0,lyrno=0,contcheck=1,i;
   float cSol=1.;
   
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
				strcpy(angfile,argv[i]);//input Dinf flow direction grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-dg")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(dgfile,argv[i]);//input indicator grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-dm")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(dmfile,argv[i]);//Input decay multiplier grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-ctpt")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(ctptfile,argv[i]);//Output concentration grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-q")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(qfile,argv[i]); //input specific discharge grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-csol")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%f",&cSol);//concentration threshold grid
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
		else if(strcmp(argv[i],"-o")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(datasrc,argv[i]);
				useOutlets = 1;	
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


		else if(strcmp(argv[i],"-nc")==0)
		{
			i++;
			contcheck=0;
		}
		else 
		{
			goto errexit;
		}
	}
	if( argc == 2) {
		nameadd(angfile,argv[1],"ang");
		nameadd(dgfile,argv[1],"dg");
		nameadd(dmfile,argv[1],"dm");
		nameadd(qfile,argv[1],"q");
		nameadd(ctptfile,argv[1],"ctpt");

	}  
	if((err=dsllArea(angfile,ctptfile,dmfile,datasrc,lyrname,uselyrname,lyrno,qfile,dgfile,useOutlets,contcheck,cSol)) != 0)
        printf("area error %d\n",err);

	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -ang <angfile>\n",argv[0]);
       printf("-dg <indicatorFile> -dm <dmfile> -ctpt <afile>\n");
	   printf("-q <qfile> [-o <outletshapefile>] [-csol <cSol>] [<-nc>] [-ddm <ddm>]\n");
	   printf("<basefilename> is the name of the raw digital elevation model\n");
	   printf("<angfile> is the D-infinity flow direction input file.\n");
	   printf("<indicatorFile> is the disturbance indicator input grid file.\n");
	   printf("<dmfile> is the decay multiplier input grid file.\n");
	   printf("<ctptfile> is the concentration output grid file.\n");
	   printf("<qfile> is the specific discharge input grid file.\n");
	   printf("<outletshapefile> is the optional outlet shape input file.\n");
	   printf("<cSol> is the optional concentration threshold.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
       printf("The flag -nc overrides edge contamination checking\n");
	   printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("ang    D-infinity flow direction input file\n");
	   printf("dg     Disturbance indicator input file\n");
	   printf("dm     Decay multiplier grid (input)\n");
	   printf("q      Specific discharge grid (input)\n");
	   printf("ctpt   Concentration grid (output)\n");
       exit(0);  
}

