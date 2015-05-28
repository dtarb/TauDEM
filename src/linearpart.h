/*  Taudem parallel linear partition classes

  David Tarboton, Kim Schreuders, Dan Watson
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

#include "mpi.h"
#include "partition.h"

#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <exception>
#ifndef LINEARPART_H
#define LINEARPART_H
using namespace std;

template <class datatype>
class linearpart : public tdpartition {
	protected:
		// Member data inherited from partition
		//long totalx, totaly;
		//long nx, ny;
		//double dx, dy;
		int rank, size;
		MPI_Datatype MPI_type;
		datatype noData;
		datatype *gridData;
		datatype *topBorder;
		datatype *bottomBorder;

	public:
		linearpart():tdpartition(){}
		~linearpart();

		void init(long totalx, long totaly, double dx_in, double dy_in, MPI_Datatype MPIt, datatype nd);
		bool isInPartition(int x, int y);
		bool hasAccess(int x, int y);

		void share();
		void passBorders();
		void addBorders();
		void clearBorders();
		int ringTerm(int isFinished);
		
		bool globalToLocal(int globalX, int globalY, int &localX, int &localY);
		void localToGlobal(int localX, int localY, int &globalX, int &globalY);

		int getGridXY( int x,int y, int *i, int *j);
		void transferPack( int *, int *, int *, int*);

		// Member functions inherited from partition
		//int getnx() {return nx;}
		//int getny() {return ny;} 
		//int gettotalx(){return totalx;}
		//int gettotaly(){return totaly;}
		void* getGridPointer(){return gridData;}
		bool isNodata(long x, long y);
		void setToNodata(long x, long y);
		datatype getData(long x, long y, datatype &val);
		void setData(long x, long y, datatype val);
		void savedxdyc(tiffIO &obj);
		void getdxdyc(long iny, double &val_dxc,double &val_dyc);
		void addToData(long x, long y, datatype val);
				
		//void areaD(queue<node> *que);

		//inherited from partition
		//int *before1;
		//int *before2;
		//int *after1;
		//int *after2;

};


//Destructor.  Just frees up memory.
template <class datatype>
linearpart<datatype>::~linearpart(){
	delete [] gridData;
	delete [] bottomBorder;
	delete [] topBorder;
}

//Init routine.  Takes the total number of rows and columns in the ENTIRE grid to be partitioned,
//dx and dy for the grid, MPI datatype (should match the template declaration), and noData value.
template <class datatype>
void linearpart<datatype>::init(long totalx, long totaly, double dx_in, double dy_in, MPI_Datatype MPIt, datatype nd){
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);

	//Store all the initialization variables in their appropriate places
	this->totalx = totalx;
	this->totaly = totaly;
	nx = totalx;
	ny = totaly / size;
	if(rank == size-1)  ny += (totaly % size); //Add extra rows to the last process
	dxA = dx_in;
	dyA = dy_in;
	MPI_type = MPIt;
	noData = nd;

	//Allocate memory for data and fill with noData value.  Catch exceptions
	uint64_t prod;  //   use long 64 bit number to hold the product to allocate
	try
	{
		prod=nx*ny;
		gridData = new datatype[prod];
		topBorder = new datatype[nx];
		bottomBorder = new datatype[nx];
	}
	catch(bad_alloc&)
	{
	//  DGT added clause below to try trap for insufficient memory in the computer.
		fprintf(stdout,"Memory allocation error during partition initialization in process %d.\n",rank);
		fprintf(stdout,"NCols: %ld, NRows: %ld, NCells: %ld\n",nx,ny,prod);
		fflush(stdout);
		MPI_Abort(MCW,-999);
	}

	for(uint64_t j=0; j<nx; j++){
		for(uint64_t i=0; i<ny; i++) gridData[i*nx+j] = noData;
		topBorder[j] = noData;
		bottomBorder[j] = noData;
	}

	//TODO: find out what these are for
	after1=after2=before1=before2=NULL;
}


//Returns true if (x,y) is in partition
template <class datatype>
bool linearpart<datatype>::isInPartition(int x, int y) {
	if(x>=0 && x<nx && y>=0 && y<ny) return true;
	else return false;
}

//Returns true if (x,y) is in or on borders of partition
template <class datatype>
bool linearpart<datatype>::hasAccess( int x, int y) {
	//isInPartition takes care of the case where (x,y) is inside the grid
	if(x>=0 && x<nx && y>=0 && y<ny) return true;   // DGT reducing function nesting for efficiency
//	if(isInPartition(x,y)) return true;	

	//Now we only need to worry about borders.
	//x must be bounded by 0 and nx, y may be -1 to ny
	else if(x>=0 && x<nx ) {
		if(rank !=0 && y==-1) return true;
		if(rank !=size-1 && y==ny) return true;
	}
	return false;
}

//Shares border information between adjacent processes.  Border information is stored
//in the "topBorder" and "bottomBorder" arrays of each process.
template <class datatype>
void linearpart<datatype>::share() {
	MPI_Status status;
	if(size<=1) return; //if there is only one process, we're all done sharing


	datatype *ptr;
	int place;
	datatype *buf;
	int bsize=nx*sizeof(datatype)+MPI_BSEND_OVERHEAD; 
	buf = new datatype[bsize];

	if(rank<size-1){
		MPI_Buffer_attach(buf,bsize);
		MPI_Bsend(gridData+((ny-1)*nx), nx, MPI_type, rank+1, 0, MCW);
		MPI_Buffer_detach(&ptr,&place);
	}
	if(rank >0)	MPI_Recv(topBorder, nx, MPI_type, rank-1, 0, MCW, &status);
	if(rank>0){
		MPI_Buffer_attach(buf,bsize);
		MPI_Bsend(gridData, nx, MPI_type, rank-1, 0, MCW);
		MPI_Buffer_detach(&ptr,&place);
	}
	if(rank<size-1) MPI_Recv(bottomBorder, nx, MPI_type, rank+1, 0, MCW, &status);

	delete [] buf;   // added by dww -- why not elsewhere?

/*
	if(rank == 0){ //Top partition in grid - only send and receive the bottom
		
//		cout << rank << " 1" << endl;
		MPI_Bsend(gridData+((ny-1)*nx), nx, MPI_type, rank+1, 0, MCW);
		MPI_Recv(bottomBorder, nx, MPI_type, rank+1, 0, MCW, &status);
//		cout << rank << " end" << endl;

	}else if(rank == size-1){ //Bottom partition - only send and receive top
		MPI_Bsend(gridData, nx, MPI_type, rank-1, 0, MCW);
		MPI_Recv(topBorder, nx, MPI_type, rank-1, 0, MCW, &status);
//		cout << rank << " end" << endl;

	}else{ //Send and receive top and bottom
		MPI_Bsend(gridData+((ny-1)*nx), nx, MPI_type, rank+1, 0, MCW);
		MPI_Recv(bottomBorder, nx, MPI_type, rank+1, 0, MCW, &status);

		MPI_Buffer_detach(&ptr,&place);
		MPI_Buffer_attach(buf,bsize);

		MPI_Bsend(gridData, nx, MPI_type, rank-1, 0, MCW);
		MPI_Recv(topBorder, nx, MPI_type, rank-1, 0, MCW, &status);
//		cout << rank << " end" << endl;

	}
	MPI_Buffer_detach(&ptr,&place);
*/
}

//Swaps border information between adjacent processes.  In this way, no data is
//overwritten.  If this function is called a second time, the original state is
//restored.
template <class datatype>
void linearpart<datatype>::passBorders() {
	MPI_Status status;
	if(size<=1) return; //if there is only one process, we're all done sharing

	datatype *ptr;
	int place;
	datatype *buf;
	datatype *tempBorder;
	int bsize=nx*sizeof(datatype)+MPI_BSEND_OVERHEAD; 
	buf = new datatype[bsize];
	tempBorder = new datatype[nx];

	if(rank<size-1){
		MPI_Buffer_attach(buf,bsize);
		MPI_Bsend(bottomBorder, nx, MPI_type, rank+1, 0, MCW);
		MPI_Buffer_detach(&ptr,&place);
	}
	if(rank >0)	MPI_Recv(tempBorder, nx, MPI_type, rank-1, 0, MCW, &status);
	if(rank>0){
		MPI_Buffer_attach(buf,bsize);
		MPI_Bsend(topBorder, nx, MPI_type, rank-1, 0, MCW);
		MPI_Buffer_detach(&ptr,&place);
	}
	if(rank<size-1) MPI_Recv(bottomBorder, nx, MPI_type, rank+1, 0, MCW, &status);
	memmove(topBorder,tempBorder,nx*sizeof(datatype)); 

	delete [] buf;   // added by dww -- why not elsewhere?
	delete [] tempBorder;


/*


	if(rank == 0){ //Top partition in grid - only send and receive the bottom
		MPI_Bsend(bottomBorder, nx, MPI_type, rank+1, 0, MCW);
		MPI_Recv(bottomBorder, nx, MPI_type, rank+1, 0, MCW, &status);

	}else if(rank == size-1){ //Bottom partition - only send and receive top
		MPI_Bsend(topBorder, nx, MPI_type, rank-1, 0, MCW);
		MPI_Recv(topBorder, nx, MPI_type, rank-1, 0, MCW, &status);

	}else{ //Send and receive top and bottom
		MPI_Bsend(bottomBorder, nx, MPI_type, rank+1, 0, MCW);
		MPI_Recv(bottomBorder, nx, MPI_type, rank+1, 0, MCW, &status);

		MPI_Buffer_detach(&ptr,&place);
		MPI_Buffer_attach(buf,bsize);

		MPI_Bsend(topBorder, nx, MPI_type, rank-1, 0, MCW);
		MPI_Recv(topBorder, nx, MPI_type, rank-1, 0, MCW, &status);
	}
	MPI_Buffer_detach(&ptr,&place);
*/


}

//Swaps border information between adjacent processes,
//then adds the values from received borders to the local copies.
template <class datatype>
void linearpart<datatype>::addBorders(){
	//Start by calling passBorders to get information.
	passBorders();

	uint64_t i;
	for(i=0; i<nx; i++){
		//Add the values passed in from other process
		if(isNodata(i,-1) || isNodata(i,0)) setData(i, 0, noData);
		else addToData(i, 0, topBorder[i]);

		if(isNodata(i, ny) || isNodata(i, ny-1)) setData(i, ny-1, noData);
		else addToData(i, ny-1, bottomBorder[i]);

	}
}

//Clears borders (sets them to zero).
template <class datatype>
void linearpart<datatype>::clearBorders(){
	uint64_t i;
	for(i=0; i<nx; i++){
		topBorder[i] = 0;
		bottomBorder[i] = 0;
	}
}


//TODO: revisit the way this function works.  I don't like it.
//      It really shouldn't even be here.
template <class datatype>
int linearpart<datatype>::ringTerm(int isFinished) {
	int ringBool = isFinished;
	//The parameter isFinished tells us if the que is empty.
	MPI_Status status;
	//Ring termination check
//	cout << rank << " ring Term begin" << endl;
	if(size>1) {
		//First processor will send a token.  It will tell the next proc if it is done or not
		if( rank==0 ) {
		//	cout << rank << " sending..." << endl;
			MPI_Send( &ringBool, 1, MPI_INT, rank+1 ,1,MCW);
			//cout << rank << " reciving..." << endl;
			MPI_Recv( &ringBool, 1,MPI_INT, size-1,1,MCW,&status);
			//cout << rank << " finished ..." << endl;
		}
		//The rest of the processors recv, if they are not finished, they change the token to NOTFINISHED
		else {
			//cout << rank << " reciving ..." << endl;
 			MPI_Recv( &ringBool, 1,MPI_INT, rank-1,1,MCW,&status);
			//cout << rank << " sending ..." << endl;
			if( isFinished == NOTFINISHED ) ringBool = NOTFINISHED;
			MPI_Send( &ringBool, 1,MPI_INT, (rank+1)%size, 1, MCW);
			//cout << rank << " finished ..." << endl;
		}

		//If the token came back as FINISHED, all ranks may quit, but first they need to know they can quit.
		// First proc sends ringBool=FINISHED or NOTFINISHED, each processor sends it on until the last recieves it
		if( rank==0 ) {
			//cout << " distribute results" << endl;
			MPI_Send( &ringBool, 1, MPI_INT, rank+1 ,1,MCW);
		}
		else {
			//cout << " recive results" << endl;
			MPI_Recv( &ringBool, 1,MPI_INT, rank-1,1,MCW,&status);
			if( rank!=size-1)
				MPI_Send( &ringBool, 1,MPI_INT, (rank+1)%size, 1, MCW);
		}
	}
	//cout << rank <<" ring term end" << endl;
	return ringBool; 
}

//Converts global coordinates (for the whole grid) to local coordinates (for this
//partition).  Function returns TRUE only if the coordinates are contained
//in this partition.
template <class datatype>
bool linearpart<datatype>::globalToLocal(int globalX, int globalY, int &localX, int &localY){
	localX = globalX;
	localY = globalY - rank * ny;
	//  For the last process ny is greater than the size of the other partitions so rank*ny does not get the row right.
	//  totaly%size was added to ny for the last partition, so the size of partitions above is actually 
	//  ny - totaly%size  (the remainder from dividing totaly by size).
	if(rank == size-1) 
		localY = globalY - rank * (ny - totaly%size); 
	return isInPartition(localX, localY);
} 

//Converts local coordinates (for this partition) to the whole grid.
template <class datatype>
void linearpart<datatype>::localToGlobal(int localX, int localY, int &globalX, int &globalY){
	globalX = localX;
	globalY = rank * ny + localY;
	if(rank == size-1) 
		globalY = rank * (ny - totaly%size) + localY;
}

//TODO: Figure out what this function is actually for.
//I don't think it is called more than once in Taudem.
template <class datatype>
int linearpart<datatype>::getGridXY( int x, int y, int *i, int *j) {
	*i = *j = -1;
	int numRowsPerProc = ny/size;
	int starty = numRowsPerProc*rank;
	if( rank == size-1)
		numRowsPerProc += (totaly%size);
	int  endy = starty + numRowsPerProc;
	if( x >= 0 && x < nx && y >= starty && y < endy) {
		*i = x;
		*j = y - starty;
		return 1;
	}
	return 0;
}

//TODO: Revisit this function to see how necessary it is.
//It only gets called a couple of times throughout Taudem.
template <class datatype>
void linearpart<datatype>::transferPack( int *countA, int *bufferAbove, int *countB, int *bufferBelow) {
	MPI_Status status;
	if(size==1) return;

	int place;
	datatype *abuf, *bbuf;
	int absize;
	int bbsize;
	absize=*countA*sizeof(int)+MPI_BSEND_OVERHEAD;  
	bbsize=*countB*sizeof(int)+MPI_BSEND_OVERHEAD;  
		
	abuf = new datatype[absize];
	bbuf = new datatype[bbsize];
	//MPI_Buffer_attach(buf,bbsize);

	if( rank >0 ) {
		MPI_Buffer_attach(abuf,absize);
		MPI_Bsend( bufferAbove, *countA, MPI_INT, rank-1, 3, MCW );
		MPI_Buffer_detach(&abuf,&place);
	}
	if( rank < size-1) {
		MPI_Probe( rank+1,3,MCW, &status);  // Blocking function this only returns when there is a message to receive
		MPI_Get_count( &status, MPI_INT, countA);  //  To get count from the status variable
		MPI_Recv( bufferAbove, *countA,MPI_INT, rank+1,3,MCW,&status);  // Receives message sent in first if from another process
		MPI_Buffer_attach(bbuf,bbsize);
		MPI_Bsend( bufferBelow, *countB, MPI_INT, rank+1,3,MCW);
		MPI_Buffer_detach(&bbuf,&place);
	}
	if( rank > 0 ) {
		MPI_Probe( rank-1,3,MCW, &status);
		MPI_Get_count( &status, MPI_INT, countB);
		MPI_Recv( bufferBelow, *countB,MPI_INT, rank-1,3,MCW,&status);
	}

	delete abuf;
	delete bbuf;
}

//Returns true if grid element (x,y) is equal to noData.
template <class datatype>
bool linearpart<datatype>::isNodata(long inx, long iny){
	int64_t x, y;//int64 because it oculd be -1.
	x = inx;
	y = iny;
//DGT to avoid nested calls and type inconsistency
	if(x>=0 && x<nx && y>=0 && y<ny)return (abs((float)(gridData[x+y*nx]-noData))<MINEPS);  
//	if(isInPartition(x,y)) return (abs(gridData[x+y*nx]-noData)<MINEPS);
	else if(x>=0 && x<nx){
		if(y==-1) return (abs((float)(topBorder[x]-noData))<MINEPS);
		else if(y==ny) return (abs((float)(bottomBorder[x]-noData))<MINEPS);
	}
	return true;
}

//Sets the element in the grid to noData.
template <class datatype>
void linearpart<datatype>::setToNodata(long inx, long iny){
	int64_t x, y;
	x = inx;
	y = iny;
	if(x>=0 && x<nx && y>=0 && y<ny) gridData[x+y*nx] = noData;
//	if(isInPartition(x,y)) gridData[x+y*nx] = noData;
	else if(x>=0 && x<nx){
		if(y==-1) topBorder[x] = noData;
		else if(y==ny) bottomBorder[x] = noData;
	}
}

//Returns the element in the grid with coordinate (x,y).
template <class datatype>
datatype linearpart<datatype>::getData(long inx, long iny, datatype &val) {
	int64_t x, y;
	x = inx;
	y = iny;
//	if(isInPartition(x,y)) val = gridData[x+y*nx];
	if(x>=0 && x<nx && y>=0 && y<ny) val = gridData[x+y*nx];
	else if(x>=0 && x<nx){
		if(y==-1) val = topBorder[x];
		else if(y==ny) val = bottomBorder[x];
	}
	return val;
}



template <class datatype>
void linearpart<datatype>::savedxdyc( tiffIO &obj) {
    dxc=new double[ny];
	dyc=new double[ny];
    for (int i=0;i<ny;i++){
		int globalY = rank * ny +i;
	if(rank == size-1) globalY = rank * (ny - totaly%size) + i;
	    dxc[i]=obj.getdxc(globalY);
		dyc[i]=obj.getdyc(globalY);

	         }
    }
	

template <class datatype>
void linearpart<datatype>::getdxdyc(long iny, double &val_dxc,double &val_dyc){
	 int64_t y;y = iny;
	 if(y>=0 && y<ny){ val_dxc=dxc[y];val_dyc=dyc[y];}
}



//Sets the element in the grid to the specified value.
template <class datatype>
void linearpart<datatype>::setData(long inx, long iny, datatype val){
	int64_t x, y;
	x = inx;
	y = iny;
//	if(isInPartition(x,y)) gridData[x+y*nx] = val;
	if(x>=0 && x<nx && y>=0 && y<ny) gridData[x+y*nx] = val;
	else if(x>=0 && x<nx){
		if(y==-1) topBorder[x] = val;
		else if(y==ny) bottomBorder[x] = val;
	}
}

//Increments the element in the grid by the specified value.
template <class datatype>
void linearpart<datatype>::addToData(long inx, long iny, datatype val){
	int64_t x, y;
	x = inx;
	y = iny;
//	if(isInPartition(x,y)) gridData[x+y*nx] += val;
	if(x>=0 && x<nx && y>=0 && y<ny) gridData[x+y*nx] += val;
	else if(x>=0 && x<nx){
		if(y==-1) topBorder[x] += val;
		else if(y==ny) bottomBorder[x] += val;
	}
}
#endif
