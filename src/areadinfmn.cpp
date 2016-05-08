/*  TauDEM AreaDinf main program to compute contributing area 
    based on D-infinity flow model.
     
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
#include "areadinf.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;
  
int main(int argc,char **argv)
{
   char pfile[MAXLN],afile[MAXLN],wfile[MAXLN],datasrc[MAXLN],lyrname[MAXLN];
   int err,useOutlets=0,uselyrname=0,usew=0,lyrno=0,contcheck=1,i;
      
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
				strcpy(pfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-sca")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(afile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-o")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(datasrc, argv[i]);
				useOutlets=1;
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
		nameadd(afile,argv[1],"sca");
		nameadd(pfile,argv[1],"ang");
	}   
   
   if((err=area(pfile,afile,datasrc,lyrname,uselyrname,lyrno,wfile,useOutlets,usew,contcheck)) != 0)
        printf("area error %d\n",err);
   

	return 0;
	
errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -ang <angfile>\n",argv[0]);
       printf("-sca <afile> [-o <shfile>] [-wg <wfile>] [-ddm <ddm>]\n");
	   printf("<basefilename> is the name of the raw digital elevation model\n");
	   printf("<angfile> is the D-infinity flow direction input file.\n");
	   printf("<afile> is the D-infinity area output file.\n");
	   printf("[-o <shfile>] is the optional outlet shape input file.\n");
       printf("[-wg <wfile>] is the optional weight grid input file.\n");
       printf("The flag -nc overrides edge contamination checking\n");
       printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
	   printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("sca   D-infinity contributing area file (output)\n");
	   printf("ang   D-infinity flow direction output file\n");
       exit(0); 
} 
