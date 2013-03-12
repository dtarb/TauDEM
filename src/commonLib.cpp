/*  Taudem common library functions 

  David Tarboton, Dan Watson
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


#include <stdio.h>
#include <string.h>
#include "commonLib.h"
#include <math.h>



//==================================
/*  Nameadd(..)  Utility for adding suffixes to file names prior to
   "." extension   */
int nameadd(char *full,char *arg,char *suff)
{
        char *ext, *extsuff;
        long nmain;
    ext=strrchr(arg,'.');
	extsuff=strrchr(suff,'.');
    if(ext == NULL)
        {
                nmain=(long)strlen(arg);
                sprintf(full,"%s%s",arg,suff);
        }
        else
        {
                nmain=(long)(strlen(arg)-strlen(ext));
                strcpy(full,"");
                strncat(full,arg,nmain);
                strcat(full,suff);
				if(extsuff == NULL)strcat(full,ext);  //  Only append original extension if suffix does not have an extension already
        }
        return(nmain);
}

//TODO - does this function go here, or in areadinf?
double prop( float a, int k) {
	double p=0.;
	if(k<=0)k=k+8;  // DGT to guard against remainder calculations that give k=0
	if(k == 1 && a > PI)a=(float)(a-2.0*PI);
	if(a > aref[k-1] && a < aref[k+1]){
		if( a > aref[k])
			p=(aref[k+1]-a)/(aref[k+1]-aref[k]);
		else
			p=(a-aref[k-1])/(aref[k]-aref[k-1]);
	}
	if( p < 1e-5) return -1.;
	else return(p);
}
void initNeighborDinfup(tdpartition* neighbor,tdpartition* flowData,queue<node> *que,
					  int nx,int ny,int useOutlets, int *outletsX,int *outletsY,long numOutlets)
{
	//  Function to initialize the neighbor partition either whole partition or just upstream of outlets
	//  and place locations with no neighbors that drain to them on the que
	int i,j,k,in,jn;
	short tempShort;
	float tempFloat,angle,p;
	node temp;
	if(useOutlets != 1) {
		//Count the contributing neighbors and put on queue
		for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++) {
				//Initialize neighbor count to no data, but then 0 if flow direction is defined
				neighbor->setToNodata(i,j);
				if(!flowData->isNodata(i,j)) {
					//Set contributing neighbors to 0 
					neighbor->setData(i,j,(short)0);
					//Count number of contributing neighbors
					for(k=1; k<=8; k++){
						in = i+d1[k];
						jn = j+d2[k];
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
							flowData->getData(in, jn, angle);
							p = prop(angle, (k+4)%8);  //  if neighbor drains to me
							if(p>0.)
								neighbor->addToData(i,j,(short)1);
						}
					}
					if(neighbor->getData(i, j, tempShort) == 0){
						//Push nodes with no contributing neighbors on queue
						temp.x = i;
						temp.y = j;
						que->push(temp);
					}
				}
			}
		} 
	}
	// If Outlets are specified
	else {
	//Put outlets on queue to be evalutated
		queue<node> toBeEvaled;
		for( i=0; i<numOutlets; i++) {
			flowData->globalToLocal(outletsX[i], outletsY[i], temp.x, temp.y);
			if(flowData->isInPartition(temp.x, temp.y))
				toBeEvaled.push(temp);
		}

		//TODO - this is 100% linear partition dependent.
		//Create a packet for message passing
		int *bufferAbove = new int[nx];
		int *bufferBelow = new int[nx];
		int countA, countB;

		//TODO - consider copying this statement into other memory allocations
		if( bufferAbove == NULL || bufferBelow == NULL ) {
			printf("Error allocating memory\n");
			MPI_Abort(MCW,5);
		}
		
		int rank,size;
		MPI_Comm_rank(MCW,&rank);
		MPI_Comm_size(MCW,&size);

		bool finished = false;
		while(!finished) {
			countA = 0;
			countB = 0;
			while(!toBeEvaled.empty()) {
				temp = toBeEvaled.front();
				toBeEvaled.pop();
				i = temp.x;
				j = temp.y;
				// Only evaluate if cell hasn't been evaled yet
				if(neighbor->isNodata(i,j)){
					//Set contributing neighbors to 0
					neighbor->setData(i,j,(short)0);
					//Count number of contributing neighbors
					for(k=1; k<=8; k++){
						in = i+d1[k];
						jn = j+d2[k];
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){ 
							flowData->getData(in, jn, angle);
							p = prop(angle, (k+4)%8);
							if(p>0.) {
								if( jn == -1 ) {
									bufferAbove[countA] = in;
									countA +=1;
								}
								else if( jn == ny ) {
									bufferBelow[countB] = in;
									countB += 1;
								}
								else {
									temp.x = in;
									temp.y = jn;
									toBeEvaled.push(temp);
								}
								neighbor->addToData(i,j,(short)1);
							}
						}
					}					
					if(neighbor->getData(i,j, tempShort) == 0){
						//Push nodes with no contributing neighbors on queue
						temp.x = i;
						temp.y = j;
						que->push(temp);
					}
				}	
			}
			finished = true;
			
			neighbor->transferPack( &countA, bufferAbove, &countB, bufferBelow );

			if( countA > 0 || countB > 0 )
				finished = false;

			if( rank < size-1 ) {
				for( k=0; k<countA; k++ ) {
					temp.x = bufferAbove[k];
					temp.y = ny-1;
					toBeEvaled.push(temp);
				}
			}
			if( rank > 0 ) {
				for( k=0; k<countB; k++ ) {
					temp.x = bufferBelow[k];
					temp.y = 0;
					toBeEvaled.push(temp);
				}
			}
			finished = neighbor->ringTerm( finished );
		}
		delete bufferAbove;
		delete bufferBelow;
	}
}

void initNeighborD8up(tdpartition* neighbor,tdpartition* flowData,queue<node> *que,
					  int nx,int ny,int useOutlets, int *outletsX,int *outletsY,long numOutlets)
{
	//  Function to initialize the neighbor partition either whole partition or just upstream of outlets
	//  and place locations with no neighbors that drain to them on the que
	int i,j,k,in,jn;
	short tempShort;
	//float tempFloat,angle,p;
	node temp;
	if(useOutlets != 1) {
		//Count the contributing neighbors and put on queue
		for(j=0; j<ny; j++) {
			for(i=0; i<nx; i++) {
				//Initialize neighbor count to no data, but then 0 if flow direction is defined
				neighbor->setToNodata(i,j);
				if(!flowData->isNodata(i,j)) {
					//Set contributing neighbors to 0 
					neighbor->setData(i,j,(short)0);
					//Count number of contributing neighbors
					for(k=1; k<=8; k++){
						in = i+d1[k];
						jn = j+d2[k];
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
							flowData->getData(in, jn, tempShort);
							if(tempShort-k == 4 || tempShort-k == -4)
								neighbor->addToData(i,j,(short)1);
						}
					}
					if(neighbor->getData(i, j, tempShort) == 0){
						//Push nodes with no contributing neighbors on queue
						temp.x = i;
						temp.y = j;
						que->push(temp);
					}
				}
			}
		} 
	}
	// If Outlets are specified
	else {
	//Put outlets on queue to be evalutated
		queue<node> toBeEvaled;
		for( i=0; i<numOutlets; i++) {
			flowData->globalToLocal(outletsX[i], outletsY[i], temp.x, temp.y);
			if(flowData->isInPartition(temp.x, temp.y))
				toBeEvaled.push(temp);
		}

		//TODO - this is 100% linear partition dependent.
		//Create a packet for message passing
		int *bufferAbove = new int[nx];
		int *bufferBelow = new int[nx];
		int countA, countB;

		//TODO - consider copying this statement into other memory allocations
		if( bufferAbove == NULL || bufferBelow == NULL ) {
			printf("Error allocating memory\n");
			MPI_Abort(MCW,5);
		}
		
		int rank,size;
		MPI_Comm_rank(MCW,&rank);
		MPI_Comm_size(MCW,&size);

		bool finished = false;
		while(!finished) {
			countA = 0;
			countB = 0;
			while(!toBeEvaled.empty()) {
				temp = toBeEvaled.front();
				toBeEvaled.pop();
				i = temp.x;
				j = temp.y;
				// Only evaluate if cell hasn't been evaled yet
				if(neighbor->isNodata(i,j)){
					//Set contributing neighbors to 0
					neighbor->setData(i,j,(short)0);
					//Count number of contributing neighbors
					for(k=1; k<=8; k++){
						in = i+d1[k];
						jn = j+d2[k];
						if(flowData->hasAccess(in,jn) && !flowData->isNodata(in,jn)){
							flowData->getData(in, jn, tempShort);
							//  Does neighbor drain to me
							if(tempShort-k == 4 || tempShort-k == -4){
								if( jn == -1 ) {
									bufferAbove[countA] = in;
									countA +=1;
								}
								else if( jn == ny ) {
									bufferBelow[countB] = in;
									countB += 1;
								}
								else {
									temp.x = in;
									temp.y = jn;
									toBeEvaled.push(temp);
								}
								neighbor->addToData(i,j,(short)1);
							}
						}
					}					
					if(neighbor->getData(i,j, tempShort) == 0){
						//Push nodes with no contributing neighbors on queue
						temp.x = i;
						temp.y = j;
						que->push(temp);
					}
				}	
			}
			finished = true;
			
			neighbor->transferPack( &countA, bufferAbove, &countB, bufferBelow );

			if( countA > 0 || countB > 0 )
				finished = false;

			if( rank < size-1 ) {
				for( k=0; k<countA; k++ ) {
					temp.x = bufferAbove[k];
					temp.y = ny-1;
					toBeEvaled.push(temp);
				}
			}
			if( rank > 0 ) {
				for( k=0; k<countB; k++ ) {
					temp.x = bufferBelow[k];
					temp.y = 0;
					toBeEvaled.push(temp);
				}
			}
			finished = neighbor->ringTerm( finished );
		}
		delete bufferAbove;
		delete bufferBelow;
	}
}

//returns true iff cell at [nrow][ncol] points to cell at [row][col]
bool pointsToMe(long col, long row, long ncol, long nrow, tdpartition *dirData){
	short d;
	if(!dirData->hasAccess(ncol,nrow) || dirData->isNodata(ncol,nrow)){return false;}
	d=dirData->getData(ncol,nrow,d);
	if(nrow+d2[d]==row && ncol+d1[d]==col){
		return true;
	}
	return false;
}
