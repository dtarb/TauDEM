/*  Retention Limited Flow main program to compute retention limited Flow based on 
    D-infinity flow model.

  David Tarboton and Group
  Utah State University  
  Febuary 24, 2013
  
*/

/*  Copyright (C) 2013  David Tarboton, Utah State University

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
#include "retlimro.h"

int main(int argc,char **argv)
{
   char angfile[MAXLN],wgfile[MAXLN],rcfile[MAXLN],qrlfile[MAXLN];
   int err,i;
   
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
		else if(strcmp(argv[i],"-wg")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(wgfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-rc")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(rcfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-qrl")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(qrlfile,argv[i]);
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
		nameadd(rcfile,argv[1],"rc");
		nameadd(qrlfile,argv[1],"qrl");
		nameadd(wgfile,argv[1],"wg");
	}  
	if(err=retlimro(angfile, wgfile, rcfile, qrlfile) != 0)
        printf("RetlimFlow error %d\n",err);

	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
	   printf("Usage with specific file names:\n %s -ang <angfile>",argv[0]);
       printf("-rc <rcfile> -wg <wgfile> -qrl <qrlfile>\n");
	   printf("<basefilename> is the name of the raw digital elevation model\n");
	   printf("<angfile> is the D-infinity flow direction input file.\n");
	   printf("<rcfile> retention capacity file.\n");
	   printf("<wgfile> is the input weight at each grid cell.\n");
	   printf("<qrlfile> is retention limited runoff that is output.\n");
	   printf("With simple use the following are appended to the file names\n");
       printf("before the files are opened:\n");
       printf("ang    D-infinity flow direction input file\n");
	   printf("rc    retention capacity (input)\n");
	   printf("wg     weight grid (input)\n");
	   printf("qrl   retention limited runoff (output)\n");
       exit(0); 
}
