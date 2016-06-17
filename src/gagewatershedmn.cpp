/*  GaggeWatershedmn main program to compute gage watersheds
     based on D8 model  
  
  David Tarboton, John Koudelka
  Utah State University  
  January 7, 2011 
  
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
   char pfile[MAXLN],wfile[MAXLN],datasrc[MAXLN],lyrname[MAXLN],idfile[MAXLN];
   int err,useOutlets=0,useMask=0,uselyrname=0,lyrno=0,thresh=0,i=1,writeid=0;
   if(argc <= 2)
    {  	
	   goto errexit;
    }

	while(argc > i)
	{
		if(strcmp(argv[i],"-p")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(pfile,argv[i]);
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

		else if(strcmp(argv[i],"-gw")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(wfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-id")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(idfile,argv[i]);
				writeid=1;  
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}

    if( (err=gagewatershed(pfile,wfile,datasrc,lyrname,uselyrname,lyrno,idfile,writeid)) != 0)
        printf("Gage watershed error %d\n",err);

	return 0;

errexit:
	   printf("Usage:\n %s -p <pfile> -o <outletshape> -gw <gagewatershed> [-id <idfile>] [-ddm <ddm>]\n",argv[0]);
	   printf("<pfile> is the name of the input D8 flow direction grid file.\n");
	   printf("<outletshape> is the name of the input outlet shapefile.\n");
	   printf("<gagewatershed> is the output gagewatershed grid file.\n");
	   printf("<idfile> is optional output text file giving watershed downslope connectivity.\n");
           printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
       exit(0);
} 

