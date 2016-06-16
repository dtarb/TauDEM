/*  StreamNetmn main program to compute stream networks based on d8 directions.
     
  David Tarboton, Teklu K. Tesfa
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
#include "DropAnalysis.h"
#include "tardemlib.h"
#include "streamnet.h"

DecompType tdpartition::decompType = DECOMP_ROW;

int main(int argc,char **argv)
{
   char pfile[MAXLN],srcfile[MAXLN],ordfile[MAXLN],ad8file[MAXLN],elevfile[MAXLN],wfile[MAXLN],outletsds[MAXLN],lyrname[MAXLN],streamnetsrc[MAXLN];
   char treefile[MAXLN],coordfile[MAXLN];char streamnetlyr[MAXLN]="";
   long ordert=1, useoutlets=0,uselayername=0,lyrno=0; 
   int err,i;
    bool verbose=false;  //  Initialize verbose flag
   
   if(argc < 2)
    {  	
	   printf("Error: To run this program, use either the Simple Usage option or\n");
	   printf("the Usage with Specific file names option\n");
	   goto errexit;
    }

	if(argc > 2)
	{
		i = 1;
//		printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	}
	else {
		i = 2;
//		printf("You are running %s with the Simple Usage option.\n", argv[0]);
	}
	//  Some debug testing of arguments
	//int j;
	//for(j=0; j<argc; j++)
	//	printf("%d %d Arg: %s:end\n",j,strlen(argv[j]),argv[j]);
	while(argc > i)
	{
		if(strcmp(argv[i],"-fel")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(elevfile,argv[i]);
				// sscanf(argv[i],"%s",elevfile);  
				// DGT 5/14/10 Discovered that this did not copy file names with spaces - so switched to strcpy
				//printf("filename: %s:end\n",elevfile);  
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-p")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(pfile,argv[i]);
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
		else if(strcmp(argv[i],"-ord")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(ordfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-tree")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(treefile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-coord")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(coordfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-o")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletsds,argv[i]);
				i++;
				useoutlets = 1;
			}
			else goto errexit;
		}

			   else if(strcmp(argv[i],"-lyrno")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%ld",&lyrno);
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
				uselayername=1;
				i++;
				
			}
			else goto errexit;
		}

		else if(strcmp(argv[i],"-w")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(wfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-net")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(streamnetsrc,argv[i]);
				i++;
			}
			else goto errexit;
		}

		else if(strcmp(argv[i],"-netlyr")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(streamnetlyr,argv[i]);
				i++;
			}
			else goto errexit;
		}


		else if(strcmp(argv[i],"-sw")==0)
		{
			i++;
			ordert=0;
		}		
		else if(strcmp(argv[i],"-v")==0)
		{
			i++;
			verbose=true;
		}
		else 
		{
			goto errexit;
		}
	}
	if( argc == 2) {
		nameadd(elevfile,argv[1],"fel");
		nameadd(pfile,argv[1],"p");
		nameadd(ad8file,argv[1],"ad8");
		nameadd(srcfile,argv[1],"src");
		nameadd(ordfile,argv[1],"ord");
		nameadd(treefile,argv[1],"tree.txt");
		nameadd(coordfile,argv[1],"coord.txt");
		nameadd(wfile,argv[1],"w");
		//nameadd(streamnetshp,argv[1],"net.shp");
	} 

    if(err=netsetup(pfile,srcfile,ordfile,ad8file,elevfile,treefile,coordfile,outletsds,lyrname,uselayername,lyrno,wfile,streamnetsrc,streamnetlyr,useoutlets, ordert,verbose) != 0)
       printf("StreamNet error %d\n",err);

	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
       printf("Usage with specific file names:\n %s -p <pfile>\n",argv[0]);
	   printf("-fel <elevfile> -ad8 <ad8file> -src <srcfile>\n");
	   printf("-ord <ordfile> -tree <treefile> -coord <coordfile>\n");
	   printf("-fel <elevfile> -ad8 <ad8file> -dn <dn>\n");
  	   printf("<basefilename> is the name of the base digital elevation model\n");
       printf("<pfile> is the D8 flow direction input file.\n");
	   printf("<felfile> is the pit filled or carved elevation input file.\n");
	   printf("<slpdfile> is the output D8 slope distance averaged grid file.\n");
	   printf("<dn is the optional user selected downslope distance.\n");
       printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("fel   pit filled or carved elevation grid (input)\n");
       printf("p   D-infinity flow direction grid (Input)\n");
       printf("slpd   avalanche source site grod (input)\n");
       exit(0);
} 

