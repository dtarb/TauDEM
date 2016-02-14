/*  Program to compute stability index and factor of safety for landslide 
    Hazard mapping. 
 */
/*
  David G Tarboton
  Utah State University   
  
  SINMAP package version 1.0  3/30/98

Acknowledgement
---------------
This program developed with the support of Forest Renewal of British Columbia, 
in collaboration with Canadian Forest Products Ltd., Vancouver, British Columbia 
and Terratech Consulting Ltd., British Columbia, http://www.tclbc.com (Bob Pack)
and Craig Goodwin, 1610 E 1185 North, Logan, UT 84341-3036.

These programs are distributed freely with the following conditions.
1.  In any publication arising from the use for research purposes the source of 
the program should be properly acknowledged and a pre-print of the publication 
sent to David Tarboton at the address below.
2.  In any use for commercial purposes or inclusion in any commercial software 
appropriate acknowledgement is to be included and a free copy of this software 
made available to David Tarboton, Terratech and CanFor.

David Tarboton
Utah Water Research Laboratory
Utah State University
Logan, UT, 84322-8200
U. S. A.

tel:  (+1) 435 797 3172
email: dtarb@cc.usu.edu   */
//#include "stdafx.h"	
#define NOMINMAX
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include<algorithm>
#include<mpi.h>
#include "createpart.h"
#include "tiffIO.h"
//#include <iostream>
//#include "lsm.h"

#include "SINMAPErrorCodes.h"
//#include "shapefile.h"
#include "sindex.h"
//#include "sedimentlib.h"
/*  Prototypes  */
double fs(double s,double w,double c,double t,double r);
double af(double angle,double c,double t,double x,double r,double fs);
double csw(double t,double c,double r,double w,double fs);
double f3s(double x1, double x2, double y1, double y2, double b1, 
           double b2, double z );
double fai(double y1,double y2,double b1,double b2,double a);
double fai2(double y1,double y2,double b1,double b2,double a);
double fai3(double y1,double y2,double b1,double b2,double a);
double fai4(double y1,double y2,double b1,double b2,double a);
double fai5(double y1,double y2,double b1,double b2,double a);
double f2s(double x1, double x2, double y1, double y2, double z);
double fa(double y1, double y2, double b1, double b2, double a);
double sindexcell(double s,double a,double c1,double c2, 
                  double t1,double t2,double x1,double x2,double r,
                  double *sat);


//fgrid fslope,ffos,fsat;
//igrid lterg;

int *reg;  //TODO It would be better to have these not be global but pass them through function calling statements
int noreg;
#define MAXLN 4096	// originally it was defined in gridio.h

/*********************************************************************/
int readline(FILE *fp,char *fline)
{
  int i = 0, ch;
  
  for(i=0; i< MAXLN; i++)
  {
    ch = getc(fp);

    if(ch == EOF) { *(fline+i) = '\0'; return(EOF); }
    else          *(fline+i) = (char)ch;

    if((char)ch == '\n') { *(fline+i) = '\0'; return(0); }

  }
  return(1);
}

int getRegionIndex(char* calpfile, long nor)
{
	int j, index,rno;
	float tqmin,tqmax,cmin,cmax,tphimin,tphimax;  

	FILE *fp;
	fp = fopen(calpfile, "r");
	if (fp == NULL)return 15;
	float rhos;
	char headers[MAXLN];
	readline(fp, headers);
	reg = (int*)calloc(nor, sizeof(int));
	index = 0;
	//  2/14/16 DGT switched this to for loop to avoid cases where index was >= nor and overflowed allocated memory
	for(index=0; index < nor; index++)				  
	{			   
		j = fscanf(fp,"%i,%f,%f,%f,%f,%f,%f,%f \n", &rno, &tqmin,
					&tqmax, &cmin, &cmax, &tphimin, &tphimax, &rhos);  //TODO fix up based on indexing in to lookup table
		reg[index] = rno;			
	}

	fclose(fp);
 
	return 0;
}

int findRegIndex(short regno, int nor, short ndvter)
{
	//  This function returns the array position index from the global array of regions 'reg' that matches the input 'regno'
	//  -1 is returned if a match is not found in the case where the input is no data or no match is found
	//  DGT 12/31/14 changed to have short regno since these are values from a short grid 
	int index = -1;  //Default to -1 for no data return
	if (regno == ndvter) return index;
	
	for (index = 0; index < nor; index++)
	{
		if(regno == reg[index])
			//printf("returning index in findRegionIndex(): %d.\n", index);
			return index;
	}
	//  if drop through loop then there was no match - return no data
	index = -1;
	return index;  
}

int sindexcombined(char *slopefile,  char *scaterrainfile, char *scarminroadfile, char* scarmaxroadfile,
           char *tergridfile, char *terparfile, char *satfile, char* sincombinedfile, double Rminter, double Rmaxter, 
		   double *par)
{	
	MPI_Init(NULL,NULL);{
	FILE *fp;
	int i, j, mter, err, filetype, index, rno, nx, ny;
	float ndva, ndvs;
	short *ndvter;
	int nor;
	double X1, X2, cellsat, dx, dy;
	float rs, rw, g;
	float tmin, tmax, cmin, cmax, tphimin, tphimax;
  
	const double PI = 3.14159265358979;

	struct calreg{
		float tmin;
		float tmax;
		float cmin;
		float cmax;
		float tphimin;
		float tphimax;
		float r;
	} ;  /* this could be malloced or should be checked
                    to ensure fewer than 100 regions */

	int rank, size;
	MPI_Comm_rank(MCW, &rank);
	MPI_Comm_size(MCW, &size);
	if(rank == 0) printf("SinmapSI version %s\n", TDVERSION);

	double begin,end;
	//Begin timer
	begin = MPI_Wtime();

	g=par[0];
	rw=par[1];
	nor = 0;
	
	struct calreg *region;	

	char headers[MAXLN];

	// find number of regions
	fp = fopen(terparfile, "r");
	if (fp == NULL)return 15;

	// Pabitra (9/10/2015): the following code to count the number of lines 
	// handles if the last line does not end with end of line character
	while ( fgets ( headers, sizeof headers, fp) != NULL)
	{
		nor++;
	}
	/*while (readline(fp,headers) != EOF)
	{
		nor++;
	}*/
	(std::fclose)(fp);

	// number of regions is  total lines in the file minus 1 (for the header text line)
	nor = nor - 1;
		
	region = (calreg*)calloc(nor, sizeof(calreg));
	getRegionIndex(terparfile, nor);

	fp = fopen(terparfile, "r");	
	readline(fp,headers);
	//  i = -1;  DGT 12/31/14 Not needed
	index = 0;
	do
	{
		j = fscanf(fp,"%i,%f,%f,%f,%f,%f,%f,%f \n", &rno, &tmin,
					&tmax, &cmin, &cmax,  &tphimin, &tphimax, &rs);
		
		region[index].tmin = tmin;
		region[index].tmax = tmax;
		region[index].cmin  = cmin;
		region[index].cmax  = cmax;
		region[index].tphimin =tphimin * PI/180 ;
		region[index].tphimax = tphimax * PI/180;
		region[index].r = rw/rs;
		index++;
	}while (index < nor);

	(std::fclose)(fp);


	//Create tiff object, read and store header info
	tiffIO slp(slopefile, FLOAT_TYPE);
	long totalX = slp.getTotalX();
	long totalY = slp.getTotalY();
	dx = slp.getdxA();
	dy = slp.getdyA();

	if(rank==0)
	{
		float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
		fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
		fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
		fflush(stderr);
	}

	tiffIO sca(scaterrainfile, FLOAT_TYPE);
	if(!slp.compareTiff(sca)) { //Unhappy error message and error return
		if(rank==0)fprintf(stderr,"Slope (%s) and Contributing area (%s) files have different dimensions",slopefile, scaterrainfile);
		return 1; 
	}

	tiffIO *sca_min = NULL;
	if (*scarminroadfile != NULL)
	{
		sca_min = new tiffIO(scarminroadfile, FLOAT_TYPE);	
	    if(!slp.compareTiff(*sca_min)) { //Unhappy error message and error return
			if(rank==0)fprintf(stderr,"Slope (%s) and road min contributing area (%s) files have different dimensions",slopefile, scarminroadfile);
			return 1; 
	    }
	}
	
	tiffIO *sca_max = NULL;
	if (*scarmaxroadfile != NULL)
	{
		sca_max = new tiffIO(scarmaxroadfile, FLOAT_TYPE);		
	    if(!slp.compareTiff(*sca_max)) { //Unhappy error message and error return
			if(rank==0)fprintf(stderr,"Slope (%s) and road max contributing area (%s) files have different dimensions",slopefile, scarmaxroadfile);
			return 1; 
		}
	}
		
	tiffIO cal(tergridfile, SHORT_TYPE);
    if(!slp.compareTiff(cal)) { //Unhappy error message and error return
		if(rank==0)fprintf(stderr,"Slope (%s) and road max contributing area (%s) files have different dimensions",slopefile, tergridfile);
		return 1; 
	}
	
	//Create partition and read data from slope file
	tdpartition *slpData;
	slpData = CreateNewPartition(slp.getDatatype(), totalX, totalY, dx, dy, slp.getNodata());
	nx = slpData->getnx();
	ny = slpData->getny();
	ndvs = *(float*)slp.getNodata();	
	int xstart, ystart;
	slpData->localToGlobal(0, 0, xstart, ystart);
	// TODO: put similar comments for the other savedxdyc() function used in this function
	// for a complete slpData partition would need to run slpData->savedxdyc(slp) to save lat variable cell sizes
	// here not done as cell size not needed
	//slpData->savedxdyc(slp);

	slp.read(xstart, ystart, ny, nx, slpData->getGridPointer());

	//Create partition and read data from clibration grid file
	tdpartition *calData;
	calData = CreateNewPartition(cal.getDatatype(), totalX, totalY, dx, dy, -1);
	calData->localToGlobal(0, 0, xstart, ystart);
	calData->savedxdyc(cal);
	cal.read(xstart, ystart, ny, nx, calData->getGridPointer());
			
	ndvter = (short*)cal.getNodata();	
			
	//Create partition and read data from sca grid file
	tdpartition *scaData;
	scaData = CreateNewPartition(sca.getDatatype(), totalX, totalY, dx, dy, sca.getNodata());
	scaData->localToGlobal(0, 0, xstart, ystart);
	scaData->savedxdyc(sca);
	sca.read(xstart, ystart, ny, nx, scaData->getGridPointer());

	//Create partition and read data from sca min grid file
	tdpartition *scaMinData = NULL;
	if (sca_min != NULL)
	{		
		scaMinData = CreateNewPartition(sca_min->getDatatype(), totalX, totalY, dx, dy, sca_min->getNodata());
		scaMinData->localToGlobal(0, 0, xstart, ystart);
		scaMinData->savedxdyc(*sca_min);
		sca_min->read(xstart, ystart, ny, nx, scaMinData->getGridPointer());
	}
	
	//Create partition and read data from sca max grid file
	tdpartition *scaMaxData = NULL;
	if(sca_max != NULL)
	{		
		scaMaxData = CreateNewPartition(sca_max->getDatatype(), totalX, totalY, dx, dy, sca_max->getNodata());
		scaMaxData->localToGlobal(0, 0, xstart, ystart);
		scaMaxData->savedxdyc(*sca_max);
		sca_max->read(xstart, ystart, ny, nx, scaMaxData->getGridPointer());
	}
	

	short cal_cell_value = 0;
	float slope_cell_value = 0;
	float sca_cell_value = 0;
	float sca_min_cell_value = 0;
	float sca_max_cell_value = 0;
	float csi_data = 0;
	int region_index = 0;

	//Create empty partition to store csi data
	tdpartition *csiData;
	csiData = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, -1.0f);

	//Create empty partition to store sat data
	tdpartition *satData;
	satData = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, -1.0f);

	//Share information and set borders to zero
	slpData->share();
	scaData->share();
	if(scaMinData != NULL)
	{
		scaMinData->share();
	}
	
	if(scaMaxData != NULL)
	{
		scaMaxData->share();
	}

	calData->share();
	csiData->clearBorders();
	satData->clearBorders();	
	
	for(j = 0; j < ny; j++) {
		for(i = 0; i < nx; i++ ) {
			calData->getData(i, j, cal_cell_value);
			slpData->getData(i, j, slope_cell_value);
			scaData->getData(i, j, sca_cell_value);
			if(scaMinData != NULL)
			{
				scaMinData->getData(i, j, sca_min_cell_value);
			}

			if(scaMaxData != NULL)
			{
				scaMaxData->getData(i, j, sca_max_cell_value);
			}
			
			region_index = findRegIndex(cal_cell_value, nor, *ndvter);
			
			//  DGT 12/31/14.  Changed the logic below from region_index < *ndvter to region_index < 0 to allow for *ndvter to be positive
			if (region_index < 0 || sca_cell_value < 0 || sca_min_cell_value < 0 || sca_max_cell_value < 0 || slope_cell_value == ndvs){
				csiData->setToNodata(i, j);
				satData->setToNodata(i, j);				
			}
			else{
				
				if(Rmaxter != 0 || sca_max_cell_value != 0){
					X2 = (sca_cell_value * Rmaxter + sca_max_cell_value)/region[region_index].tmin;
					X1 = (sca_cell_value * Rminter + sca_min_cell_value)/region[region_index].tmax;	
					csi_data = (float)sindexcell(slope_cell_value, 1,
								region[region_index].cmin, region[region_index].cmax, 
								region[region_index].tphimin, region[region_index].tphimax,
								X1, X2, region[region_index].r, &cellsat);
				
									
					csiData->setData(i, j, csi_data);
					satData->setData(i, j, float(cellsat));
					
				}				
			}				
		}
	}
	
	//  DGT 12/31/14 commented passing of borders.  This function acts locally so does not need any border sharing
	//Pass information
	//csiData->addBorders();		
	//satData->addBorders();	

	//Clear out borders
	//csiData->clearBorders();
	//satData->clearBorders();

	//Stop timer
	end = MPI_Wtime();
 	double compute, temp;
    compute = end-begin;

    MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
    compute = temp/size;

    if( rank == 0)
	{
		printf("Compute time: %f\n",compute);		
	}

	//Create and write to the csi TIFF file
	float aNodata = -1.0f;
	tiffIO csi(sincombinedfile, FLOAT_TYPE, &aNodata, slp);
	csi.write(xstart, ystart, ny, nx, csiData->getGridPointer());
	//Create and write to the sat TIFF file
	tiffIO sat(satfile, FLOAT_TYPE, &aNodata, slp);
	sat.write(xstart, ystart, ny, nx, satData->getGridPointer());

	double writetime=MPI_Wtime()-end;
	MPI_Allreduce (&writetime, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	writetime=temp/size;
	if( rank == 0)
	{
		printf("File write time: %f\n",writetime);		
	}


	}MPI_Finalize();

	return 0;
}


/*******************************************************************/
/* New code that is conversion of DGT SPlus probabilistic theory code */

/* function to compute stability index with potentially 
   uncertain parameters  for a specific grid cell */
double sindexcell(double s, double a, double c1, double c2, 
                  double t1, double t2, double x1, double x2, double r,
                  double *sat)
{
		/*  c1, c2  bounds on dimensionless cohesion
			t1,t2 bounds on friction angle
			x1,x2 bounds on wetness parameter q/T
			r Density ratio
			s slope angle
			a specific catchment area */

		double cs, ss, w1, w2, fs2, fs1, cdf1, cdf2, y1, y2, as, si;

		/* cng - added this to prevent division by zero */
		if (s == 0.0)
		{
		  *sat = 3.0;
		  return 10;
		}

		/*  DGT - The slope in the GIS grid file is the tangent of the angle  */
		s = atan(s);
		/*  t1 and t2 input as angle must be converted to tan  */
		t1 = tan(t1);
		t2 = tan(t2);

		cs = cos(s);
		ss = sin(s);

		if(x1 > x2)
		{  /*  Error these are wrong way round - switch them  */
			w1 = x2;   /*  Using w1 as a temp variable  */
			x2 = x1;
			x1 = w1;
		}
		/* Wetness is coded between 0 and 1.0 based on the lower T/q parameter 
		  (this is conservative).  If wetness based on lower T/q is > 1.0 and wetness based 
		  on upper T/q <1.0, then call it the "threshold saturation zone" and hard code it 
		  to 2.0.  Then if wetness based on upper T/q >1.0, then call it the 
		  "saturation zone" and hard code it to 3.0.  
		  Lower T/q correspounds to upper q/T = x = x2 ->  w2  */
		w2 = x2*a/ss;
		w1 = x1*a/ss;
		*sat = w2;
		if(w2 > 1.0)
		{
			w2 = 1.0;
			*sat = 2.0;
		}
		if(w1 > 1.0)
		{
			w1 = 1.0;
			*sat = 3.0;
		}
		
		fs2 = fs(s,w1,c2,t2,r);

		if (fs2 < 1)  /* always unstable */
		{
		si  =  0;
		}
		else
			{
				fs1 = fs(s,w2,c1,t1,r);
				if(fs1 >= 1) /*  Always stable */
				{
 					si  =  fs1;
				}
				else
					{
						if(w1 == 1) /* region 1 */
						{
							si = 1-f2s(c1/ss,c2/ss,cs*(1-r)/ss*t1,cs*(1-r)/ss*t2,1);
						}
						else
							{
								if(w2 == 1) /* region 2 */
								{
									as = a/ss;
									y1 = cs/ss*(1-r);
									y2 = cs/ss*(1-x1*as*r);
									cdf1 = (x2*as-1)/(x2*as-x1*as)*f2s(c1/ss,c2/ss,cs*(1-r)/ss*t1,cs*(1-r)/ss*t2,1);
									cdf2 = (1-x1*as)/(x2*as-x1*as)*f3s(c1/ss,c2/ss,y1,y2,t1,t2,1);
									si = 1-cdf1-cdf2;
								}
								else  /* region 3 */
									{
										as = a/ss;
										y1 = cs/ss*(1-x2*as*r);
										y2 = cs/ss*(1-x1*as*r);
										si = 1-f3s(c1/ss,c2/ss,y1,y2,t1,t2,1);
									}
							}
  					}
			}

		/* cng - need to limit the size spread for arcview since it has
		   difficulties with equal intervals over a large range of numbers */
		if (si > 10.0)
		  si = 10.0;

		return si;
}
/**********************************************************************/
/*  Generic function to implement dimensionless form of infinite slope
    stability model */
double fs(double s,double w,double c,double t,double r)
{
	double cs, ss;

	cs = cos(s);
	ss = sin(s);
	return ((c+cs*(1-w*r)*t)/ss);
}


/**********************************************************************/
/*  function to return specific catchment area that results in the given factor of safety
    for the given parameters */
double af(double angle,double c,double t,double x,double r,double fs)
{
/* angle = slope angle (radians)
   c = dimensionless cohesion
   t = tan of friction angle
   x = q/T wetness indicator  w = q a/(T sin slope angle) = x a/sin ...
   r = density ratio
   fs = safety factor  */

	double ss, cs, a;

	ss = sin(angle);
	cs = cos(angle);
	a = ss*(1-(fs*ss-c)/(t*cs))/(r*x);

	return a;
}
/**********************************************************************/
/* Function to return angle of slope with given conditions and safety factor*/
double csw(double t,double c,double r,double w,double fs)
{
	double rwt, cs;

	rwt = (1-r*w)*t;
	cs = 0;   /*  default - this will give 90 degrees for large cohesion */
	if(c < 1)cs = (sqrt(fs*fs*(fs*fs+rwt*rwt-c*c))-c*rwt)/(fs*fs+rwt*rwt);
	return acos(cs);
}
/**********************************************************************/
double f3s(double x1, double x2, double y1, double y2, double b1, 
           double b2, double z )
{
	double p;

	if (x2 < x1 || y2 < y1 || b2 < b1)
		{
	/*	cat("Bad input parameters, upper limits less than lower limits\n");
		p = NA; */
	/* cng - make a big number to be subtracted and yield ndv */
	/*  p = 1000.0 */
	  p = 1000.0;
	  }
	else if	(x1 < 0 || y1 < 0 || b1 < 0)
		{
	/* 	cat("Bad input parameters, Variables cannot be negative\n");
		p = NA; */
	  p = 1000.;
	  }
	else
		{
		if(y1 == y2 || b1 == b2)
			{  /* degenerate on y or b - reverts to additive convolution */
		p = f2s(x1,x2,y1*b1,y2*b2,z);
		}
		else
			{
		if(x2 == x1) 
    		{
		  p = fa(y1, y2, b1, b2, z - x1);
		  }
		else 
		  {
		  p = (fai(y1, y2, b1, b2, z - x1) - 
      			 fai(y1, y2, b1, b2, z - x2)) / (x2 - x1);
		  }
			}
		}
	return p;
}
/**********************************************************************/
/*  y2 > y1 and x2 > x1. */
double fai(double y1,double y2,double b1,double b2,double a)
{
	double p, t, a1, a2, a3, a4, c2, c3, c4, c5;

	/* p = rep(0,length(a)); */
	p = 0.0;

	if (y1*b2 > y2*b1)
		{  					/* Invoke symmetry and switch so that y1*b2 < y2*b1 */
		t = y1;
		y1 = b1;
		b1 = t;
		t = y2;
		y2 = b2;
		b2 = t;
		}
								/* define limits */
	a1 = y1*b1;
	a2 = y1*b2;
	a3 = y2*b1;
	a4 = y2*b2;

								/* Integration constants */
	c2 =  - fai2(y1,y2,b1,b2,a1);

								/* Need to account for possibility of 0 lower bounds */
	if(a2 == 0)
	  c3  =  0;
	else
	  c3 =  fai2(y1,y2,b1,b2,a2)+c2-fai3(y1,y2,b1,b2,a2);

	if(a3 == 0)
	  c4  =  0;
	else
	  c4 =  fai3(y1,y2,b1,b2,a3)+c3-fai4(y1,y2,b1,b2,a3);

	c5 =  fai4(y1,y2,b1,b2,a4)+c4-fai5(y1,y2,b1,b2,a4);

	/* evaluate */
	p = (a1 < a && a <= a2) ? fai2(y1,y2,b1,b2,a)+c2 : p;
	p = (a2 < a && a <= a3) ? fai3(y1,y2,b1,b2,a)+c3 : p;
	p = (a3 < a && a <  a4) ? fai4(y1,y2,b1,b2,a)+c4 : p;
	p = (a >= a4) ? fai5(y1,y2,b1,b2,a)+c5 : p;

	return p;
}
/**********************************************************************/
double fai2(double y1,double y2,double b1,double b2,double a)
{
	double adiv;

	adiv = (y2-y1)*(b2-b1);
	return (a*a*log(a/(y1*b1))/2 - 3*a*a/4+y1*b1*a)/adiv;
}
/**********************************************************************/
double fai3(double y1,double y2,double b1,double b2,double a)
{
	double adiv;

	adiv = (y2-y1)*(b2-b1);
	return (a*a*log(b2/b1)/2-(b2-b1)*y1*a)/adiv;
}
/**********************************************************************/
double fai4(double y1,double y2,double b1,double b2,double a)
{
	double adiv;
	adiv = (y2-y1)*(b2-b1);
	return (a*a*log(b2*y2/a)/2+3*a*a/4+b1*y1*a-b1*y2*a-b2*y1*a)/adiv;
}
/**********************************************************************/
double fai5(double y1,double y2,double b1,double b2,double a)
{
	return a;
}
/**********************************************************************/
double f2s(double x1, double x2, double y1, double y2, double z)
{
	double p, mn, mx, d, d1, d2;

	/* p = rep(0, length(z)); */
	p = 0.0;

	mn = (std::min)(x1 + y2, x2 + y1);
	mx = (std::max)(x1 + y2, x2 + y1);
	d1 = (std::min)(x2 - x1, y2 - y1);
	d = z - y1 - x1;
	d2 = (std::max)(x2 - x1, y2 - y1);
	p = (z > (x1 + y1) && z < mn) ? (d*d)/(2 * (x2 - x1) * (y2 - y1)) : p;
	p = ((mn <= z) & (z <= mx)) ? (d - d1/2)/d2 : p;
	p = ((mx < z) & (z < (x2 + y2))) ? 1-pow((z-y2-x2),2)/(2*(x2-x1)*(y2-y1)) : p;
	p =  z >= (x2 + y2) ? 1 : p;

	return p;
}

/**********************************************************************/
double fa(double y1, double y2, double b1, double b2, double a)
{
	double p, adiv, t;

	/* p = rep(0, length(a)); */
	p = 0.0;

	if(y1 * b2 > y2 * b1) 
		{ /* Invoke symmetry and switch so that y1*b2 < y2*b1 */
		t = y1;
	  y1 = b1;
	  b1 = t;
			/* was   y <- y2  which is wrong */
	  t = y2;
	  y2 = b2;
	  b2 = t;
	  }

	adiv = (y2 - y1) * (b2 - b1);
	p = (y1 * b1 < a && a <= y1 * b2) ? 
		(a * log(a/(y1 * b1)) - a + y1 * b1)/adiv : p;
	p = (y1 * b2 < a && a <= y2 * b1) ? 
		(a * log(b2/b1) - (b2 - b1) * y1)/adiv : p;
	p = (y2 * b1 < a && a < y2 * b2) ? 
		(a * log((b2 * y2)/a) + a + b1 * y1 - b1 * y2 - b2 * y1)/adiv : p;
	p =  a >= (b2 * y2) ? 1 : p;

	return p;
}


