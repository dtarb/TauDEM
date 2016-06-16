/*  SlopeAreamn main program for function that evaluates S^m A^n based on slope and specific catchment area 
  grid inputs, and parameters m and n.  This is intended for use with the slope-area stream 
  raster delineation method.
     
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

DecompType tdpartition::decompType = DECOMP_ROW;

int main(int argc,char **argv)  
{
   char slopefile[MAXLN], scaterrainfile[MAXLN], scarminroadfile[MAXLN], scarmaxroadfile[MAXLN], tergridfile[MAXLN], terparfile[MAXLN], satfile[MAXLN], sincombinedfile[MAXLN];
   *scarminroadfile = NULL;
   *scarmaxroadfile = NULL;
   float Rminter = 0.0, Rmaxter = 0.0;
   float temp_par[2];
   double par[2];
   temp_par[0] = 0.0;
   temp_par[1] = 0.0;
   
   par[0] = 0.0;
   par[1] = 0.0;
   
   float road_impact = false;
   float p[2];
   int err;
      
   if(argc < 11) goto errexit;

   // Set defaults
   p[0]=2;  //  m exponent
   p[1]=1;  // n exponent

   // argument values are positional
   if(argc == 11)
   {
	   //Function call format:
	   //SinmapSI demslp.tif demsca.tif calpar.csv demcal.tif demsi.tif demsat.tif 0.0009 0.00135 9.81 1000

	   strcpy(slopefile,argv[1]);
	   strcpy(scaterrainfile,argv[2]);	 
	   strcpy(terparfile,argv[3]);
	   strcpy(tergridfile,argv[4]);	   	  
	   strcpy(sincombinedfile,argv[5]);
	   strcpy(satfile,argv[6]);
	   sscanf(argv[7],"%f",&Rminter);	   
	   sscanf(argv[8],"%f",&Rmaxter);
	   sscanf(argv[9],"%f",&temp_par[0]);
	   sscanf(argv[10],"%f",&temp_par[1]);	  	   
	   par[0] = (double)temp_par[0];
	   par[1] = (double)temp_par[1];	   
   }   
   else if(argc == 13)
   {
	   //Function call format:
	   //SinmapSI demslp.tif demsca.tif calpar.csv demcal.tif demsi.tif demsat.tif 0.0009 0.00135 9.81 1000 demscamin.tif demscamax.tif

	   strcpy(slopefile,argv[1]);
	   strcpy(scaterrainfile,argv[2]);	 
	   strcpy(terparfile,argv[3]);
	   strcpy(tergridfile,argv[4]);	   	  
	   strcpy(sincombinedfile,argv[5]);
	   strcpy(satfile,argv[6]);
	   sscanf(argv[7],"%f",&Rminter);	   
	   sscanf(argv[8],"%f",&Rmaxter);
	   sscanf(argv[9],"%f",&temp_par[0]);
	   sscanf(argv[10],"%f",&temp_par[1]);	   	
	   par[0] = (double)temp_par[0];
	   par[1] = (double)temp_par[1];	   
	   strcpy(scarminroadfile,argv[11]);
	   strcpy(scarmaxroadfile,argv[12]);

   }   
   // argument values are not positional and identified by argument identifiers ( eg. '-slp' for slope file etc)
   else if(argc > 13)
   {
	   //Function call format
	   //-slp demslp.tif -sca demsca.tif -calpar demcalp.txt -cal demcal.tif -si demsi.tif -sat demsat.tif -par 0.0009 0.00135 9.81 1000 -scamin scamin.tif -scamax scamax.tif
        
	   int i = 1;	
		while(argc > i)
		{			
			if(strcmp(argv[i], "-slp")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(slopefile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-sca")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(scaterrainfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-scamin")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(scarminroadfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-scamax")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(scarmaxroadfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-cal")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(tergridfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-calpar")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(terparfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-si")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(sincombinedfile, argv[i]);
					i++;
				}
				else goto errexit;
			}
			else if(strcmp(argv[i], "-sat")==0)
			{
				i++;
				if(argc > i)
				{
					strcpy(satfile, argv[i]);
					i++;
				}
				else goto errexit;
			}			
			else if(strcmp(argv[i], "-par")==0)
			{
				i++;
				if(argc > i+3)
				{
					sscanf(argv[i], "%f", &Rminter);
					i++;
					sscanf(argv[i], "%f", &Rmaxter);
					i++;
					sscanf(argv[i], "%f", &temp_par[0]);
					par[0] = temp_par[0];
					i++;
					sscanf(argv[i], "%f", &temp_par[1]);
					par[1] = temp_par[1];
					i++;					
				}
				else goto errexit;
			}
		    else goto errexit;
		}
   }
    if((err = sindexcombined(slopefile, scaterrainfile, scarminroadfile, scarmaxroadfile, tergridfile, terparfile, satfile, sincombinedfile, Rminter, Rmaxter, par)) != 0)
        printf("Sinmap Stability Index Error %d\n", err);
	//getchar();
	return 0;
errexit:
   // TODO: This needs to be fixed to match arument list and format used in the main function
   printf("Simple Use:\n %s <basefilename>\n", argv[0]);
   printf("Use with specific file names:\n %s -slp <slopefile>\n", argv[0]);
   printf("-sca <scafile> -calpar <calpfile> -cal <calfile> -si <sifile> -sat <satfile> -par <rminter> <rmaxter> <g> <rhow> [-scamin <scaminfile> -scamax <scamaxfile>] \n");
   printf("<basefilename> is the name of the base digital elevation model without suffixes for simple input. Suffixes 'slp', 'sca' and 'sa' will be appended. \n");
   printf("<slopefile> is the name of the input slope file.\n");
   printf("<scafile> is the name of input contributing area file.\n");
   printf("<calpfile> is the name of input calibration region parameter text file.\n");
   printf("<calfile> is the name of input calibration grid file file.\n");
   printf("<sifile> is the name of output combined stability index grid file.\n");
   printf("<satfile> is the name of output <TODO> file.\n");  
   printf("<rminter> is the minimum terrain recharge constant.\n");
   printf("<rmaxter> is the maximum terrain recharge constant.\n");
   printf("<g> is the gravitational force.\n");
   printf("<rhow> is the <TODO> constant.\n");
   printf("<scaminfile> is the name of input minimum contributing area file.\n");
   printf("<scamaxfile> is the name of input maximum contributing area file.\n");
   
   //getchar();
   return 0; 
} 
   