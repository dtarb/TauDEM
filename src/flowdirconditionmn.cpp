/*  TauDEM D8FlowDir main program to compute flow direction based on d8 flow model.
     
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
#include "d8.h"

 int flowdircond( char *pfile, char *zfile, char *zfdcfile);
int main(int argc,char **argv)
{
  char pfile[MAXLN], zfile[MAXLN], zfdcfile[MAXLN];
  int err, i;
    short useflowfile=0;

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
				strcpy(zfile,argv[i]);
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
		
		else if(strcmp(argv[i],"-zfdc")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(zfdcfile,argv[i]);
				i++;
				useflowfile=1;
			}
			else goto errexit;
		}

		else 
		{
			goto errexit;
		}
	}
	if( argc == 2) {
		nameadd(zfile,argv[1],"z");
		nameadd(pfile,argv[1],"p");
		nameadd(zfdcfile,argv[1],"zfdc");	
	}

    if((err=flowdircond( pfile, zfile, zfdcfile)) != 0)
        printf("flowdiircond error %d\n",err);

	return 0;

errexit:
	   printf("Usage TODO\n",argv[0]);
	  /* printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -fel <demfile>\n",argv[0]);
       printf("-sd8 <slopefile> -p <angfile> [-sfdr <flowfile>]\n");
	   printf("<basefilename> is the name of the raw digital elevation model\n");
	   printf("<demfile> is the pit filled or carved DEM input file.\n");
	   printf("<slopefile> is the slope output file.\n");
	   printf("<pointfile> is the output d8 flow direction file.\n");
       printf("[-sfdr <flowfile>] is the optional user imposed stream flow direction file.\n");
       printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("fel    carved or pit filled input elevation file\n");
       printf("sd8    D8 slope file (output)\n");
	   printf("p   D8 flow direction output file\n");*/
       exit(0);
}    

