/*  DinfTransLimAccum main program to compute D-infinity transport limited accumulation.
  
  David Tarboton,Teklu K Tesfa
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

//-ang demang.tif -tsup demtsup.tif -tc demtc.tif [-cs demcs.tif -ctpt demctpt.tiff] 
//-tla demtla.tif -tdep demtdep.tif [-o outletfile.shp] [-nc]

int main(int argc,char **argv)
{
   char angfile[MAXLN],tsupfile[MAXLN],tcfile[MAXLN],tlafile[MAXLN],depfile[MAXLN];
   char cinfile[MAXLN],coutfile[MAXLN],datasrc[MAXLN],lyrname[MAXLN];
   int err,useOutlets=0,usec=0,uselyrname=0,lyrno=0,compctpt=0,contcheck=1,i;
   
   if(argc < 2)
    {  
       printf("Error: To run this program, use either the Simple Usage option or\n");
	   printf("the Usage with Specific file names option\n");
	   goto errexit;   
    }

   else if(argc > 2)
	{
		i = 1;
		//printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	}
	else {
		i = 2;
		//printf("You are running %s with the Simple Usage option.\n", argv[0]);
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
		else if(strcmp(argv[i],"-tsup")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(tsupfile,argv[i]);//Input supply grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-tc")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(tcfile,argv[i]);//Transport capacity grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-cs")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(cinfile,argv[i]);//Input concentration grid (optional)
				usec=1;//use input concentraiotn file
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
		else if(strcmp(argv[i],"-ctpt")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(coutfile,argv[i]);//Output concentraiton grid (optional)
				compctpt=1;//compute output concentration file
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-tla")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(tlafile,argv[i]);//Output transport limitted accumulation grid
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-tdep")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(depfile,argv[i]);//Output deposition grid
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
		nameadd(tsupfile,argv[1],"tsup");
		nameadd(tcfile,argv[1],"tc");
		nameadd(tlafile,argv[1],"tla");
		nameadd(depfile,argv[1],"tdep");

	}  
	usec=usec*compctpt;  //  This ensures that both cinfile and coutfile have to be provided for concentration to be evaluated
	if((err=tlaccum(angfile,tsupfile,tcfile,tlafile,depfile,cinfile,coutfile,datasrc,lyrname,uselyrname,lyrno,
		useOutlets,usec,contcheck)) != 0)
        printf("tlaccum error %d\n",err);
	
	return 0;
	
errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -ang <pfile>\n",argv[0]);
       printf("-tsup <wfile> -tc <tcfile> [-cs <cfile> -ctpt <coutfile>]\n");
	   printf("-tla <tlafile> -tdep <depfile> [-o <shfile>] [<-nc>] [-ddm <ddm>]\n");
	   printf("<basefilename> is the name of the raw digital elevation model\n");
	   printf("<angfile> is the D-infinity flow direction input file.\n");
	   printf("<wfile> is the input transport supply grid file.\n");
	   printf("<tcfile> is the input transport capacity grid file.\n");
	   printf("<cfile> is the optional input concentration grid file.\n");
	   printf("<coutfile> is the optional output concentration grid file.\n");
	   printf("<tlafile> is the output transport limitted accumulation grid file.\n");
	   printf("<depfile> is the output deposition grid file.\n");
	   printf("<shfile> is the optional outlet shapefile.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
       printf("The flag -nc overrides edge contamination checking\n");
	   printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("ang    D-infinity flow direction input file\n");
	   printf("tsup   Input transport supply grid\n");
	   printf("tc     Input transport capacity grid\n");
	   printf("tla    Output transport limitted accumulation grid\n");
	   printf("tdep   output deposition grid\n");
       exit(0); 
}