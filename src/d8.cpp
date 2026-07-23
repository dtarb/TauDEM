/*  TauDEM D8FlowDir function to compute flow direction based on d8 flow model.
     
  David G Tarboton, Dan Watson, Jeremy Neff
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
//#include "linearpart.h"   Part of CommonLib
#include "createpart.h"
#include "commonLib.h"
#include "d8.h"
//#include "tiffIO.h"    Part of Commonlib
#include "Node.h"

using namespace std;

double **fact;

//Checks if cells cross
int dontCross( int k, int i, int j, tdpartition *flowDir) {
	long n1, n2, c1, c2, ans=0;
	long in1,jn1,in2,jn2;
	short tempShort;
	switch(k){
		case 2:
			n1=1; c1=4; n2=3; c2=8;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempShort)) == c1 || (flowDir->getData(in2,jn2,tempShort)) == c2)
				ans=1;
                	break;
 		case 4:
			n1=3; c1=6; n2=5; c2=2;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempShort)) == c1 || (flowDir->getData(in2,jn2,tempShort)) == c2)
                   		ans=1;
                	break;
          	case 6:
                	n1=7; c1=4; n2=5; c2=8;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempShort) == c1 ) )
                   		ans=1;
			if (flowDir->getData(in2,jn2,tempShort) == c2 )
                   		ans=1;
                	break;
          	case 8:
			n1=1; c1=6; n2=7; c2=2;
			in1=i+d1[n1];
			jn1=j+d2[n1];
			in2=i+d1[n2];
			jn2=j+d2[n2];
			if( (flowDir->getData(in1,jn1,tempShort)) == c1 || (flowDir->getData(in2,jn2,tempShort)) == c2)
                        	ans=1;
                	break;
		default: break;
	}
	return(ans);
}

//Set positive flowdirections of elevDEM
void setFlow(int i, int j, tdpartition *flowDir, tdpartition *elevDEM, tdpartition *area, int useflowfile) {

	float slope,smax=0;
	long in,jn;
	short k,dirnb;
	short tempShort;
	float tempFloat;
	int aneigh = -1;
	int amax=0;
		
	for(k=1; k<=8 && !flowDir->isNodata(i,j); k+=2) {
		in=i+d1[k];
		jn=j+d2[k];

		slope = fact[j][k] * ( elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat) );
		if( useflowfile == 1) {	
			aneigh = area->getData(in,jn,tempShort);
		}
		if( aneigh > amax && slope >= 0 ) {
			amax = aneigh;
			dirnb = flowDir->getData(in,jn,tempShort);
			if( dirnb > 0 && abs( dirnb -k ) != 4 ) {
				flowDir->setData( i,j, k);
			}
		}
		if( slope > smax ) {
			smax=slope;
			dirnb=flowDir->getData(in,jn,tempShort);
			if( dirnb >0 && abs(dirnb-k) == 4)
				flowDir->setToNodata(i,j);
			else flowDir->setData(i,j,k);
		}
	}
	for(k=2;k<=8 && !flowDir->isNodata(i,j) ; k+=2) {
		in=i+d1[k];
		jn=j+d2[k];

		slope = fact[j][k] * ( elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat) );

		if( slope > smax  && dontCross(k,i,j,flowDir)==0 ) {
			smax = slope;
			dirnb = flowDir->getData(in,jn,tempShort);
			if( dirnb >0 && abs(dirnb-k) == 4)
				flowDir->setToNodata(i,j);
			else flowDir->setData(i,j,k);
		}
	}
}

//Calculate the slope information of flowDir to slope
void calcSlope( tdpartition *flowDir, tdpartition *elevDEM, tdpartition *slope) {
	float elevDiff, tempFloat;
	int in,jn;
	short tempShort;

	int nx = elevDEM->getnx();
	int ny = elevDEM->getny();

	for( int j = 0; j < ny; j++) {
		for( int i=0; i < nx; i++ ) {
			//If i,j is on the border or flowDir has no data, set slope(i,j) to slopeNoData 
			if ( flowDir->isNodata(i,j) || !flowDir->hasAccess(i-1,j) || !flowDir->hasAccess(i+1,j) || 
					!flowDir->hasAccess(i,j-1) || !flowDir->hasAccess(i,j+1) )  {
				slope->setData(i,j,-1.0f);
			}
			else {
				flowDir->getData(i,j,tempShort);
				in = i + d1[tempShort];	
				jn = j + d2[tempShort];	
				elevDiff = elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat);
				slope->setData(i,j,float(elevDiff*fact[j][tempShort]));
			}
		}
	}
}


//Open files, Initialize grid memory, makes function calls to set flowDir, slope, and resolvflats, writes files
int setdird8( char* demfile, char* pointfile, char *slopefile, char *flowfile, int useflowfile) {

	MPI_Init(NULL,NULL);{

	//Only needed to output time
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)
	{
		printf("D8FlowDir version %s\n",TDVERSION);
		fflush(stdout);
	}

	double begint = MPI_Wtime();

	//Read DEM from file
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
	short flowDirNodata = -32768;
	flowDir = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, flowDirNodata);

//	flowDir = new linearpart<short>;

//	flowDir->init(totalX, totalY, dx, dy, MPI_SHORT, short(-32768));

	//If using flowfile is enabled, read it in
	tdpartition *imposedflow, *area;
	area = new linearpart<int32_t>;
	area->init(totalX, totalY, dxA, dyA, MPI_LONG, int32_t(-1));

	if( useflowfile == 1) {
		tiffIO flow(flowfile,SHORT_TYPE);
		imposedflow = CreateNewPartition(flow.getDatatype(), flow.getTotalX(), flow.getTotalY(), flow.getdxA(), flow.getdyA(), flow.getNodata());

		if(!dem.compareTiff(flow)) {
			printf("Error using imposed flow file.\n");	
			return 1;
		}
		


		int i,j;
		for( j = 0; j < elevDEM->getny(); j++) {
			for( i=0; i < elevDEM->getnx(); i++ ) {
				short data;
				imposedflow->getData(i,j,data);
				if (imposedflow->isNodata(i,j) || !imposedflow->hasAccess(i-1,j) || !imposedflow->hasAccess(i+1,j) ||
				    !imposedflow->hasAccess(i,j-1) || !imposedflow->hasAccess(i,j+1)){
					//Do nothing
				}
				else if((data>0) && data <=8 ) 
						flowDir->setData(i,j,data);
			}
		}
		delete imposedflow;
		//TODO - why is this here?
		//darea( &flowDir, &area, NULL, NULL, 0, 1, NULL, 0, 0 );
	}

	long numFlat, totalNumFlat, lastNumFlat;

	double computeSlopet;
	{
		//Creates empty partition to store new slopes
		tdpartition *slope;
		float slopeNodata = -1.0f;
		slope = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, slopeNodata);
	
		numFlat = setPosDir(elevDEM, flowDir, area, useflowfile);
		calcSlope( flowDir, elevDEM, slope);

		//Stop timer
		computeSlopet = MPI_Wtime();

		tiffIO slopeIO(slopefile, FLOAT_TYPE, slopeNodata, dem);
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

	tiffIO pointIO(pointfile, SHORT_TYPE, flowDirNodata, dem);
	pointIO.write(xstart, ystart, ny, nx, flowDir->getGridPointer());
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
	}MPI_Finalize();
	return 0;
}

// Sets only flowDir only where there is a positive slope
// Returns number of cells which are flat
long setPosDir(tdpartition *elevDEM, tdpartition *flowDir, tdpartition *area, int useflowfile) {
	double dxA = elevDEM->getdxA();
	double dyA = elevDEM->getdyA();
	long nx = elevDEM->getnx();
	long ny = elevDEM->getny();
	short tempShort;
	int i,j,k,in,jn, con;
	long numFlat = 0;
    double tempdxc,tempdyc;
	//Set direction factors
    fact = new double*[ny]; // initialize 2d array by Nazmus 2/16
    for(int m = 0; m<ny;m++)
    fact[m] = new double [9];
    for(int m=0;m<ny;m++){
	  for( k=1; k<= 8; k++ ){
		  elevDEM->getdxdyc(m,tempdxc,tempdyc);
		  fact[m][k] = (double) (1./sqrt(d1[k]*d1[k]*tempdxc*tempdxc + d2[k]*d2[k]*tempdyc*tempdyc));
	  }
    }

	tempShort = 0;
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
					tempShort=(short)0;
					flowDir->setData(i,j,tempShort);
					setFlow( i,j, flowDir, elevDEM, area, useflowfile);
					if( flowDir->getData(i,j,tempShort)==0)
						numFlat++;
				}
			}	
		}
	}
	return numFlat;
}

//  Function to set flow direction based on incremented artificial elevations
void setFlow2(int i,int j, tdpartition *flowDir, tdpartition *elevDEM, tdpartition *elev2, tdpartition *dn) 
{
/*  This function sets directions based upon secondary elevations for
  assignment of flow directions across flats according to Garbrecht and Martz
  scheme.  There are two possibilities: 
	A.  The neighbor is outside the flat set
	B.  The neighbor is in the flat set.
	In the case of A the input elevations are used and if a draining neighbor is found it is selected.  
	Case B requires slope to be positive.  Remaining flats are removed by iterating this process*/

	float slope,smax,ed;
	long in,jn;
	short k;
	smax=0.;
	short tempShort;
	float tempFloat;
	short order[8]={1,3,5,7,2,4,6,8};
	for(int ii=0;ii<8;ii++)  //k=1; k<=8; k=k+1)
	{
		k=order[ii];
		jn=j+d2[k];  //y
		in=i+d1[k];  //x
		dn->getData(in,jn,tempShort);
		if(tempShort > 0)  // In flat
		{
			slope = fact[j][k]*(elev2->getData(i,j,tempShort)-elev2->getData(in,jn,tempShort));
			if(slope > smax)
			{
				flowDir->setData(i,j,k);
				smax=slope;
			}
		}
		else   // neighbor is not in flat
		{
			ed=elevDEM->getData(i,j,tempFloat)-elevDEM->getData(in,jn,tempFloat);
			if(ed >= 0)
			{
				flowDir->setData(i,j,k);
				break;  // Found a way out - this is outlet
			}
		}
	}
}

//************************************************************************

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
	bool doNothing, done;
	long numFlat;
	short tempShort;
	int32_t tempLong;
	float tempFloat;
	long numInc, numIncOld, numIncTotal;

	//create and initialize temporary storage for Garbrecht and Martz
	tdpartition *elev2, *dn, *s;
	elev2 = CreateNewPartition(SHORT_TYPE, totalx, totaly, dxA, dyA, (int16_t)1);
	   //  The assumption here is that resolving a flat does not increment a cell value 
	   //  more than fits in a short
	dn = CreateNewPartition(SHORT_TYPE, totalx, totaly, dxA, dyA, (int16_t)0);

	node temp;
	//  First time through add flat grid cells indicated by flowdir = 0 on to queue
	//  The queue is retained for later passes so this only needs be done at beginning
	if(first)
	{
		first=false;
		for(j=0; j<ny; j++){
			for(i=0; i<nx; i++){
				if(flowDir->getData(i,j,tempShort)==0)
				{
					temp.x=i; temp.y=j; que->push(temp);
				}
			}
		}
	}

	//  Grid cells in interior of flat are incremented implicitly
	long nimplicit=0;
	long nflat=que->size(), iflat;
	for(iflat=0; iflat < nflat; iflat++)
	{
		temp=que->front(); que->pop(); i=temp.x; j=temp.y;
		short nborHasFlowDir = 0;
		if ( i == 0 || i == nx-1 || j == 0 || j == ny-1 ) {
			nborHasFlowDir = 1;  // do edges explicitly
		}
		// check if any neighbor has flow direction set
		for(k=1; k<=8 && nborHasFlowDir == 0; k++){
			jn = j + d2[k];
			in = i + d1[k];
			nborHasFlowDir=flowDir->getData(in,jn,tempShort);
		}
		if ( nborHasFlowDir == 0 ) {
			// set to 0 to indicate implicit incrementing
			// default of 1 will be explicitly updated
			elev2->setData(i,j,nborHasFlowDir);
			nimplicit++;
		} else {
			que->push(temp);
		}
	}
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

	// Avoid repeated calling of dontCross() function
	// Use bit per direction (1<<k) plus ones bit indicating data present
	{ vector<short> dontCrossCache(nx*ny, 0);

	while(numIncTotal != numIncOld){  // Continue to loop as long as grid cells are being incremented
		numInc = nimplicit;
		numIncOld = numIncTotal;
		nflat=que->size();
		for(iflat=0; iflat < nflat; iflat++)
		{
			temp=que->front(); que->pop(); i=temp.x; j=temp.y;

				int dccval = dontCrossCache[nx*j+i];
				if ( ! dccval ) { // cell not cached
					dccval = 1;
					for(k=1; k<=8; k++) {
						if ( dontCross(k,i,j, flowDir) ) {
							dccval |= (1<<k);
						}
					}
					dontCrossCache[nx*j+i] = (short) dccval;
				}
				doNothing=false;
				for(k=1; k<=8 && ! doNothing; k++){
					if((dccval&(1<<k))==0){ // dontCross(k,...) == 0
						jn = j + d2[k];
						in = i + d1[k];
						elevDiff = elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat);
						tempShort=flowDir->getData(in,jn,tempShort); 
						if(elevDiff >= 0 && tempShort > 0 && tempShort < 9) //adjacent cell drains and is equal or lower in elevation so this is a low boundary
								doNothing = true;
						else if(elevDiff == 0) //if neighbor is in flat
							if(elev2->getData(in,jn,tempShort) > 0 && elev2->getData(in,jn,tempShort)<st) //neighbor is not being incremented
								doNothing = true;
					}
				}
				if(!doNothing){  // explicitly increment and re-queue
					elev2->addToData(i,j,short(1));
					numInc++;
					que->push(temp);
				} else {  // add any neighbors that were not on the queue
					for(k=1; k<=8; k++){
						jn = j + d2[k];
						if ( jn < 0 || jn >= ny ) continue;
						in = i + d1[k];
						if ( in < 0 || in >= nx ) continue;
						if ( elev2->getData(in,jn,tempShort) == 0 ) {
							// switch to explicit incrementing
							elev2->setData(in,jn,short(st+1));
							temp.x=in; temp.y=jn;
							que->push(temp);
							nimplicit--;
						}
					}
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
	} // while loop
	} // scope of dontCrossCache

	if(numIncTotal > 0)  // Not all grid cells were resolved - pits remain
		// Remaining grid cells are unresolvable pits
	{
		//There are pits remaining - set direction to no data
		while ( que->size() ) {
			temp=que->front(); que->pop(); i=temp.x; j=temp.y;
			flowDir->setToNodata(i,j);  // mark pit
			// add any neighbors that were not on the queue
			for(k=1; k<=8; k++){
				jn = j + d2[k];
				if ( jn < 0 || jn >= ny ) continue;
				in = i + d1[k];
				if ( in < 0 || in >= nx ) continue;
				if ( elev2->getData(in,jn,tempShort) == 0 ) {
					elev2->setData(in,jn,short(st));
					temp.x=in; temp.y=jn;
					que->push(temp);
					nimplicit--;
				}
			}
		}
		flowDir->share();
		
		//
		//long total = 0;
		//MPI_Allreduce(&allDone, &total, 1, MPI_LONG, MPI_SUM, MCW);
		//if(total != 0)
		//	done = false;
		//if(numIncOld == 0)
		//	done = true;
	}
	//  DGT moved from above - write directly into elev2
	s = CreateNewPartition(SHORT_TYPE, totalx, totaly, dxA, dyA, (int16_t)0);  //  Use 0 as no data to avoid need to initialize

	//incrise - drain away from higher ground
	done = false;
	numIncOld = 0;
	if(rank==0)
	{
		fprintf(stderr,"\nDraining flats away from higher adjacent terrain\n");  
		fflush(stderr);
	}

	vector<node> flats;  // cells that will need final update

	st = 1;
	numInc = 0;
	for(j=0; j<ny; j++){
		for(i=0; i<nx; i++){
			if(flowDir->getData(i,j,tempShort)==0)
			{
				temp.x=i; temp.y=j; flats.push_back(temp);
				bool setdn = false;
				for(k=1; k<=8 && ! setdn; k++){
					jn = j + d2[k];
					in = i + d1[k];
					if(elevDEM->getData(i,j,tempFloat) - elevDEM->getData(in,jn,tempFloat)<0)	//adjacent cell is higher
						setdn = true;
				}
				if ( setdn ) {
					s->setData(i,j,short(-st));
					dn->setData(i,j,short(1));
					numInc++;
					for(k=1; k<=8; k++){
						jn = j + d2[k];
						if(jn < 0 || jn >= ny) continue;
						in = i + d1[k];
						if(in < 0 || in >= nx) continue;
						if(flowDir->getData(in,jn,tempShort)!=0) continue;
						if(dn->getData(in,jn,tempShort)!=0) continue;
						dn->setData(in,jn,short(2));
						temp.x=in; temp.y=jn;
						que->push(temp);
					}
				}
			}
		}
	}
	s->share();

	while(!done){
		st++;
		// always check the borders
		for(j=0; j<ny; j+=ny-1){
			for(i=0; i<nx; i++){
				if(flowDir->getData(i,j,tempShort)!=0) continue;
				if(dn->getData(i,j,tempShort)!=0) continue;
				dn->setData(i,j,short(2));
				temp.x=i; temp.y=j;
				que->push(temp);
			}
		}
		long nque = que->size();
		for(iflat=0; iflat < nque; iflat++)
		{
				temp=que->front(); que->pop(); i=temp.x; j=temp.y;
				if ( s->getData(i,j,tempShort)<0 ) continue;
				dn->setData(i,j,short(0));
				bool setdn = false;
				for(k=1; k<=8 && ! setdn; k++){
					jn = j + d2[k];
					in = i + d1[k];
					if(s->getData(in,jn,tempShort)<0) //adjacent cell has been marked already
						setdn = true;
				}
				if ( setdn ) que->push(temp);
		}
		nque = que->size();
		for(iflat=0; iflat < nque; iflat++)
		{
				temp=que->front(); que->pop(); i=temp.x; j=temp.y;
				s->setData(i,j,short(-st));
				dn->setData(i,j,short(1));
				numInc++;
				for(k=1; k<=8; k++){
					jn = j + d2[k];
					if(jn < 0 || jn >= ny) continue;
					in = i + d1[k];
					if(in < 0 || in >= nx) continue;
					if(flowDir->getData(in,jn,tempShort)!=0) continue;
					if(dn->getData(in,jn,tempShort)!=0) continue;
					dn->setData(in,jn,short(2));
					temp.x=in; temp.y=jn;
					que->push(temp);
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

	st++;
	nflat = flats.size();
	for(iflat=0; iflat < nflat; iflat++)
	{
			temp = flats[iflat]; i=temp.x; j=temp.y;
			if ( s->getData(i,j,tempShort) ) {
				s->setData(i,j,short(tempShort+st));
				elev2->addToData(i,j,short(tempShort+st));
			}
	}
	elev2->share();

	long localStillFlat = 0;
	long totalStillFlat = 0;

	if(rank==0)
	{
		fprintf(stderr,"\nSetting directions\n");  
		fflush(stderr);
	}
	for(iflat=0; iflat < nflat; iflat++)
	{
				temp = flats[iflat]; i=temp.x; j=temp.y;

				setFlow2( i, j, flowDir, elevDEM, elev2, dn) ;
				if(flowDir->getData(i,j,tempShort) == 0)
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
