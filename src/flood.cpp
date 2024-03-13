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

struct ChangedElem {
	ChangedElem(int _i, int _j, float _filled)
		: i(_i), j(_j), filled(_filled) { ; }
	friend bool operator>(const ChangedElem& lhs, const ChangedElem &rhs) {
		return lhs.filled > rhs.filled;
	}
	int i,j;
	float filled;
};

// priority queue optimized by additional stack for pushing elements at top
class ChangedQueue {
public:
	// combine top() and pop() to avoid redundant branching
	void extract_top(long &i, long &j, float &filled) {
		if ( stack.empty() || ( ! queue.empty() && stack.back() > queue.top() ) ) {
			const ChangedElem& elem = queue.top();
			i = elem.i;
			j = elem.j;
			filled = elem.filled;
			queue.pop();
		} else {
			const ChangedElem& elem = stack.back();
			i = elem.i;
			j = elem.j;
			filled = elem.filled;
			stack.pop_back();
		}
	}
	bool empty() const {
		return queue.empty() && stack.empty();
	}
	long size() const {
		return queue.size() + stack.size();
	}
	// only call emplace_top if filled <= last extracted value
	void emplace_top(long i, long j, float filled) {
		stack.emplace_back(i, j, filled);
	}
	void emplace(long i, long j, float filled) {
		queue.emplace(i, j, filled);
	}
private:
	priority_queue<ChangedElem, std::vector<ChangedElem>, std::greater<ChangedElem>> queue;
	std::vector<ChangedElem> stack;
};

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

	// 0 if updates possible (has water that might drain)
	// 1 if no updates possible (bare ground or DEM is nodata)
	linearpart<char> isDrained;
	isDrained.init(totalX, totalY, dxA, dyA, MPI_CHAR, char(4));

	// optimized priority queue - yields point with lowest filled elevation
	ChangedQueue changed;

	long i,j;
	short k;
	long in,jn;
	bool con=false, finished;
	int scan=0;
	float tempFloat=0, neighborFloat=0;
	char tempChar=0;

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
  for (j = 0; j < ny; j++) {
	  for (i = 0; i < nx; i++) {
		  //If elevDEM has no data, planchon has no data.
		  if (elevDEM->isNodata(i, j))
			  planchon->setToNodata(i, j);

		  else if (use_mask && maskPartition->getData(i, j, tmpshort) == 1) // logic for setting the elevation when using a depression mask
			  planchon->setData(i, j, elevDEM->getData(i, j, tempFloat));

		  //If i,j is on the border, set planchon(i,j) to elevDEM(i,j)
		  else if (!elevDEM->hasAccess(i - 1, j) || !elevDEM->hasAccess(i + 1, j) ||
			  !elevDEM->hasAccess(i, j - 1) || !elevDEM->hasAccess(i, j + 1))
			  planchon->setData(i, j, elevDEM->getData(i, j, tempFloat));
		  //Check if cell is "contaminated" (neighbors have no data)
		  //  set planchon to elevDEM(i,j) if it is, else set to FLT_MAX
		  else {
			  con = false;
			  for (k = 1; k <= 8 && !con; k += step) {
				  in = i + d1[k];
				  jn = j + d2[k];
				  if (elevDEM->isNodata(in, jn)) con = true;
			  }
			  if (con)
				  planchon->setData(i, j, elevDEM->getData(i, j, tempFloat));
			  else if (!elevDEM->isNodata(i, j))
				  planchon->setData(i, j, FLT_MAX);
		  }

		  if ( planchon->getData(i, j, tempFloat) == FLT_MAX ) {
			  isDrained.setData(i, j, char(0));
		  } else {
			  isDrained.setData(i, j, char(1));
			  if ( ! planchon->isNodata(i, j) ) {
				  // must have been set to DEM
				  changed.emplace(i, j, tempFloat);
			  }
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
	long pass=0;

	//Enqueue initial points from borders and make copy for comparison
	std::vector<float> upperBorderCopy(nx), lowerBorderCopy(nx);
	if ( planchon->hasAccess(0,-1) ) {
		for (i=0; i<nx; i++) {
			const float filled = planchon->getData(i,-1,tempFloat);
			if ( filled != FLT_MAX && ! planchon->isNodata(i, -1) ) {
				changed.emplace(i, -1, filled);
			}
			upperBorderCopy[i] = filled;
		}
	}
	if ( planchon->hasAccess(0,ny) ) {
		for (i=0; i<nx; i++) {
			const float filled = planchon->getData(i,ny,tempFloat);
			if ( filled != FLT_MAX && ! planchon->isNodata(i, ny) ) {
				changed.emplace(i, ny, filled);
			}
			lowerBorderCopy[i] = filled;
		}
	}

	//  progress and debug prints
	if(verbose)
	{
		pass=pass+1;
		long remaining = changed.size();
		printf("Process: %d, Pass: %ld, Remaining: %ld\n",rank,pass,remaining);
		fflush(stdout);
	}

	// optimize for data locality in pits by pushing same row onto stack last
	const int d1opt[9] = { 0,-1, 1, 1,-1, 0, 0,-1, 1};
	const int d2opt[9] = { 0,-1,-1, 1, 1,-1, 1, 0, 0};

	finished = false;
	long limitwork = (long)nx * 8;
	while(!finished){
		long localwork = 0;
		while ( localwork < limitwork ) {
			if ( changed.empty() ) break;
			float filled;
			changed.extract_top(i,j,filled);
			if ( planchon->getData(i,j,tempFloat) != filled ) continue;
			++localwork;
			for(k=(is_4Point?5:1); k<=8; ++k){
				in = i+d1opt[k];
				jn = j+d2opt[k];
				if ( ! planchon->isInPartition(in,jn) ) continue;
				if ( isDrained.getData(in,jn,tempChar) ) continue;
				if ( planchon->getData(in,jn,tempFloat) <= filled ) continue;
				if ( elevDEM->getData(in,jn,tempFloat) >= filled ) {
					// remove all water, set to elevDEM
					planchon->setData(in,jn,tempFloat);
					isDrained.setData(in,jn,char(1));
					changed.emplace(in,jn,tempFloat);
				} else {
					// remove water to match neighbor
					planchon->setData(in,jn,filled);
					changed.emplace_top(in,jn,filled);
				}
			}
		}

		planchon->share();
		if ( planchon->hasAccess(0,-1) ) {
			for (i=0; i<nx; i++) {
				const float filled = planchon->getData(i,-1,tempFloat);
				if ( upperBorderCopy[i] != filled ) {
					changed.emplace(i,-1,filled);
					upperBorderCopy[i] = filled;
				}
			}
		}
		if ( planchon->hasAccess(0,ny) ) {
			for (i=0; i<nx; i++) {
				const float filled = planchon->getData(i,ny,tempFloat);
				if ( lowerBorderCopy[i] != filled ) {
					changed.emplace(i,ny,filled);
					lowerBorderCopy[i] = filled;
				}
			}
		}
		finished = changed.empty();
		finished = planchon->ringTerm(finished);
		//  progress and debug prints
		if(verbose)
		{
			pass=pass+1;
			long remaining = changed.size();
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
	tiffIO fel(felfile, FLOAT_TYPE, felNodata, dem);
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
