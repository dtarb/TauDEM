/*  DropAnalysismn main program for function that applies a series of thresholds (determined from the input parameters) 
  to the input ssa grid and outputs in the drp.txt file the stream drop statistics table.  
  
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
#include "DropAnalysis.h"

DecompType tdpartition::decompType = DECOMP_BLOCK;

int dropan(char *areafile, char *dirfile, char *elevfile, char *ssafile, char *dropfile,
        char* datasrc, char* lyrname, int uselyrname, int lyrno, float threshmin, float threshmax, int nthresh, int steptype,
        float *threshopt);

int main(int argc, char **argv) {
    char areafile[MAXLN], dirfile[MAXLN], elevfile[MAXLN], ssafile[MAXLN], dropfile[MAXLN], datasrc[MAXLN], lyrname[MAXLN];
    float threshmin, threshmax, threshopt;
    int err, nthresh, uselyrname = 0, lyrno = 0, steptype;

    if (argc < 2) goto errexit;
    // Set defaults
    threshmin = 5;
    threshmax = 500;
    nthresh = 10;
    steptype = 0;
    if (argc == 2) {
        printf("No simple use option for this function because an outlets file is needed.\n", argv[0]);
        goto errexit;
    }
    if (argc > 2) {
        //printf("You are running %s with the specific file names option.\n", argv[0]);
        int i = 1;
        while (argc > i) {
            if (strcmp(argv[i], "-ad8") == 0) {
                i++;
                if (argc > i) {
                    strcpy(areafile, argv[i]);
                    i++;
                } else goto errexit;
            } else if (strcmp(argv[i], "-p") == 0) {
                i++;
                if (argc > i) {
                    strcpy(dirfile, argv[i]);
                    i++;
                } else goto errexit;
            } else if (strcmp(argv[i], "-fel") == 0) {
                i++;
                if (argc > i) {
                    strcpy(elevfile, argv[i]);
                    i++;
                } else goto errexit;
            } else if (strcmp(argv[i], "-ssa") == 0) {
                i++;
                if (argc > i) {
                    strcpy(ssafile, argv[i]);
                    i++;
                } else goto errexit;
            } else if (strcmp(argv[i], "-ddm") == 0) {
                i++;
                if (argc > i) {
                    if (strcmp(argv[i], "row") == 0) {
                        tdpartition::decompType = DECOMP_ROW;
                    } else if (strcmp(argv[i], "column") == 0) {
                        tdpartition::decompType = DECOMP_COLUMN;
                    } else if (strcmp(argv[i], "block") == 0) {
                        tdpartition::decompType = DECOMP_BLOCK;
                    } else {
                        goto errexit;
                    }
                    i++;
                } else goto errexit;
            } else if (strcmp(argv[i], "-o") == 0) {
                i++;
                if (argc > i) {
                    strcpy(datasrc, argv[i]);
                    //useOutlets = 1;	
                    i++;
                } else goto errexit;
            }

            else if (strcmp(argv[i], "-lyrno") == 0) {
                i++;
                if (argc > i) {
                    sscanf(argv[i], "%d", &lyrno);
                    i++;
                } else goto errexit;
            }

            else if (strcmp(argv[i], "-lyrname") == 0) {
                i++;
                if (argc > i) {
                    strcpy(lyrname, argv[i]);
                    uselyrname = 1;
                    i++;
                } else goto errexit;
            }


            else if (strcmp(argv[i], "-drp") == 0) {
                i++;
                if (argc > i) {
                    strcpy(dropfile, argv[i]);
                    i++;
                } else goto errexit;
            } else if (strcmp(argv[i], "-par") == 0) {
                i++;
                if (argc > i + 3) {
                    sscanf(argv[i], "%f", &threshmin);
                    i++;
                    sscanf(argv[i], "%f", &threshmax);
                    i++;
                    sscanf(argv[i], "%d", &nthresh);
                    i++;
                    sscanf(argv[i], "%d", &steptype);
                    i++;
                } else goto errexit;
            } else goto errexit;
        }
    }
    if ((err = dropan(areafile, dirfile, elevfile, ssafile, dropfile,
            datasrc, lyrname, uselyrname, lyrno, threshmin, threshmax, nthresh, steptype,
            &threshopt)) != 0)
        printf("Drop Analysis Error %d\n", err);
    //	else printf("%f  Value for optimum that drop analysis selected - see output file for details.\n",threshopt);

    return 0;
errexit:
    printf("\nUse with specific file names:\n %s -slp <slopefile>\n", argv[0]);
    printf("-ad8 <ad8file> -p <dirfile> -fel <elevfile> -ssa <ssafile> -o <outletsshapefile>\n");
    printf("-drp <dropfile> [-par <min> <max> <nthresh> <steptype>] [-ddm <ddm>]\n");
    printf("<ad8file> is the name of the input contributing area file used in calculations of drainage density. \n");
    printf("<dirfile> is the name of the input D8 flow directions file.\n");
    printf("<elevfile> is the name of the input elevation file.\n");
    printf("<ssafile> is the name of the accumulated stream source file.  This needs to have the property that it is\n");
    printf("monotonically increasing downstream along the D8 flow directions.\n");
    printf("<outletsshapefile> is the name of the shapefile containing input outlets.\n");
    printf("<dropfile> a text file for drop analysis tabular output.\n");
    printf("<min> Lower bound of range used to search for optimum threshold.\n");
    printf("<max> Upper bound of range used to search for optimum threshold.\n");
    printf("<nthresh> Number of thresholds used to search for optimum threshold.\n");
    printf("<steptype> Type of threshold step to be used (0=log, 1=arithmetic).\n");
    printf("<ddm> is the data decomposition method. Either \"row\", \"column\" or \"block\".\n");
    return 0;
}

