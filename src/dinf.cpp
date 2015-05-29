/*  DinfFlowDir function to compute flow direction based on D-infinity flow model.
     
  David Tarboton, Jeremy Neff, Dan Watson
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

#include <mpi.h>

#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include <math.h>
#include "Node.h"
using namespace std;

//double fact[9];
double **fact;

//int setPosDirDinf(tdpartition *elevDEM, tdpartition *flowDir, tdpartition *slope, tdpartition *area, int useflowfile);
long setPosDirDinf(tdpartition *elevDEM, tdpartition *flowDir, tdpartition *slope, int useflowfile);
long resolveflats( tdpartition *elevDEM, tdpartition *flowDir, queue<node> *que, bool &first);
//Checks if cells cross
int dontCross( int k, int i, int j, tdpartition *flowDir) {
	long n1, n2, c1, c2, ans=0;
	long in1,jn1,in2,jn2;
	short tempShort;
	float tempFloat;
	switch(k){
		case 2:
			n1=1; c1=4; n2=3; c2=8;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempFloat)) == c1 || (flowDir->getData(in2,jn2,tempFloat)) == c2)
				ans=1;
                	break;
 		case 4:
			n1=3; c1=6; n2=5; c2=2;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempFloat)) == c1 || (flowDir->getData(in2,jn2,tempFloat)) == c2)
                   		ans=1;
                	break;
          	case 6:
                	n1=7; c1=4; n2=5; c2=8;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempFloat) == c1 ) )
                   		ans=1;
			if (flowDir->getData(in2,jn2,tempFloat) == c2 )
                   		ans=1;
                	break;
          	case 8:
			n1=1; c1=6; n2=7; c2=2;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempFloat)) == c1 || (flowDir->getData(in2,jn2,tempFloat)) == c2)
                        	ans=1;
                	break;
		default: break;
	}
	return(ans);
}

//Set positive flowdirections of elevDEM

int setdir( char* demfile, char* angfile, char *slopefile, char *flowfile, int useflowfile) {

	MPI_Init(NULL,NULL);{

	//Only needed to output time
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)
	{
		printf("DinfFlowDir version %s\n",TDVERSION);
		fflush(stdout);
	}

	double begint = MPI_Wtime();

	//Read DEM from file
//	printf("Before demread rank: %d\n",rank);
	tiffIO dem(demfile, FLOAT_TYPE);
	long totalX = dem.getTotalX();
	long totalY = dem.getTotalY();
	double dxA = dem.getdxA();
	double dyA = dem.getdyA();
	
	
	tdpartition *elevDEM;
	elevDEM = CreateNewPartition(dem.getDatatype(), totalX, totalY, dxA, dyA, dem.getNodata());
	int xstart, ystart;
	int nx = elevDEM->getnx();
	int ny = elevDEM->getny();
	elevDEM->localToGlobal(0, 0, xstart, ystart);
	elevDEM->savedxdyc(dem);
	double headert = MPI_Wtime();

	if(rank==0)
	{
		float timeestimate=(2.8e-9*pow((double)(totalX*totalY),1.55)/pow((double) size,0.65))/60+1;  // Time estimate in minutes
		//fprintf(stderr,"%d %d %d\n",totalX,totalY,size);
		fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
		fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
		fflush(stderr);
	}


	dem.read(xstart, ystart, ny, nx, elevDEM->getGridPointer());
	elevDEM->share();

	double readt = MPI_Wtime();
	
	//Creates empty partition to store new flow direction 
	tdpartition *flowDir;
	flowDir = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	//If using flowfile is enabled, read it in
	//tdpartition *imposedflow, *area;
	//area = CreateNewPartition(LONG_TYPE, totalX, totalY, dx, dy, MISSINGLONG);
//	area = new linearpart<long>;
//	area->init(totalX, totalY, dx, dy, MPI_LONG, long(-1));

	//if( useflowfile == 1) {
	//	tiffIO flow(flowfile,SHORT_TYPE);
	//	imposedflow = CreateNewPartition(flow.getDatatype(), flow.getTotalX(), flow.getTotalY(), flow.getdx(), flow.getdy(), flow.getNodata());

	//	if(!dem.compareTiff(flow)) {
	//		printf("Error using imposed flow file.\n");	
	//		return 1;
	//	}
	//	


	//	int i,j;
	//	for( j = 0; j < elevDEM->getny(); j++) {
	//		for( i=0; i < elevDEM->getnx(); i++ ) {
	//			short data;
	//			imposedflow->getData(i,j,data);
	//			if (imposedflow->isNodata(i,j) || !imposedflow->hasAccess(i-1,j) || !imposedflow->hasAccess(i+1,j) ||
	//			    !imposedflow->hasAccess(i,j-1) || !imposedflow->hasAccess(i,j+1)){
	//				//Do nothing
	//			}
	//			else if((data>0) && data <=8 ) 
	//					flowDir->setData(i,j,(float)data);
	//		}
	//	}
	////	delete imposedflow;
	//	//TODO - why is this here?
	//	//darea( &flowDir, &area, NULL, NULL, 0, 1, NULL, 0, 0 );
	//}

	//Creates empty partition to store new slopes
	long numFlat, totalNumFlat, lastNumFlat;
	double computeSlopet;
	{
	tdpartition *slope;
	float slopeNodata = -1.0f;
	slope = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, slopeNodata);

	//numFlat = setPosDirDinf(elevDEM, flowDir, slope, area, useflowfile);
	numFlat = setPosDirDinf(elevDEM, flowDir, slope, useflowfile);

	//Stop timer
	computeSlopet = MPI_Wtime();
	tiffIO slopeIO(slopefile, FLOAT_TYPE, &slopeNodata, dem);
	slopeIO.write(xstart, ystart, ny, nx, slope->getGridPointer());
	}  // This bracket intended to destruct slope partition and release memory

	double writeSlopet = MPI_Wtime();

	MPI_Allreduce(&numFlat,&totalNumFlat,1,MPI_LONG,MPI_SUM,MCW);
	if(rank==0)
	{
		fprintf(stderr,"All slopes evaluated. %ld flats to resolve.\n",totalNumFlat);
		fflush(stderr);
	}

	queue<node> que;  //  que to be used in resolveflats
	bool first=true;  //  Variable to be used in iteration to know whether first or subsequent iteration
	if( totalNumFlat > 0)
	{
		lastNumFlat=totalNumFlat;
		totalNumFlat = resolveflats(elevDEM, flowDir, &que, first);  
		//Repeatedly call resolve flats until there is no change 
		while(totalNumFlat > 0  && totalNumFlat < lastNumFlat)
		{
			if(rank==0)
			{
				fprintf(stderr,"Iteration complete. Number of flats remaining: %ld\n",totalNumFlat);
				fflush(stderr);
			}
			lastNumFlat=totalNumFlat;
			totalNumFlat = resolveflats(elevDEM, flowDir, &que, first);
		}
	}

	//Timing info
	double computeFlatt = MPI_Wtime();
//	printf("Before angwrite rank: %d\n",rank);
	float flowDirNodata=MISSINGFLOAT;
	tiffIO flowIO(angfile, FLOAT_TYPE, &flowDirNodata, dem);
	flowIO.write(xstart, ystart, ny, nx, flowDir->getGridPointer());

	double writet = MPI_Wtime();

        double headerRead, dataRead, computeSlope, writeSlope, computeFlat,writeFlat, write, total,temp;
        headerRead = headert-begint;
        dataRead = readt-headert;
        computeSlope = computeSlopet-readt;
        writeSlope = writeSlopet-computeSlopet;
        computeFlat = computeFlatt-writeSlopet;
        writeFlat = writet-computeFlatt;
        total = writet - begint;

        MPI_Allreduce (&headerRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        headerRead = temp/size;
        MPI_Allreduce (&dataRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = temp/size;
        MPI_Allreduce (&computeSlope, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        computeSlope = temp/size;
        MPI_Allreduce (&computeFlat, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        computeFlat = temp/size;
        MPI_Allreduce (&writeSlope, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        writeSlope = temp/size;
        MPI_Allreduce (&writeFlat, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        writeFlat = temp/size;
        MPI_Allreduce (&total, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = temp/size;

        if( rank == 0)
                //  These times are only for  process 0 - not averaged across processors.  This may be an approximation - but probably do not want to hold processes up to synchronize just so as to get more accurate timing
                printf("Processors: %d\nHeader read time: %f\nData read time: %f\nCompute Slope time: %f\nWrite Slope time: %f\nResolve Flat time: %f\nWrite Flat time: %f\nTotal time: %f\n",
                  size,headerRead,dataRead, computeSlope, writeSlope,computeFlat,writeFlat,total);

	}
	//MPI_Barrier(MCW);
	MPI_Finalize();
	return 0;
}
void   VSLOPE(float E0,float E1, float E2,
			  float D1,float D2,float DD,
			  float *S,float *A)
{
	//SUBROUTINE TO RETURN THE SLOPE AND ANGLE ASSOCIATED WITH A DEM PANEL 
	float S1,S2,AD;
	if(D1!=0)
		S1=(E0-E1)/D1;
	if(D2!=0)
		S2=(E1-E2)/D2;

	if(S2==0 && S1==0) *A=0;
	else
		*A= (float) atan2(S2,S1);
	AD= (float) atan2(D2,D1);
	if(*A  <   0.)
	{
		*A=0.;
		*S=S1;
	}
	else if(*A > AD)
	{
		*A=AD;
		*S=(E0-E2)/DD;
	}
	else
		*S= (float) sqrt(S1*S1+S2*S2);
}
// Sets only flowDir only where there is a positive slope
// Returns number of cells which are flat

void   SET2(int I, int J,float *DXX,float DD, tdpartition *elevDEM, tdpartition *flowDir, tdpartition *slope)
{
	double dxA = elevDEM->getdxA();
	double dyA = elevDEM->getdyA();
	float SK[9];
	float ANGLE[9];
	float SMAX;
	float tempFloat;
	int K;
	int KD;

	int ID1[]= {0,1,2,2,1,1,2,2,1 }; 
	int ID2[]= {0,2,1,1,2,2,1,1,2};
	int I1[] = {0,0,-1,-1,0,0,1,1,0 };
	int I2[] = {0,-1,-1,-1,-1,1,1,1,1};
	int J1[] = {0,1,0,0,-1,-1,0,0,1};
	int J2[] = {0,1,1,-1,-1,-1,-1,1,1};
	float  ANGC[]={0,0.,1.,1.,2.,2.,3.,3.,4.};
	float  ANGF[]={0,1.,-1.,1.,-1.,1.,-1.,1.,-1.};


	for(K=1; K<=8; K++)
	{
		VSLOPE(
			elevDEM->getData(J,I,tempFloat),//felevg.d[J][I],
			elevDEM->getData(J+J1[K],I+I1[K],tempFloat),//[felevg.d[J+J1[K]][I+I1[K]],
			elevDEM->getData(J+J2[K],I+I2[K],tempFloat),//felevg.d[J+J2[K]][I+I2[K]],
			DXX[ID1[K]],
			DXX[ID2[K]],
			DD,
			&SK[K],
			&ANGLE[K]
		);
	}
	tempFloat = -1;
	SMAX=0.;
	KD=0;
	flowDir->setData(J,I,tempFloat);  //USE -1 TO INDICATE DIRECTION NOT YET SET 
	for(K=1; K<=8; K++)
	{
		if(SK[K] >  SMAX)
		{
			SMAX=SK[K];
			KD=K;
		}
	}

	if(KD  > 0)
	{
		tempFloat = (float) (ANGC[KD]*(PI/2)+ANGF[KD]*ANGLE[KD]) ;
		flowDir->setData(J,I,tempFloat);//set to angle
	}
	slope->setData(J,I,SMAX);
}
//Overloaded SET2 for use in resolve flats when slope is no longer recorded.  Also uses artificial elevations and actual elevations
void   SET2(int I, int J,float *DXX,float DD, tdpartition *elevDEM, tdpartition *elev2, tdpartition *flowDir, tdpartition *dn)
{
	float SK[9];
	float ANGLE[9];
	float SMAX=0.0;
	float tempFloat;
	short tempShort, tempShort1, tempShort2;
	int K;
	int KD=0;

	int ID1[]= {0,1,2,2,1,1,2,2,1 }; 
	int ID2[]= {0,2,1,1,2,2,1,1,2};
	int I1[] = {0,0,-1,-1,0,0,1,1,0 };
	int I2[] = {0,-1,-1,-1,-1,1,1,1,1};
	int J1[] = {0,1,0,0,-1,-1,0,0,1};
	int J2[] = {0,1,1,-1,-1,-1,-1,1,1};
	float  ANGC[]={0,0.,1.,1.,2.,2.,3.,3.,4.};
	float  ANGF[]={0,1.,-1.,1.,-1.,1.,-1.,1.,-1.};
	bool diagOutFound=false;

	for(K=1; K<=8; K++)
	{
		dn->getData(J+J1[K],I+I1[K],tempShort1);//Check each square to see if it is in the flat.  If it is in the flat, use artifical elevations if not use real elevations
		dn->getData(J+J2[K],I+I2[K],tempShort2);//dn = 0 if it is not in the flat.  dn = 1 if in flat.
		if(tempShort1 <= 0 && tempShort2 <= 0) { //Both E1 and E2 are outside the flat get slope and angle
			float a=elevDEM->getData(J,I,tempFloat);
			float b=elevDEM->getData(J+J1[K],I+I1[K],tempFloat);
			float c=elevDEM->getData(J+J2[K],I+I2[K],tempFloat);
			VSLOPE(
				a,//E0
				b,//E1
				c,//E2
				DXX[ID1[K]],//dx or dy depending on ID1
				DXX[ID2[K]],//dx or dy depending on ID2
				DD,//Hypotenuse
				&SK[K],//Slope Returned
				&ANGLE[K]//Angle Returned
			);
			if(SK[K]>=0.0) //  Found an outlet
			{
				if(b>a)  // Outlet found had better be a diagonal, because it is not an edge
				{
					if(!diagOutFound)
					{
						diagOutFound=true;
						KD=K;
					}
				}
				else
				{    //  Here it is an adjacent outlet
					KD=K;
					break;
				}
			}

		}else if(tempShort1 <= 0 && tempShort2 >0){//E1 is outside of the flat and E2 is inside the flat. Use DEM elevations. tempShort2/E2 is in the artificial grid
			float a=elevDEM->getData(J,I,tempFloat);
			float b=elevDEM->getData(J+J1[K],I+I1[K],tempFloat);

			if(a>=b)
			{
				ANGLE[K]=0.0;
				SK[K]=0.0;
				KD=K;
				break;
			}
			short a1=elev2->getData(J,I,tempShort);
			short c1=elev2->getData(J+J2[K],I+I2[K],tempShort);
			short b1=max(a1,c1);
			VSLOPE(
				(float)a1,//felevg.d[J][I],
				(float)b1,//[felevg.d[J+J1[K]][I+I1[K]],
				(float)c1,//felevg.d[J+J2[K]][I+I2[K]],
				DXX[ID1[K]],//dx or dy
				DXX[ID2[K]],//dx or dy
				DD,//Hypotenuse
				&SK[K],//Slope Returned
				&ANGLE[K]//Angle Reutnred
			);
			if(SK[K]>SMAX){
				SMAX=SK[K];
				KD=K;
			}
		}else if(tempShort1 > 0 && tempShort2 <= 0){//E2 is out side of the flat and E1 is inside the flat, use DEM elevations
			float a=elevDEM->getData(J,I,tempFloat);
			//float b=elevDEM->getData(J+J1[K],I+I1[K],tempFloat);
			float c=elevDEM->getData(J+J2[K],I+I2[K],tempFloat);
			if(a>=c) 
			{
				if(!diagOutFound)
				{
					ANGLE[K]=(float) atan2(DXX[ID2[K]],DXX[ID1[K]]);
					SK[K]=0.0;
					KD=K;
					diagOutFound=true;
				}
			}
			else
			{
				short a1=elev2->getData(J,I,tempShort);
				short b1=elev2->getData(J+J1[K],I+I1[K],tempShort);
				short c1=max(a1,b1);
				VSLOPE(
					(float)a1,//felevg.d[J][I],
					(float)b1,//[felevg.d[J+J1[K]][I+I1[K]],
					(float)c1,//felevg.d[J+J2[K]][I+I2[K]],
					DXX[ID1[K]],//dx or dy
					DXX[ID2[K]],//dx or dy
					DD,//Hypotenuse
					&SK[K],//Slope Returned
					&ANGLE[K]//Angle Reutnred
				);
				if(SK[K]>SMAX){
					SMAX=SK[K];
					KD=K;
				}

			}
		}else{//Both E1 and E2 are in the flat. Use artificial elevation to get slope and angle
			short a, b,c;
			a = elev2->getData(J,I,a);
			b = elev2->getData(J+J1[K],I+I1[K],b);
			c = elev2->getData(J+J2[K],I+I2[K],c);
			VSLOPE(
				(float)a,//felevg.d[J][I],
				(float)b,//[felevg.d[J+J1[K]][I+I1[K]],
				(float)c,//felevg.d[J+J2[K]][I+I2[K]],
				DXX[ID1[K]],//dx or dy
				DXX[ID2[K]],//dx or dy
				DD,//Hypotenuse
				&SK[K],//Slope Returned
				&ANGLE[K]//Angle Reutnred
			);
			if(SK[K]>SMAX){
				SMAX=SK[K];
				KD=K;
			}
		}
	}
	//USE -1 TO INDICATE DIRECTION NOT YET SET, 
	// but only for non pit grid cells.  Pits will have flowDir as no data
	if(!flowDir->isNodata(J,I))
	{
		tempFloat = -1;
		flowDir->setData(J,I,tempFloat);  
	}

	if(KD  > 0 )//We have a flow direction.  Calculate the Angle and save/write it.
	{
		tempFloat = (float) (ANGC[KD]*(PI/2)+ANGF[KD]*ANGLE[KD]);//Calculate the Angle
		if(tempFloat >= 0.0)//Make sure the angle is positive
			flowDir->setData(J,I,tempFloat);//set the angle in the flowPartition
	}
}
//int setPosDirDinf(tdpartition *elevDEM, tdpartition *flowDir, tdpartition *slope, tdpartition *area, int useflowfile)
long setPosDirDinf(tdpartition *elevDEM, tdpartition *flowDir, tdpartition *slope, int useflowfile) {
	double dxA = elevDEM->getdxA();
	double dyA = elevDEM->getdyA();
	long nx = elevDEM->getnx();
	long ny = elevDEM->getny();
	float tempFloat;double tempdxc,tempdyc;
	int i,j,k,in,jn, con;
	long numFlat = 0;

	//Set direction factors
	//for( k=1; k<= 8; k++ ){
	//	fact[k] = (double) (1./sqrt(d1[k]*dx*d1[k]*dx + d2[k]*d2[k]*dy*dy));
	//}

	



	tempFloat = 0;
	for( j = 0; j < ny; j++) {
		for( i=0; i < nx; i++ ) {

		
			//FlowDir is nodata if it is on the border OR elevDEM has no data
			if ( elevDEM->isNodata(i,j) || !elevDEM->hasAccess(i-1,j) || !elevDEM->hasAccess(i+1,j) || 
						!elevDEM->hasAccess(i,j-1) || !elevDEM->hasAccess(i,j+1) )  {
				//do nothing			
			}
			else { 
				//Check if cell is "contaminated" (neighbors have no data)
				//  set flowDir to noData if contaminated
				con = 0;
				for( k=1;k<=8 && con != -1;k++) {
					in=i+d1[k];
					jn=j+d2[k];
					if( elevDEM->isNodata(in,jn) ) con=-1;
				}
				if( con == -1 ) flowDir->setToNodata(i,j);
				//If cell is not contaminated,
				else {
					tempFloat= -1.;
					flowDir->setData(i,j,tempFloat);//set to -1
					elevDEM->getdxdyc(j,tempdxc,tempdyc);
		        
					
					float DXX[3] = {0,tempdxc,tempdyc};//tardemlib.cpp ln 1291
					float DD = sqrt(tempdxc*tempdxc+tempdyc*tempdyc);//tardemlib.cpp ln 1293
					SET2(j,i,DXX,DD, elevDEM,flowDir,slope);//i=y in function form old code j is x switched on purpose
					//  Use SET2 from serial code here modified to get what it has as felevg.d from elevDEM partition
					//  Modify to return 0 if there is a 0 slope.  Modify SET2 to output flowDIR as no data (do nothing 
					//  if verified initialization to nodata) and 
					//  slope as 0 if a positive slope is not found

					//setFlow( i,j, flowDir, elevDEM, area, useflowfile);
					if( flowDir->getData(i,j,tempFloat)==-1)
						numFlat++;
				}
			}	
		}
	}
	return numFlat;
}

//Resolve flat cells according to Garbrecht and Martz
long resolveflats( tdpartition *elevDEM, tdpartition *flowDir, queue<node> *que, bool &first) {
	elevDEM->share();
	flowDir->share();
	//Header data
	long totalx = elevDEM->gettotalx();
	long totaly = elevDEM->gettotaly();
	long nx = elevDEM->getnx();
	long ny = elevDEM->getny();
	double dxA = elevDEM->getdxA();
	double dyA = elevDEM->getdyA();

	int rank;
	MPI_Comm_rank(MCW,&rank);

	long i,j,k,in,jn;
	bool doNothing, done; double tempdxc,tempdyc;
	long numFlat;
	short tempShort;
	int32_t tempLong;
	float tempFloat;
	long numInc, numIncOld, numIncTotal;

	//create and initialize temporary storage for Garbrecht and Martz
	tdpartition *elev2, *dn, *s;
	elev2 = CreateNewPartition(SHORT_TYPE, totalx, totaly, dxA, dyA, 1);
	   //  The assumption here is that resolving a flat does not increment a cell value 
	   //  more than fits in a short
	dn = CreateNewPartition(SHORT_TYPE, totalx, totaly, dxA, dyA, 0);

	node temp;
	long nflat=0, iflat;
	//  First time through add flat grid cells indicated by flowdir = 0 on to queue
	//  The queue is retained for later passes so this only needs be done at beginning
	if(first)
	{
		first=false;
		for(j=0; j<ny; j++){
			for(i=0; i<nx; i++){
				if(!flowDir->isNodata(i,j))
				{
					if(flowDir->getData(i,j,tempFloat) < 0.0)
					{
						temp.x=i; temp.y=j; que->push(temp);
					}
				}
			}
		}
	}
 	nflat=que->size();
	dn->share();
	elev2->share();

	//incfall - drain toward lower ground
	// Setting up while loop to calculate elev2 - the grid draining to lower ground
	done = false;
	numIncOld = -1; //holds number of grid cells incremented on previous iteration 
	//  use -1 to force inequality on first pass through
	short st = 1; // used to indicate the level to which elev2 has been incremented 
	float elevDiff;
	numIncTotal = 0; 
	if(rank==0)
	{
		fprintf(stderr,"Draining flats towards lower adjacent terrain\n");  
		fflush(stderr);
	}

	while(numIncTotal != numIncOld){  // Continue to loop as long as grid cells are being incremented
		numInc = 0;
		numIncOld = numIncTotal;
		for(iflat=0; iflat < nflat; iflat++)
		{
			temp=que->front(); que->pop(); i=temp.x; j=temp.y; que->push(temp);

				doNothing=false;
				for(k=1; k<=8; k++){
					if(dontCross(k,i,j, flowDir)==0){
						jn = j + d2[k];
						in = i + d1[k];
					elevDiff = elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat);// get elevations and elevation difference.
					flowDir->getData(in,jn,tempFloat); //get current flow direction...
					if(elevDiff >= 0 && tempFloat >= 0.0) //adjacent cell drains and is equal or lower in elevation so this is a low boundary
						doNothing = true;//I don't have to do anything move on...
					else if(elevDiff == 0) //if neighbor is in flat
						if(elev2->getData(in,jn,tempShort) >=0 && elev2->getData(in,jn,tempShort)<st) //neighbor is not being incremented
							doNothing = true;//I don't have to do anything
				}
			}
			if(!doNothing){//if I still have to do something...
				elev2->addToData(i,j,short(1));//increment the s partition in this cell by one.
					numInc++;
				}
		}

		elev2->share();
		MPI_Allreduce(&numInc, &numIncTotal, 1, MPI_LONG, MPI_SUM, MCW);
		//  only break from while when total from all processes is no longer converging
		st++;
		if(rank==0)
		{
			fprintf(stderr,".");  // print a . at each pass to give an indication of progress
			fflush(stderr);
		}
	}
	if(numIncTotal > 0)  // Not all grid cells were resolved - pits remain
		// Remaining grid cells are unresolvable pits
	{
//			There are pits remaining - set direction to no data
		for(iflat=0; iflat < nflat; iflat++)
		{
			temp=que->front(); que->pop(); i=temp.x; j=temp.y; que->push(temp);
			doNothing=false;
				for(k=1; k<=8; k++){
					if(dontCross(k,i,j, flowDir)==0){
						jn = j + d2[k];
						in = i + d1[k];
						elevDiff = elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat);
						flowDir->getData(in,jn,tempFloat); 
						if(elevDiff >= 0 && tempFloat >= 0.) //adjacent cell drains and is equal or lower in elevation so this is a low boundary
								doNothing = true;
						else if(elevDiff == 0) //if neighbor is in flat
							if(elev2->getData(in,jn,tempShort) >=0 && elev2->getData(in,jn,tempShort)<st) //neighbor is not being incremented
								doNothing = true;
					}
				}
				if(!doNothing)
					flowDir->setData(i,j,MISSINGFLOAT);  // mark pit
			}
			flowDir->share();
		
		//numIncOld = numIncTotal;
		//int total = 0;
		//	MPI_Allreduce(&allDone, &total, 1, MPI_INT, MPI_SUM, MCW);//see if anybody as done any work.
		//if(total != 0)//Somebody did some work.
		//	done = false;
		//if(numIncOld == 0)//Nobody did any work! we are done.
		//	done = true;
	}
	//  DGT moved from above - write directly into elev2
	s = CreateNewPartition(SHORT_TYPE, totalx, totaly, dxA, dyA, 0);  //  Use 0 as no data to avoid need to initialize

	//incrise - drain away from higher ground
	done = false;
	numIncOld = 0;
	if(rank==0)
	{
		fprintf(stderr,"\nDraining flats away from higher adjacent terrain\n");  
		fflush(stderr);
	}

	while(!done){
		numInc = 0;
		for(iflat=0; iflat < nflat; iflat++)
		{
				temp=que->front(); que->pop(); i=temp.x; j=temp.y; que->push(temp);
				for(k=1; k<=8; k++){
					jn = j + d2[k];
					in = i + d1[k];
					if(elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat)<0)	//adjacent cell is higher
						dn->setData(i,j,short(1));//cell in flat
					if(dn->getData(in,jn,tempShort)>0 && s->getData(in,jn,tempShort)>0) //adjacent cell has been marked already
						dn->setData(i,j,short(1));
				}
		} 
		dn->share();
		for(j=0; j<ny; j++){
			for(i=0; i<nx; i++){//add dn values to s
				dn->getData(i,j,tempShort);
				s->addToData(i,j,(tempShort>0 ? short(1) : short(0)));
				if(tempShort>0) numInc++;
			}
		}
		s->share();
		dn->share();
		MPI_Allreduce(&numInc, &numIncTotal, 1, MPI_LONG, MPI_SUM, MCW);
		if(numIncTotal==numIncOld) done=true;
		numIncOld = numIncTotal;
		if(rank==0)
		{
			fprintf(stderr,".");  // print a . at each pass to give an indication of progress
			fflush(stderr);
		}
	}

	for(iflat=0; iflat < nflat; iflat++)
	{
			temp=que->front(); que->pop(); i=temp.x; j=temp.y; que->push(temp);

			elev2->addToData(i,j,s->getData(i,j,tempShort));
	}
	elev2->share();

		
	long localStillFlat = 0;
	long totalStillFlat = 0;
	////elevDEM->getdxdyc(j,tempdxc,tempdyc);
	//float DXX[3] = {0,tempdxc,tempdyc};//tardemlib.cpp ln 1291
	//float DD = sqrt(tempdxc*tempdxc+tempdyc*tempdyc);//tardemlib.cpp ln 1293
	if(rank==0)
	{
		fprintf(stderr,"\nSetting directions\n");  
		fflush(stderr);
	}
	for(iflat=0; iflat < nflat; iflat++)
	{



	temp=que->front(); que->pop(); i=temp.x; j=temp.y; //  Do not push que on this last one - so que is empty at end
				//  The logic here was to replace SETFLOW2 from D8 with SET2 so that it computes a DINF flow 
				//  direction based on the artificial elevations

	elevDEM->getdxdyc(j,tempdxc,tempdyc);
	float DXX[3] = {0,tempdxc,tempdyc};//tardemlib.cpp ln 1291
	float DD = sqrt(tempdxc*tempdxc+tempdyc*tempdyc);//tardemlib.cpp ln 1293

			SET2(j,i,DXX,DD,elevDEM,elev2,flowDir,dn);	//use new elevations to calculate flowDir.	
			if(!flowDir->isNodata(i,j)&& flowDir->getData(i,j,tempFloat)< 0.) //this is still a flat
			{
				que->push(temp);
				localStillFlat++;
			}
	}

	MPI_Allreduce(&localStillFlat, &totalStillFlat, 1, MPI_LONG, MPI_SUM, MCW);
	if(totalStillFlat >0)  //  We will have to iterate again so overwrite original elevation with the modified ones and hope for the best
	{
		for(j=0; j<ny; j++)
			for(i=0; i<nx; i++){
				elevDEM->setData(i,j,(float)elev2->getData(i,j,tempShort));//set/add change jjn friday
			}
	}
	delete elev2;  //  to avoid memory leaks
	delete dn;
	delete s;
	return totalStillFlat;
}
