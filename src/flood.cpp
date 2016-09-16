/*  PitRemove flood function to compute pit filled DEM using the flooding approach.
     
  David Tarboton, Dan Watson, Chase Wallis
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
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include <stack>
using namespace std;

int flood( char* demfile, char* felfile, char *sfdrfile, int usesfdr, bool verbose, 
           bool is_4Point,bool use_mask,char *maskfile)  // these three added by arb, 5/31/11
{

	MPI_Init(NULL,NULL);{

	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)
	{
		printf("PitRemove version %s\n",TDVERSION);
		fflush(stdout);
	}

	double begint = MPI_Wtime();

  //logic for 4-way flow or 8-way flow
  int step=1;
  if (is_4Point)
    step=2;

	//Create tiff object, read and store header info
	tiffIO dem(demfile,FLOAT_TYPE);
  tiffIO *depmask;
  if (use_mask) {
	  depmask=new tiffIO(maskfile,SHORT_TYPE); // arb added, 5/31/11
    if (!dem.compareTiff(*depmask))
    {
      if (rank==0) {
        printf("Error: depression mask and input DEM are not similar. Files must have the same number of rows/columns.\n");
        fflush(stdout);
      }
      return 1;
    }
  }
	long totalX = dem.getTotalX();
	long totalY = dem.getTotalY();
	double dxA = dem.getdxA();
	double dyA = dem.getdyA();

	//Create partition and read data
	tdpartition* elevDEM=NULL;
  tdpartition* maskPartition=NULL;
	elevDEM = CreateNewPartition(dem.getDatatype(), totalX, totalY, dxA, dyA, dem.getNodata());
  if (use_mask)
    maskPartition=CreateNewPartition(depmask->getDatatype(), totalX, totalY, dxA, dyA, depmask->getNodata());

	int nx = elevDEM->getnx();
	int ny = elevDEM->getny();
	int xstart, ystart;
	elevDEM->localToGlobal(0, 0, xstart, ystart);

	double headert = MPI_Wtime();

	if(rank==0)
	{
		float timeestimate=(1.5e-6*totalX*totalY/pow((double) size,0.5))/60+1;  // Time estimate in minutes
		fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
		fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
		fflush(stderr);
	}

	if(verbose)  // debug writes
	{
		printf("Header read\n");
		printf("Process: %d, totalX: %ld, totalY: %ld\n",rank,totalX,totalY);
		printf("Process: %d, nx: %d, ny: %d\n",rank,nx,ny);
		printf("Process: %d, xstart: %d, ystart: %d\n",rank,xstart,ystart);
    if (use_mask)
    {
      printf("Process: %d, Using depression mask data...\n",rank);
    }
		fflush(stdout);
	}

	dem.read(xstart, ystart, ny, nx, elevDEM->getGridPointer());	
  if (use_mask)
	  depmask->read(xstart, ystart, ny, nx, maskPartition->getGridPointer());	

/////////////////////////////////
// begin timer
	double readt = MPI_Wtime();

	//Create empty partition to store new information
	tdpartition *planchon;
	float felNodata = -3.0e38;
	planchon = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, felNodata);

	long i,j;
	short k;
	long in,jn;
	bool con=false, finished;
	int scan=0;
	float tempFloat=0, neighborFloat=0;

	if(verbose)
	{
		printf("Data read\n");
		long nxm=nx/2;
		long nym=ny/2;
		elevDEM->getData(nxm,nym,tempFloat);
		printf("Midpoint of partition: %d, nxm: %ld, nym: %ld, value: %f\n",rank,nxm,nym,tempFloat);
		fflush(stdout);
	}
	
	//These will be used to scan in different directions
	const int X0[8] = {  0, nx-1,   0, nx-1, nx-1,    0,    0, nx-1};
	const int Y0[8] = {  0, ny-1,   0, ny-1,    0, ny-1, ny-1,    0};
	const int dX[8] = {  1,   -1,   0,    0,   -1,    1,    0,    0};
	const int dY[8] = {  0,    0,   1,   -1,    0,    0,    -1,    1};  //  DGT changed 8/14/09
	//  was  const int dY[8] = { 0, 0,1,-1, 0,0,1,1};
	const int fX[8] = {-nx,   nx,   1,   -1,   nx,  -nx,    1,   -1};
	const int fY[8] = {  1,   -1, -ny,   ny,    1,   -1,   ny,  -ny};

	//If using sfdr 
	/*if(usesfdr == 1) {
		tdpartition *sfdr, *neighbor;
		sfdr = CreateNewPartition(FLOAT_TYPE

		linearpart sfdr,neighbor;
		sfdr.init( sfdrfile);
		if( totalCols != sfdr.getTotalCols() || totalRows != sfdr.getTotalRows() || xllcenter != sfdr.getXll() || yllcenter != sfdr.getYll() || 
					dx != sfdr.getdx() || dy != sfdr.getdy() ) {

				printf("Sfdr file does not match given DEM\n");
				MPI_Abort(MCW,-1);
		}
		else printf("Using %s as specified streams\n",sfdrfile);
		short sNoData =-1;
		neighbor.init( SHORT_TYPE, elevDEM,&sNoData);
		
		short tempShort0=0;
		short tempShort1=1;
		queue <node> que;
		node temp;

		//Count contributing neighbors of sfdr and add peaks to queue
		for( j=0;j<ny;++j) {
			for( i=0;i<nx;++i) {
				if( sfdr.getShort(i,j) >=1 || sfdr.getShort(i,j) <=8 ) {
					neighbor.setGrid(i,j,tempShort0);
					for( k=1;k<=8;k++) {
						in = i+d1[k];
						jn = j+d2[k];
	
						if(sfdr.hasAccess(in,jn) && sfdr.getShort(in,jn) >= 1 && sfdr.getShort(in,jn) <=8 &&
								( sfdr.getShort(in,jn) - k == 4 || sfdr.getShort(in,jn) -k == -4) ) {
							neighbor.addToGrid(i,j,tempShort1);
						}
					}
					if( neighbor.getShort(i,j) == 0) {
						temp.x = i;
						temp.y = j;
						que.push(temp);
					}
				}
			}
		}

		//Pop off queue and carve if necessary
		short tempShort = -1;	
		while( !que.empty() ) {
			temp = que.front();
			que.pop();
			i=temp.x;
			j=temp.y;
			in = i+d1[ sfdr.getShort(i,j) ];
			jn = j+d2[ sfdr.getShort(i,j) ];
			if( elevDEM.getFloat(in,jn) - fNoData > 1e-5) {
				if( elevDEM.getFloat(in,jn) > (tempFloat=elevDEM.getFloat(i,j))) {
					elevDEM.setGrid(in,jn, tempFloat);
				}
				neighbor.addToGrid(in,jn,tempShort);
				if( neighbor.getShort(in,jn) == 0 ) {
					temp.x = in;
					temp.y = jn;
					que.push(temp);
				}
			}
		}

		sfdr.remove();
		neighbor.remove();
	}*/

/////////////////////////////////////////
  short tmpshort=0;
	//Fill the border arrays across the partitions
	elevDEM->share();   
  if (use_mask)
    maskPartition->share();
	//Initialize the new grid
	for(j=0; j<ny; j++){
		for(i=0; i<nx; i++){
			//If elevDEM has no data, planchon has no data.
			if(elevDEM->isNodata(i,j)) 
				planchon->setToNodata(i,j);

      else if (use_mask && maskPartition->getData(i,j,tmpshort)==1) // logic for setting the elevation when using a depression mask
        planchon->setData(i,j,elevDEM->getData(i,j,tempFloat));

			//If i,j is on the border, set planchon(i,j) to elevDEM(i,j)
			else if (!elevDEM->hasAccess(i-1,j) || !elevDEM->hasAccess(i+1,j) ||
					 !elevDEM->hasAccess(i,j-1) || !elevDEM->hasAccess(i,j+1))
				planchon->setData(i,j, elevDEM->getData(i,j,tempFloat));
			//Check if cell is "contaminated" (neighbors have no data)
			//  set planchon to elevDEM(i,j) if it is, else set to FLT_MAX
			else{ 
				con = false;
				for(k=1; k<=8 && !con; k+=step) {
					in = i+d1[k];
					jn = j+d2[k];
					if(elevDEM->isNodata(in,jn)) con=true;
				}
				if(con)
					planchon->setData(i,j,elevDEM->getData(i,j,tempFloat));
				else if(!elevDEM->isNodata(i,j))
					planchon->setData(i,j,FLT_MAX);
			}
		}
	}
	//Done initializing grid
	//Make sure everyone has updated subgirds
	planchon->share();
	if(verbose)
	{
		printf("Planchon grid initialized rank: %d\n",rank);
		fflush(stdout);
	}
	
//////////////////////////////////////		
	//First pass - put unresolved grid cells on a stack
//	finished = false;
//	while( finished == false ) {
	finished = true;
	i = X0[scan];
	j = Y0[scan];

	stack<long> s1, s2;
	long pass=0;
	long stacksize=100;  // Stack size above which verbose message is written
	while(planchon->isInPartition(i,j)) { 
		//If statement - only enter if there is data there OR
		// there is "water" on planchon
		if(!planchon->isNodata(i,j) && planchon->getData(i,j,tempFloat) > elevDEM->getData(i,j,neighborFloat)){
			//Checks each direction...
			neighborFloat = FLT_MAX;
			for(k=1; k<=8; k+=step){
				in = i+d1[k];
				jn = j+d2[k];
				if(planchon->hasAccess(in,jn) && planchon->getData(in,jn,tempFloat) < neighborFloat)
						//Get neighbor data and store as planchon for self
						planchon->getData(in,jn, neighborFloat);
			}
//				if( neighborFloat < FLT_MAX ) {  //DGT This check is redundant - because scans start from the side
				//Set the grid to either elevDEM, all "water" can be taken off"
			if(elevDEM->getData(i,j, tempFloat) >= neighborFloat ){
				planchon->setData(i,j, elevDEM->getData(i,j,tempFloat));
				finished = false;  
			}
			// or some water can be taken off
			else 
			{				
				s1.push(i);
				s1.push(j);
				if(verbose)
				{
					if(s1.size()>stacksize)
					{
						long psz=s1.size();
						printf("Rank: %d, Stack size: %ld\n",rank,psz);
						fflush(stdout);
						stacksize=stacksize+100000;
					}			
				}
				//  DGT.  The second part of the condition below is redundant
				if(planchon->getData(i,j,tempFloat) > neighborFloat /* && elevDEM->getData(i,j,tempFloat) < neighborFloat */){
					planchon->setData(i,j,neighborFloat);
					finished = false;
				}
			} 
//				}
//				else   //DGT code used to verify that above if was redundant
//					printf("I am here - should never be\n");
		}
		//Now we need to set i,j to the next one to evaluate
		i += dX[scan];
		j += dY[scan];
		if(!planchon->isInPartition(i,j) ) {
			i+= fX[scan];
			j+= fY[scan];
		}
	}
	planchon->share();
	//  progress and debug prints
	if(verbose)
	{
		pass=pass+1;
		stacksize=100;  // reset stacksize
		long remaining=s1.size();
		printf("Process: %d, Pass: %ld, Remaining: %ld\n",rank,pass,remaining);
		fflush(stdout);
	}
	//  This step is to check if all processes are finished in which case while loop is skipped for all processes
	finished = planchon->ringTerm(finished);
// Now repeat the scanning but pulling off stack and putting on new stack
	while(!finished){
		finished=true;
		while(!s1.empty()){
			j=s1.top();
			s1.pop();
			i=s1.top();
			s1.pop();
			neighborFloat = FLT_MAX;
			for(k=1; k<=8; k+=step){
				in = i+d1[k];
				jn = j+d2[k];
				if(planchon->hasAccess(in,jn) && planchon->getData(in,jn,tempFloat) < neighborFloat)
						//Get neighbor data and store as planchon for self
						planchon->getData(in,jn, neighborFloat);
			}
	//				if( neighborFloat < FLT_MAX ) {  //DGT This check is redundant - because scans start from the side
				//Set the grid to either elevDEM, all "water" can be taken off"
			if(elevDEM->getData(i,j, tempFloat) >= neighborFloat ){
				planchon->setData(i,j, elevDEM->getData(i,j,tempFloat));
				finished = false;
			}
			// or some water can be taken off
			else 
			{   // Keep grid cell on scan list because still above original elevation
				s2.push(i);
				s2.push(j);
				if(verbose)
				{
					if(s2.size()>stacksize)
					{
						long psz=s2.size();
						printf("Rank: %d, Stack 2 size: %ld\n",rank,psz);
						fflush(stdout);
						stacksize=stacksize+100000;
					}			
				}
				//  condition below is commented out for efficiency.  It has already passed this test from if above
				if(planchon->getData(i,j,tempFloat) > neighborFloat /*&& elevDEM->getData(i,j,tempFloat) < neighborFloat */)
				{
					planchon->setData(i,j,neighborFloat);
					finished = false;
				}
			} 
		}
		planchon->share();

		//  progress and debug prints
		if(verbose)
		{
			pass=pass+1;
			long remaining=s2.size();
			stacksize=100;  //  reset stack size
			printf("Process: %d, Pass: %ld, Remaining: %ld\n",rank,pass,remaining);
			fflush(stdout);
		}

        //  Repeat but with stacks interchanged
		finished=true;
		while(!s2.empty()){
			j=s2.top();
			s2.pop();
			i=s2.top();
			s2.pop();
			neighborFloat = FLT_MAX;
			for(k=1; k<=8; k+=step){
				in = i+d1[k];
				jn = j+d2[k];
				if(planchon->hasAccess(in,jn) && planchon->getData(in,jn,tempFloat) < neighborFloat)
						//Get neighbor data and store as planchon for self
						planchon->getData(in,jn, neighborFloat);
			}
	//				if( neighborFloat < FLT_MAX ) {  //DGT This check is redundant - because scans start from the side
				//Set the grid to either elevDEM, all "water" can be taken off"
			if(elevDEM->getData(i,j, tempFloat) >= neighborFloat ){
				planchon->setData(i,j, elevDEM->getData(i,j,tempFloat));
				finished = false;
			}
			// or some water can be taken off
			else 
			{   // Keep grid cell on scan list because still above original elevation
				s1.push(i);
				s1.push(j);
				if(verbose)
				{
					if(s1.size()>stacksize)
					{
						long psz=s1.size();
						printf("Rank: %d, Stack 1 size: %ld\n",rank,psz);
						fflush(stdout);
						stacksize=stacksize+100000;
					}			
				}
				//  condition below is commented out for efficiency.  It has already passed this test from if above
				if(planchon->getData(i,j,tempFloat) > neighborFloat /*&& elevDEM->getData(i,j,tempFloat) < neighborFloat */)
				{
					planchon->setData(i,j,neighborFloat);
					finished = false;
				}
			} 
		}
		finished = planchon->ringTerm(finished);

		//scan++;
		//if(scan == 8) {
		//	scan=0;
		//	//Terminate if nothing had been done
		//	// only check every 8 scans to reduce message passing
		//	finished = planchon->ringTerm(finished);
		//	////////////////////////////
		//}
		//else finished = false;
		planchon->share();

		//  progress and debug prints
		if(verbose)
		{
			pass=pass+1;
			long remaining=s1.size();
			stacksize=100;  // reset stack size
			printf("Process: %d, Pass: %ld, Remaining: %ld\n",rank,pass,remaining);
			fflush(stdout);
		}
	}

	//Stop timer
	double computet = MPI_Wtime();
	if(verbose)
	{
		printf("About to write\n");
		long nxm=nx/2;
		long nym=ny/2;
		planchon->getData(nxm,nym,tempFloat);
		printf("Result midpoint of partition: %d, nxm: %ld, nym: %ld, value: %f\n",rank,nxm,nym,tempFloat);
		fflush(stdout);
	}

	//Create and write TIFF file
	tiffIO fel(felfile, FLOAT_TYPE, &felNodata, dem);
	fel.write(xstart, ystart, ny, nx, planchon->getGridPointer());

	if(verbose)printf("Partition: %d, written\n",rank);
	double headerRead, dataRead, compute, write, total,temp;
	double writet = MPI_Wtime();
	headerRead = headert-begint;
	dataRead = readt-headert;
	compute = computet-readt;
	write = writet-computet;
	total = writet - begint;

	MPI_Allreduce (&headerRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	headerRead = temp/size;
	MPI_Allreduce (&dataRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	dataRead = temp/size;
	MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	compute = temp/size;
	MPI_Allreduce (&write, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	write = temp/size;
	MPI_Allreduce (&total, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
	total = temp/size;

	if( rank == 0)
		printf("Processes: %d\nHeader read time: %f\nData read time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
		  size,headerRead , dataRead, compute, write,total);


	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();

	return 0;
}
