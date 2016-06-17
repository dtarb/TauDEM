/*  linklib functions for streamnet to compute stream networks based on d8 directions.
     
  David Tarboton, Dan Watson, Jeremy Neff
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

#ifndef LINKLIB_H
#define LINKLIB_H

#include <iostream>
#include <cmath>
#include <mpi.h>
#include <iomanip>
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "partition.h"
#include "tiffIO.h"

long LAST_ID = -1;

struct point{
	long x;
	long y;
	float elev;
	float area;
	float length;
};

struct streamlink{
	int32_t Id;
	int32_t u1;
	int32_t u2;
	long d;
	long magnitude;
	long shapeId;//from outletshapefile
	double elevU;
	double elevD;
	double length;
	short order;
    std::queue<point> coord; //  We store the coordinates of a link in a queue
	long numCoords;
	bool terminated;
	
};

struct llnode{
	streamlink *data;
	llnode *next;
};
struct LinkedLL{
	llnode *head;
	int numLinks;
};

LinkedLL linkSet;
void makeLinkSet(){
	linkSet.head = NULL;
	linkSet.numLinks = 0;
	return;
}
void setLinkInfo(long **LinkIdU1U2DMagShapeid,double **LinkElevUElevDLength,double **PointXY, float **PointElevArea, tdpartition *elev, tiffIO *elevIO);
void getNumLinksAndPoints(long &myNumLinks,long &myNumPoints);
void SendAndReciveLinks(int nx,int ny,tdpartition *idGrid, tdpartition *contribs,tdpartition* flowDir, tdpartition* src);
long getMagnitude(int32_t Id);
void appendPoint(int32_t Id, point* addPoint);
void setDownLinkId(int32_t Id, long dId);
streamlink* createLink(int32_t u1, int32_t u2, long d, point* coord); //,long numCoords);  //, float dx, float dy);
void linkSetInsert(streamlink* linkToAdd);
long GetOrderOf(long ID);
long findLinkThatEndsAt(long x, long y, tdpartition*);
bool recvLink(int src);
bool sendLink(int32_t Id, int dest);
void terminateLink(int32_t Id);
//long findLinkThatStartsAt(long x, long y);
streamlink* FindLink(int32_t Id);
streamlink* getFirstLink();
streamlink* takeOut(int32_t Id);
llnode* LSInsert(llnode *head, llnode *addNode);
//void makeLinkSet();

// non recursive version -jjn
llnode* LSInsert(llnode *head, llnode *addNode){
	addNode->next = head;
	head = addNode;
	return head;
}


//assignment link* takeOut(linkId){}  link* joinLinks(LinkId1,LinkId2){}/ending point of one link is the starting point of the other link. (same x and y or MPI abort)   movelink(Id)from one prosses to annother prosses/send and recive?
//Takes a link ID as input.  Finds the link in the linked list linkSet and removes the llnode from the linked list.  It returns a pointer to the link.  If the ID is not found it returns NULL
streamlink* takeOut(int32_t Id){
	streamlink* linkToBeRemoved;
	llnode* nodeToBeRemoved;
	if(linkSet.head == NULL){
		return NULL;//linkSet is empty
	}
	if(linkSet.head->data->Id == Id){
		//remove llnode and return streamlink* to head data	
		linkToBeRemoved = linkSet.head->data;
		nodeToBeRemoved = linkSet.head;
		linkSet.head = linkSet.head->next;
		delete nodeToBeRemoved;
		linkSet.numLinks--;
		return linkToBeRemoved;
	}else{// not the first llnode in the list
		llnode *previous, *current;
		current = linkSet.head;
		previous = NULL;
		while(current->data->Id != Id && current->next != NULL){//search the linked list
			previous = current;
			current = current->next;
		}
		if(current->data->Id == Id){//found it
			previous->next = current->next;
			nodeToBeRemoved = current;
			linkToBeRemoved = current->data;
			delete nodeToBeRemoved;
			linkSet.numLinks--;
			return linkToBeRemoved;
		}
		//We didn't find the ID
	}
	return NULL;
}
streamlink* FindLink(int32_t Id){
	streamlink* linkToBeRemoved;
	llnode* nodeToBeRemoved;
	if(linkSet.head == NULL){
		return NULL;//linkSet is empty
	}
	if(linkSet.head->data->Id == Id){
		return linkSet.head->data;
	}else{// not the first llnode in the list
		llnode *current;
		current = linkSet.head;
		while(current->data->Id != Id && current->next != NULL){//search the linked list
			current = current->next;
		}
		if(current->data->Id == Id){//found it
			return current->data;
		}
		//We didn't find the ID
	}
	return NULL;
}
//returns -1 if coord not found.  returns ID if found.
//long findLinkThatStartsAt(long x, long y){
//	long ID = -1;
//	llnode *current = linkSet.head;
//	if(linkSet.numLinks == 0)
//		return ID;
//	else
//		while(current != NULL){
//			if(current->data->coord[0].x == x && current->data->coord[0].y == y){
//				ID = current->data->Id;
//				break;//I found it.
//			}
//			current = current->next;
//		}	
//	return ID;
//}
void terminateLink(int32_t Id){
	streamlink *linkToKill;
	linkToKill = FindLink(Id);
	linkToKill = linkToKill;
	if(linkToKill == NULL){
		MPI_Abort(MCW,4);
	}
	else{
		linkToKill->terminated = true;
	}
	return;
}

//More information sending structures as used here is 
//  at :https://computing.llnl.gov/tutorials/mpi/

/*MPI_Datatype PointType, oldtypes[2];  
int          blockcounts[2]; 

MPI_Aint offsets[2], extent;
MPI_Status stat; 

offsets[0] = 0;
oldtypes[o] = MPI_LONG;
blockcounts[0]= 2;

MPI_Type_extent(MPI_LONG, &extent);
offsets[1] = 2 * extent;
oldtypes[1] = MPI_FLOAT;
blockcounts[1] = 1;

MPI_Type_struct(2,blockcounts,offsets,oldtypes,&PointType);
MPI_Type_comit(&PointType);

MPI_Send(coord,numCoords,PointType,dest,tag,MCW);
MPI_Recv(coords,numCoords,PointType,source,tag,MCW,&stat);
*/

bool sendLink(int32_t Id, int dest){
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);


	streamlink *toSend = takeOut(Id);
	if(toSend == NULL)
		return false;
	if(toSend->terminated == true){
		linkSetInsert(toSend);
		return false;
	}
	
	MPI_Request req;
	MPI_Send(&(toSend->Id)		,1,MPI_LONG		,dest,1,MCW);//send Id
	MPI_Send(&(toSend->u1)		,1,MPI_LONG		,dest,2,MCW);
	MPI_Send(&(toSend->u2)		,1,MPI_LONG		,dest,3,MCW);
	MPI_Send(&(toSend->d)		,1,MPI_LONG		,dest,4,MCW);
	MPI_Send(&(toSend->elevU)	,1,MPI_DOUBLE	,dest,5,MCW);
	MPI_Send(&(toSend->elevD)	,1,MPI_DOUBLE	,dest,6,MCW);
	MPI_Send(&(toSend->length)	,1,MPI_DOUBLE		,dest,7,MCW);
	MPI_Send(&(toSend->order)	,1,MPI_SHORT		,dest,8,MCW);
	MPI_Send(&(toSend->numCoords)	,1,MPI_LONG	,dest,9,MCW);
	MPI_Send(&(toSend->magnitude)	,1,MPI_LONG	,dest,11,MCW);
	MPI_Send(&(toSend->shapeId)	,1,MPI_LONG	,dest,12,MCW);

	//More information on This at :https://computing.llnl.gov/tutorials/mpi/
	//create mpi Data types
	MPI_Datatype PointType, oldtypes[2];  
	int          blockcounts[2]; 

	MPI_Aint offsets[2], extent;
	MPI_Status stat; 
	//set up first blocks of storage
	offsets[0] = 0;
	oldtypes[0] = MPI_LONG;
	blockcounts[0]= 2;
	//set up second block of storage
	MPI_Type_extent(MPI_LONG, &extent);
	offsets[1] = 2 * extent;
	oldtypes[1] = MPI_FLOAT;
	blockcounts[1] = 3;
	//create define it as an MPI data type and comit it.
	MPI_Type_struct(2,blockcounts,offsets,oldtypes,&PointType);
	MPI_Type_commit(&PointType);

	MPI_Status status;

	char *ptr;
	int place;
	point *buf;
	point *sendArr;
	sendArr = new point[toSend->numCoords];
	for(int i = 0; i < toSend->numCoords ;i++)
	{
		sendArr[i].x = toSend->coord.front().x;//y elec area length
		sendArr[i].y = toSend->coord.front().y;
		sendArr[i].elev = toSend->coord.front().elev;
		sendArr[i].area = toSend->coord.front().area;
		sendArr[i].length = toSend->coord.front().length;
		toSend->coord.pop();
	}	


	int bsize = toSend->numCoords*sizeof(point)*2+MPI_BSEND_OVERHEAD;  // Experimentally this seems to need to have >47 added for overhead
	buf = new point[bsize];
	
	MPI_Buffer_attach(buf,bsize);
	MPI_Bsend(sendArr,toSend->numCoords,PointType,dest,10,MCW);
	MPI_Buffer_detach(&ptr,&place);
	delete sendArr;
	delete toSend;

	return true;
}
bool recvLink(int src){
	streamlink* toRecv = new streamlink;
	MPI_Status stat;
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);

	MPI_Recv(&(toRecv->Id)		,1,MPI_LONG		,src,1,MCW,&stat);//send Id
	MPI_Recv(&(toRecv->u1)		,1,MPI_LONG		,src,2,MCW,&stat);
	MPI_Recv(&(toRecv->u2)		,1,MPI_LONG		,src,3,MCW,&stat);
	MPI_Recv(&(toRecv->d)		,1,MPI_LONG		,src,4,MCW,&stat);
	MPI_Recv(&(toRecv->elevU)	,1,MPI_DOUBLE		,src,5,MCW,&stat);
	MPI_Recv(&(toRecv->elevD)	,1,MPI_DOUBLE		,src,6,MCW,&stat);
	MPI_Recv(&(toRecv->length)	,1,MPI_DOUBLE		,src,7,MCW,&stat);
	MPI_Recv(&(toRecv->order)	,1,MPI_SHORT		,src,8,MCW,&stat);
	MPI_Recv(&(toRecv->numCoords)	,1,MPI_LONG		,src,9,MCW,&stat);
	MPI_Recv(&(toRecv->magnitude)	,1,MPI_LONG		,src,11,MCW,&stat);
	MPI_Recv(&(toRecv->shapeId)	,1,MPI_LONG		,src,12,MCW,&stat);

	//More information on This at :https://computing.llnl.gov/tutorials/mpi/
	//create mpi Data types
	MPI_Datatype PointType, oldtypes[2];  
	int          blockcounts[2]; 

	MPI_Aint offsets[2], extent;
	MPI_Status stat1; 
	//set up first blocks of storage
	offsets[0] = 0;
	oldtypes[0] = MPI_LONG;
	blockcounts[0]= 2;
	//set up second block of storage
	MPI_Type_extent(MPI_LONG, &extent);
	offsets[1] = 2 * extent;
	oldtypes[1] = MPI_FLOAT;
	blockcounts[1] = 3;
	//create define it as an MPI data type and comit it.
	MPI_Type_struct(2,blockcounts,offsets,oldtypes,&PointType);
	MPI_Type_commit(&PointType);
	int flag;
	//MPI_Request req;
	//MPI_Test(&req,&flag,&stat1);
	//if(!(stat1.MPI_SOURCE >=0 && stat1.MPI_SOURCE < size))
	//	return false;
//	toRecv->coord = new point[toRecv->numCoords];

	point *recvArr;
	recvArr = new point[toRecv->numCoords];
	MPI_Recv(recvArr,toRecv->numCoords,PointType,src,10,MCW,&stat);
	toRecv->terminated = false;
	
	point temp;
	for(int i=0;i<toRecv->numCoords > 0;i++)
	{
		temp.x = recvArr[i].x;//y elec area length
		temp.y = recvArr[i].y;//y elec area length
		temp.elev = recvArr[i].elev;//y elec area length
		temp.area = recvArr[i].area;//y elec area length
		temp.length = recvArr[i].length;//y elec area length
		toRecv->coord.push(temp);
	}	

	linkSetInsert(toRecv);
	delete recvArr;

	return true;
}
//returns -1 if coord not found.  returns ID if found.
long findLinkThatEndsAt(long x, long y, tdpartition *elev){
	long ID = -1;
	llnode* current = linkSet.head;
	if(linkSet.numLinks == 0)
		return ID;
	else
	{
		int gx,gy;
		elev->localToGlobal((int)x,(int)y,gx,gy);  
		while(current != NULL){
//			cout << "x:" << current->data->coord[current->data->numCoords - 1].x << endl;
//          cout << "x:" << current->data->coord[current->data->numCoords - 1].x << endl;
			if(current->data->coord.back().x == gx && current->data->coord.back().y == gy && !current->data->terminated){
				ID = current->data->Id;
				break;//I found it.
			}
			current = current->next;
		}
	}
	return ID;
}
//returns -1` if ID not found.  Else it rturns the order of the link.
long GetOrderOf(long ID){
	llnode* current = linkSet.head;
	if(linkSet.numLinks == 0)
		return -1;
	else
		while(current != NULL){
			if(ID == current->data->Id){
				return current->data->order;
			}
			current = current->next;
		}	
	return -1;
}

void linkSetInsert(streamlink* linkToAdd)
{
	llnode *newNode = new llnode;
	newNode->data = linkToAdd;
	newNode->next = NULL;

	linkSet.numLinks++;
	linkSet.head = LSInsert(linkSet.head, newNode); 	
	//cout << "New ID entered: " << linkToAdd->Id << endl;
	return;
}
streamlink* createLink(int32_t u1, int32_t u2, long d, point* coord) //,long numCoords) //, double dx, double dy)
{
	int size ;
	 MPI_Comm_size(MCW,&size);
	int rank ;
	 MPI_Comm_rank(MCW,&rank);

	streamlink *newLink = new streamlink;

	if(LAST_ID == -1){
		newLink->Id = rank;
	}else{
		newLink->Id =LAST_ID + size;
	}
	LAST_ID = newLink->Id;

	newLink->u1 = u1;	//maybe NULL
	newLink->u2 = u2;	//maybe NULL
	newLink->d = d;  	//maybe NULL

	newLink->coord.push(*coord);

	//newLink->coord = new point[numCoords];
	//memcpy(newLink->coord,coord,numCoords*sizeof(point));

	newLink->numCoords = 1; //numCoords;
	newLink->elevU = coord->elev;  //newLink->coord[0].elev;
	newLink->elevD = coord->elev;  //newLink->coord[numCoords-1].elev;
	newLink->order = 1;//a link starts at order 1
	newLink->terminated = false;

	newLink->magnitude = 1;
	int i=0;
	newLink->length = 0;
	//  DGT commented code below for efficiency reasons - length not used
	//float diag = sqrt(dx*dx+dy*dy);
	//for(i = 0; i < numCoords-1; i++){
	//	if(newLink->coord[i].x != newLink->coord[i+1].x && newLink->coord[i].y != newLink->coord[i+1].y)
	//		newLink->length += diag;
	//	else if(newLink->coord[i].x == newLink->coord[i+1].x )
	//		newLink->length += dy;
	//	else
	//		newLink->length += dx;
	//}
	newLink->shapeId = -1;
	//cout << rank << " Putting link into link set..."<< newLink->Id << endl;
	linkSetInsert(newLink);
	//cout << rank << " link put in linkset." << endl;
	return newLink;
}

void setDownLinkId(int32_t Id, long dId){
	streamlink* myLink = FindLink(Id);
	myLink->d = dId;
	return;
}
void appendPoint(int32_t Id, point* addPoint){
	streamlink* myLink = FindLink(Id);
	//cout << "ID = " << Id << endl;
	if(myLink == NULL)
		MPI_Abort(MCW,8);//link not found cannot proccess
	//point* points;
	myLink->numCoords = myLink->numCoords +1;//make room

	//points = new point[myLink->numCoords];
//	if(points == NULL)
//		MPI_Abort(MCW,48);//link not found cannot proccess
	//cout << "before memcopy " << Id << endl;
	long i;
	//for(i=0; i < (myLink->numCoords) -1; i++)
	//{
	//	points[i]=myLink->coord[i];
	//}
	//memcpy(points,myLink->coord,(myLink->numCoords-1)*sizeof(point));
	//cout << "after memcopy " << Id << endl;
	//points[myLink->numCoords-1] = *addPoint;
	//delete [] myLink->coord;
	//cout << "after delete " << Id << endl;
	myLink->coord.push(*addPoint);  
	myLink->elevD = addPoint->elev;
	//myLink->numCoords++;
	//myLink->length++;  // DGT this is not used 
	return;
}
void SendAndReciveLinks(int nx,int ny,tdpartition* idGrid, tdpartition* contribs, tdpartition* flowDir, tdpartition* src){
	int linksSent = 0;
	int linksRecv = 0;
	int totalSent = 0;
	int totalRecv = 0;
	int toSend = 0;
	int toRecv = 0;
	
	short tempShort;
	int32_t tempLong;

	int rank,size,sent;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	MPI_Status stat;
	flowDir->share();
	if(size == 1)
		return;
	//Evens send and odds recive
	if(rank%2 == 0){//even send
		//evens send up
		int i;
		int inext,jnext;
		if(rank!=0){
			for(i = 0; i < nx; i++){//count links to send down
				if(!contribs->isNodata(i,-1) && contribs->getData(i,-1,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
					if( i>0 && flowDir->getData((long)i-1,(long)0,tempShort) == 2 && src->getData(i-1,0,tempShort) == 1 && idGrid->getData(i-1,0,tempLong) >=0){
						toSend++;
					}else if(flowDir->getData((long)i,(long)0,tempShort) == 3 && src->getData(i,0,tempShort) == 1 && idGrid->getData(i,0,tempLong) >=0){
						toSend++;
					}else if(i < nx && flowDir->getData((long)i+1,(long)0,tempShort) == 4 && src->getData(i+1,0,tempShort) == 1 && idGrid->getData(i+1,0,tempLong) >=0){
						toSend++;
					}
				}
			}
			MPI_Send(&toSend,1,MPI_INT,rank-1,rank,MCW);//tell the reciver how many links to recive.
			for(i = 0; i < nx; i++){//count links to send up
				if(!contribs->isNodata(i,-1) && contribs->getData(i,-1,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
					if( i>0 && flowDir->getData((long)i-1,(long)0,tempShort) == 2 && src->getData(i-1,0,tempShort) == 1 && idGrid->getData(i-1,0,tempLong) >=0){
						if(sendLink(idGrid->getData(i-1,0,tempLong), rank-1)){//check to see if sent properly
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}else if(flowDir->getData((long)i,(long)0,tempShort) == 3 && src->getData(i,0,tempShort) == 1 && idGrid->getData(i,0,tempLong) >=0){
						if(sendLink(idGrid->getData(i,0,tempLong), rank-1)){//check to see if sent properly
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}else if(i < nx && flowDir->getData((long)i+1,(long)0,tempShort) == 4 && src->getData(i+1,0,tempShort) == 1 && idGrid->getData(i+1,0,tempLong) >=0){
						if(sendLink(idGrid->getData(i+1,0,tempLong), rank-1)){//check to see if sent properl
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}
				}
			}
		}
		//send down
		toSend = 0;
		if(rank!=size-1){
			for(i = 0; i < nx; i++){//count links to send down
				if(!contribs->isNodata(i,ny) && contribs->getData(i,ny,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
					if( i>0 && flowDir->getData((long)i-1,(long)ny-1,tempShort) == 8 && src->getData(i-1,ny-1,tempShort) == 1 && idGrid->getData(i-1,ny-1,tempLong) >=0){
						toSend++;
					}else if(flowDir->getData((long)i,(long)ny-1,tempShort) == 7 && src->getData(i,ny-1,tempShort) == 1 && idGrid->getData(i,ny-1,tempLong) >=0){
						toSend++;
					}else if(i < nx && flowDir->getData((long)i+1,(long)ny-1,tempShort) == 6 && src->getData(i+1,ny-1,tempShort) == 1 && idGrid->getData(i+1,ny-1,tempLong) >=0){
						toSend++;
					}
				}
			}
			MPI_Send(&toSend,1,MPI_INT,rank+1,rank,MCW);//tell the reciver how many links to recive.
			for(i = 0; i < nx; i++){//count links to send up
				if(!contribs->isNodata(i,ny) && contribs->getData(i,ny,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
					if( i>0 && flowDir->getData((long)i-1,(long)ny-1,tempShort) == 8 && src->getData(i-1,ny-1,tempShort) == 1 && idGrid->getData(i-1,ny-1,tempLong) >=0){
						if(sendLink(idGrid->getData(i-1,ny-1,tempLong), rank+1)){//check to see if sent properly
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}else if(flowDir->getData((long)i,(long)ny-1,tempShort) == 7 && src->getData(i,ny-1,tempShort) == 1 && idGrid->getData(i,ny-1,tempLong) >=0){
						if(sendLink(idGrid->getData(i,ny-1,tempLong), rank+1)){//check to see if sent properly
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}else if(i < nx && flowDir->getData((long)i+1,(long)ny-1,tempShort) == 6 && src->getData(i+1,ny-1,tempShort) == 1 && idGrid->getData(i+1,ny-1,tempLong) >=0){
						if(sendLink(idGrid->getData(i+1,ny-1,tempLong), rank+1)){//check to see if sent properl
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}
				}
			}
		}
		//evens send down
	}else{//odd recev
		int i = 0;
		if(rank != 0){//recv from bellow
			MPI_Recv(&toRecv,1,MPI_INT,rank-1,rank-1,MCW,&stat);//how many do I recive?
			int i =0;
			for(i=0;i<toRecv;i++){//recive links
				//MPI_Recv(&sent,1,MPI_INT,rank-1,rank-1,MCW,&stat);//how many do I recive?
				if(recvLink(rank-1))//might need to check if all recived... JJN
					linksRecv++;
				else
					MPI_Abort(MCW,2);
			}
		}
		i = 0;
		if(rank != size-1){//recive from above
			MPI_Recv(&toRecv,1,MPI_INT,rank+1,rank+1,MCW,&stat);//how many do I recive?
			int i =0;
			for(i=0;i<toRecv;i++){//recive links
				//MPI_Recv(&sent,1,MPI_INT,rank-1,rank-1,MCW,&stat);//how many do I recive?
				if(recvLink(rank+1))//might need to check if all recived... JJN
					linksRecv++;
				else
					MPI_Abort(MCW,2);
			}
		}
	}
	MPI_Barrier(MCW);
	
	
	if(rank%2 == 1){//odds send and evens recive
		int i;
		int inext,jnext;
		// The logic here is to examine the row in the border of contribs.
		// If it is less than 0 then there are links dangling that need to be passed
		// examime each neighbor that could be contributing and if it does 
		for(i = 0; i < nx; i++){//count links to send down
			if(!contribs->isNodata(i,-1) && contribs->getData(i,-1,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
				if( i>0 && flowDir->getData((long)i-1,(long)0,tempShort) == 2 
					&& src->getData(i-1,0,tempShort) == 1 && idGrid->getData(i-1,0,tempLong) >=0){
						toSend++;
				}
				if(flowDir->getData((long)i,(long)0,tempShort) == 3 && src->getData(i,0,tempShort) == 1 && idGrid->getData(i,0,tempLong) >=0){
					toSend++;
				}
				if(i < nx && flowDir->getData((long)i+1,(long)0,tempShort) == 4 && src->getData(i+1,0,tempShort) == 1 && idGrid->getData(i+1,0,tempLong) >=0){
					toSend++;
				}
			}
		}
		MPI_Send(&toSend,1,MPI_INT,rank-1,rank,MCW);//tell the reciver how many links to recive.
		for(i = 0; i < nx; i++){//count links to send up
			if(!contribs->isNodata(i,-1) && contribs->getData(i,-1,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
				if( i>0 && flowDir->getData((long)i-1,(long)0,tempShort) == 2 && src->getData(i-1,0,tempShort) == 1 && idGrid->getData(i-1,0,tempLong) >=0){
					if(sendLink(idGrid->getData(i-1,0,tempLong), rank-1)){//check to see if sent properly
						linksSent++;
					}else{//didn't send right.
						MPI_Abort(MCW,2);
					}
				}
				if(flowDir->getData((long)i,(long)0,tempShort) == 3 && src->getData(i,0,tempShort) == 1 && idGrid->getData(i,0,tempLong) >=0){
					if(sendLink(idGrid->getData(i,0,tempLong), rank-1)){//check to see if sent properly
						linksSent++;
					}else{//didn't send right.
						MPI_Abort(MCW,2);
					}
				}
				if(i < nx && flowDir->getData((long)i+1,(long)0,tempShort) == 4 && src->getData(i+1,0,tempShort) == 1 && idGrid->getData(i+1,0,tempLong) >=0){
					if(sendLink(idGrid->getData(i+1,0,tempLong), rank-1)){//check to see if sent properl
						linksSent++;
					}else{//didn't send right.
						MPI_Abort(MCW,2);
					}
				}
			}
		}
		//send down
		toSend = 0;
		if(rank!=size-1){
			for(i = 0; i < nx; i++){//count links to send down
				if(!contribs->isNodata(i,ny) && contribs->getData(i,ny,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
					if( i>0 && flowDir->getData((long)i-1,(long)ny-1,tempShort) == 8 && src->getData(i-1,ny-1,tempShort) == 1 && idGrid->getData(i-1,ny-1,tempLong) >=0){
						toSend++;
					}else if(flowDir->getData((long)i,(long)ny-1,tempShort) == 7 && src->getData(i,ny-1,tempShort) == 1 && idGrid->getData(i,ny-1,tempLong) >=0){
						toSend++;
					}else if(i < nx && flowDir->getData((long)i+1,(long)ny-1,tempShort) == 6 && src->getData(i+1,ny-1,tempShort) == 1 && idGrid->getData(i+1,ny-1,tempLong) >=0){
						toSend++;
					}
				}
			}
			MPI_Send(&toSend,1,MPI_INT,rank+1,rank,MCW);//tell the reciver how many links to recive.
			for(i = 0; i < nx; i++){//count links to send up
				if(!contribs->isNodata(i,ny) && contribs->getData(i,ny,tempShort) < 0){//i am on the boarder and I am negitive, check who points to me and send there id.
					if( i>0 && flowDir->getData((long)i-1,(long)ny-1,tempShort) == 8 && src->getData(i-1,ny-1,tempShort) == 1 && idGrid->getData(i-1,ny-1,tempLong) >=0){
						if(sendLink(idGrid->getData(i-1,ny-1,tempLong), rank+1)){//check to see if sent properly
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}else if(flowDir->getData((long)i,(long)ny-1,tempShort) == 7 && src->getData(i,ny-1,tempShort) == 1 && idGrid->getData(i,ny-1,tempLong) >=0){
						if(sendLink(idGrid->getData(i,ny-1,tempLong), rank+1)){//check to see if sent properly
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}else if(i < nx && flowDir->getData((long)i+1,(long)ny-1,tempShort) == 6 && src->getData(i+1,ny-1,tempShort) == 1 && idGrid->getData(i+1,ny-1,tempLong) >=0){
						if(sendLink(idGrid->getData(i+1,ny-1,tempLong), rank+1)){//check to see if sent properl
							linksSent++;
						}else{//didn't send right.
							MPI_Abort(MCW,2);
						}
					}
				}
			}
		}
		//evens send down
	}else{//odd recev
		if(rank != 0){//recv from bellow
			MPI_Recv(&toRecv,1,MPI_INT,rank-1,rank-1,MCW,&stat);//how many do I recive?
			for(int i=0;i<toRecv;i++){//recive links
				//MPI_Recv(&sent,1,MPI_INT,rank-1,rank-1,MCW,&stat);//how many do I recive?
				if(recvLink(rank-1))//might need to check if all recived... JJN
					linksRecv++;
				else
					MPI_Abort(MCW,2);
			}
		}
		if(rank != size-1){//recive from above
			MPI_Recv(&toRecv,1,MPI_INT,rank+1,rank+1,MCW,&stat);//how many do I recive?
			for(int i=0;i<toRecv;i++){//recive links
				//MPI_Recv(&sent,1,MPI_INT,rank-1,rank-1,MCW,&stat);//how many do I recive?
				if(recvLink(rank+1))//might need to check if all recived... JJN
					linksRecv++;
				else
					MPI_Abort(MCW,2);
			}
		}
	}
	MPI_Barrier(MCW);
	//make sure I total sent is the same as total recived...
	MPI_Allreduce(&linksSent,&totalSent,1,MPI_INT,MPI_SUM,MCW);
	MPI_Allreduce(&linksRecv,&totalRecv,1,MPI_INT,MPI_SUM,MCW);
	int diff = totalSent - totalRecv;
	if(diff != 0)
		MPI_Abort(MCW,1);
	//all done!
	return;
}
long getMagnitude(int32_t Id){
	streamlink* myLink = FindLink(Id);
	if(myLink == NULL)
		MPI_Abort(MCW,7);
	return myLink->magnitude;
}
void getNumLinksAndPoints(long &NumLinks,long &NumPoints){
long ID = -1;
	llnode* current = linkSet.head;
	NumLinks = linkSet.numLinks;
	NumPoints = 0;
	if(linkSet.numLinks == 0)
		return ;
	else
		while(current != NULL){
			NumPoints += current->data->numCoords;
			current = current->next;
		}	
	return;
}

void setLinkInfo(long **LinkIdU1U2DMagShapeid,double **LinkElevUElevDLength,double **PointXY, float **PointElevArea, tdpartition *elev, tiffIO *elevIO)
{
	long counter = 0;
	long pointsSoFar = 0;
	llnode* current = linkSet.head;
	if(linkSet.numLinks == 0)
		return;
	else
	{
        long begcoord=0;
		while(current != NULL){
			LinkIdU1U2DMagShapeid[counter][0] = current->data->Id;
			LinkIdU1U2DMagShapeid[counter][1] = begcoord;
			LinkIdU1U2DMagShapeid[counter][2] = begcoord+current->data->numCoords-1;
			begcoord=LinkIdU1U2DMagShapeid[counter][2]+1;
			LinkIdU1U2DMagShapeid[counter][3] = current->data->d;
			LinkIdU1U2DMagShapeid[counter][4] = current->data->u1;
			LinkIdU1U2DMagShapeid[counter][5] = current->data->u2;
			LinkIdU1U2DMagShapeid[counter][6] = current->data->order;
			LinkIdU1U2DMagShapeid[counter][7] = current->data->shapeId;
			LinkIdU1U2DMagShapeid[counter][8] = current->data->magnitude;
/*
			LinkElevUElevDLength[counter][0] = current->data->elevU;
			LinkElevUElevDLength[counter][1] = current->data->elevD;
			LinkElevUElevDLength[counter][2] = current->data->length;
	*/		
			long i=0;
			double cellarea = (elev->getdxA())*(elev->getdyA());
			for(i=0;i<current->data->numCoords;i++){
				//elevIO->globalXYToGeo(current->data->coord[i].x, current->data->coord[i].y,PointXY[pointsSoFar][0],PointXY[pointsSoFar][1]);
				elevIO->globalXYToGeo(current->data->coord.front().x, current->data->coord.front().y,PointXY[pointsSoFar][0],PointXY[pointsSoFar][1]);	
				//PointElevArea[pointsSoFar][0] = current->data->coord[i].length;
				PointElevArea[pointsSoFar][0] = current->data->coord.front().length;
				//PointElevArea[pointsSoFar][1] = current->data->coord[i].elev;
				PointElevArea[pointsSoFar][1] = current->data->coord.front().elev;
				//PointElevArea[pointsSoFar][2] = current->data->coord[i].area*cellarea;
				PointElevArea[pointsSoFar][2] = current->data->coord.front().area*cellarea;
				//  Pop off queue	
				current->data->coord.pop();
				pointsSoFar++;
			}
			current = current->next;
			counter++;
		}	
	}
	return;
}
streamlink* getFirstLink(){
	streamlink *first;
	first = linkSet.head->data;
	return first;

}
streamlink* getLink(long node){
	llnode* current = linkSet.head;
	if(current == NULL)
		return NULL;
	long i = 0;
	while(current != NULL){
		if(i == node)
			return current->data;
		current = current->next;
		i++;
	}
	return NULL;
}

point* initPoint(tdpartition *elev,tdpartition *areaD8,tdpartition *lengths,long i,long j)
{
	point *thePoint;
	thePoint=new point[1];
	int gi,gj;
	float tempFloat;
	elev->localToGlobal((int)i,(int)j,gi,gj);  
	thePoint->x = gi;
	thePoint->y = gj;
	thePoint->elev = elev->getData(i,j,tempFloat);
	thePoint->area = areaD8->getData(i,j,tempFloat);
	thePoint->length = lengths->getData(i,j,tempFloat);
	return(thePoint);
}

bool ReceiveWaitingLinks(int rank, int size)
{
	//  This returns the number of links waiting to receive from processor with input rank
	if(size<=1)return(false);  // If single processor nothing to do
	int flag,flagup,flagdown,temp;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE, 10, MCW, &flag,&status);  
	// Tag 10 is used because this is the last tag in a link send
	while(flag == 1)
	{
		recvLink(status.MPI_SOURCE);
		MPI_Iprobe(MPI_ANY_SOURCE, 10, MCW, &flag,&status);
	}
	//  logic to see if terminating messages have been received from adjacent processes
	//  in which case the function returns false
	//  Tag 66 is used to pass a terminating message to adjacent processes indicating all links have been sent
	if(rank==0)
	{
		MPI_Iprobe(rank+1, 66, MCW, &flag,&status);
		if(flag == 1)
		{
			MPI_Recv(&temp,1,MPI_INT,rank+1,66,MCW,&status);
			return(false);
		}
		return(true);
	}
	if(rank==size-1)
	{
		MPI_Iprobe(rank-1, 66, MCW, &flag,&status);
		if(flag == 1)
		{
			MPI_Recv(&temp,1,MPI_INT,rank-1,66,MCW,&status);
			return(false);
		}
		return(true);
	}
	MPI_Iprobe(rank-1, 66, MCW, &flagup,&status);
	MPI_Iprobe(rank+1, 66, MCW, &flagdown,&status);
	if( (flagup == 1) && (flagdown == 1))
	{
		MPI_Recv(&temp,1,MPI_INT,rank-1,66,MCW,&status);
		MPI_Recv(&temp,1,MPI_INT,rank+1,66,MCW,&status);
		return(false);
	}
	return(true);
}

#endif
