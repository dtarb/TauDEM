/*  StreamNet function to compute stream networks based on d8 directions.
     
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

#include <mpi.h>
#include <math.h>
#include <iomanip>
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include "shape/shapefile.h"
#include "tardemlib.h"
#include "linklib.h"
#include "streamnet.h"
#include <fstream>

#include <limits>
using namespace std;

shapefile shp1;  //DGT would really like this not to be a global but I do not know how

void createStreamNetShapefile(char *streamnetshp)
{
	shp1.create(streamnetshp,API_SHP_POLYLINE);
	{field f("DOUT_MID",FTDouble,16,1);  shp1.insertField(f,0);}
	{field f("DOUT_START",FTDouble,16,1);  shp1.insertField(f,0);}
	{field f("DOUT_END",FTDouble,16,1);  shp1.insertField(f,0);}
	{field f("WSNO",FTInteger,6,0);  shp1.insertField(f,0);}
	{field f("US_Cont_Area",FTDouble,16,1);  shp1.insertField(f,0);}  //dgt 3/6/2006 changed 4th argument to 1 to force to store as double
	{field f("Straight_Length",FTDouble,16,1);  shp1.insertField(f,0);}  //dgt 3/6/2006 changed 4th argument to 1 to force to store as double
	{field f("Slope",FTDouble,16,12);  shp1.insertField(f,0);}
	{field f("Drop",FTDouble,16,2);  shp1.insertField(f,0);}
	{field f("DS_Cont_Area",FTDouble,16,1);  shp1.insertField(f,0);}  //dgt 3/6/2006 changed 4th argument to 1 to force to store as double
	{field f("Magnitude",FTInteger,6,0);  shp1.insertField(f,0);}
	{field f("Length",FTDouble,16,1);  shp1.insertField(f,0);}
	{field f("Order",FTInteger,6,0);  shp1.insertField(f,0);}
	{field f("DSNODEID",FTInteger,12,0);  shp1.insertField(f,0);}
	{field f("USLINKNO2",FTInteger,6,0);  shp1.insertField(f,0);}
	{field f("USLINKNO1",FTInteger,6,0);  shp1.insertField(f,0);}
	{field f("DSLINKNO",FTInteger,6,0);  shp1.insertField(f,0);}
	{field f("LINKNO",FTInteger,6,0);  shp1.insertField(f,0);}
}

// Write shape from tardemlib.cpp
int reachshape(long *cnet,float *lengthd, float *elev, float *area, double *pointx, double *pointy, long np)
{
// Function to write stream network shapefile

shape * sh = new shp_polyline();
double x,y,length,glength,x1,y1,xlast,ylast,usarea,dsarea,dslast,dl,drop,slope;
int istart,iend,j;

istart=cnet[1];  //  start coord for first link
iend=cnet[2];//  end coord for first link
x1=pointx[0];
y1=pointy[0];
length=0.;
xlast=x1;
ylast=y1;
usarea=area[0];
dslast=usarea;
dsarea=usarea;
long prt = 0;

for(j=0; j<np; j++)  //  loop over points
{
x=pointx[j];
y=pointy[j];
dl=sqrt((x-xlast)*(x-xlast)+(y-ylast)*(y-ylast));
if(dl > 0.)
{
length=length+dl;
xlast=x;  ylast=y;
dsarea=dslast;   // keeps track of last ds area
dslast=area[j];
}

api_point p(x,y);
sh ->insertPoint(p,0);

}
if(np<2){   //  DGT 8/17/04 A polyline seems to require at least 2 data points
//  so where there was only one repeat it.
api_point p(x,y);
sh ->insertPoint(p,0);

}
drop=elev[0]-elev[np-1];
slope=0.;
float dsdist=lengthd[np-1];
float usdist=lengthd[0];
float middist=(dsdist+usdist)*0.5;
if(length > 0.)slope=drop/length;
glength=sqrt((x-x1)*(x-x1)+(y-y1)*(y-y1));

shp1.insertShape(sh,0);
cell v;
v.setValue(cnet[0]);                              shp1[0]->setCell(v,0);
v.setValue(cnet[3]);             shp1[0]->setCell(v,1);
v.setValue(cnet[4]);             shp1[0]->setCell(v,2);
v.setValue(cnet[5]);             shp1[0]->setCell(v,3);
v.setValue(cnet[7]);   shp1[0]->setCell(v,4);
v.setValue(cnet[6]);             shp1[0]->setCell(v,5);
v.setValue(length);                             shp1[0]->setCell(v,6);
v.setValue(cnet[8]);                                shp1[0]->setCell(v,7);
v.setValue(dsarea);                             shp1[0]->setCell(v,8);
v.setValue(drop);                               shp1[0]->setCell(v,9);
v.setValue(slope);                              shp1[0]->setCell(v,10);
v.setValue(glength);                    shp1[0]->setCell(v,11);
v.setValue(usarea);                             shp1[0]->setCell(v,12);
v.setValue(cnet[0]);                                 shp1[0]->setCell(v,13);
v.setValue(dsdist);                             shp1[0]->setCell(v,14);
v.setValue(usdist);                             shp1[0]->setCell(v,15);
v.setValue(middist);                    shp1[0]->setCell(v,16);

//delete sh;  // DGT attempt to minimize memory leaks.  This causes a MPI error

return(0);
}


int netsetup(char *pfile,char *srcfile,char *ordfile,char *ad8file,char *elevfile,char *treefile, char *coordfile, 
			 char *outletshapefile, char *wfile, char *streamnetshp, long useOutlets, long ordert, bool verbose) 
{
	// MPI Init section
	MPI_Init(NULL,NULL);{
		int rank,size;
		MPI_Comm_rank(MCW,&rank);
		MPI_Comm_size(MCW,&size);

		//  Used throughout
		float tempFloat;
		long tempLong;
		short tempShort;

		//  Keep track of time
		double begint = MPI_Wtime();

		//  *** initiate src grid partition from src
		tiffIO srcIO(srcfile, SHORT_TYPE);  
		long TotalX = srcIO.getTotalX();
		long TotalY = srcIO.getTotalY();
		double dx = srcIO.getdx();
		double dy = srcIO.getdy();
		double diag=sqrt((dx*dx)+(dy*dy));

		//Create partition and read data
		tdpartition *src;
		src = CreateNewPartition(srcIO.getDatatype(), TotalX, TotalY, dx, dy, srcIO.getNodata());
		int nx = src->getnx();
		int ny = src->getny();
		int xstart, ystart;  
		src->localToGlobal(0, 0, xstart, ystart);  
		srcIO.read((long)xstart, (long)ystart, (long)ny, (long)nx, src->getGridPointer());

		//  *** initiate flowdir grid partition from dirfile
		tiffIO dirIO(pfile, SHORT_TYPE);
		if(!dirIO.compareTiff(srcIO)){
			printf("pfile and src files not the same size. Exiting \n");
			MPI_Abort(MCW,4);
		}
		//Create partition and read data
		tdpartition *flowDir;
		flowDir = CreateNewPartition(dirIO.getDatatype(), TotalX, TotalY, dx, dy, dirIO.getNodata());
		dirIO.read((long)xstart, (long)ystart, (long)ny, (long)nx, flowDir->getGridPointer());

		tiffIO ad8IO(ad8file, FLOAT_TYPE);
		if(!ad8IO.compareTiff(srcIO)){
			printf("ad8file and src files not the same size. Exiting \n");
			MPI_Abort(MCW,4);
		}
		//Create partition and read data
		tdpartition *areaD8;
		areaD8 = CreateNewPartition(ad8IO.getDatatype(), TotalX, TotalY, dx, dy, ad8IO.getNodata());
		ad8IO.read((long)xstart, (long)ystart, (long)ny, (long)nx, areaD8->getGridPointer());

		tiffIO elevIO(elevfile, FLOAT_TYPE);
		if(!elevIO.compareTiff(srcIO)){
			printf("elevfile and src files not the same size. Exiting \n");
			MPI_Abort(MCW,4);
		}
		//Create partition and read data
		tdpartition *elev;
		elev = CreateNewPartition(elevIO.getDatatype(), TotalX, TotalY, dx, dy, elevIO.getNodata());
		elevIO.read((long)xstart, (long)ystart, (long)ny, (long)nx, elev->getGridPointer());


		//Create empty partition to store new ID information
		tdpartition *idGrid;
		idGrid = CreateNewPartition(LONG_TYPE, TotalX, TotalY, dx, dy, MISSINGLONG);
		//Create empty partition to store number of contributing neighbors
		tdpartition *contribs;
		contribs = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dx, dy, MISSINGSHORT);
		//Create empty partition to stream order
		tdpartition *wsGrid;
		wsGrid = CreateNewPartition(LONG_TYPE, TotalX, TotalY, dx, dy, MISSINGLONG);

		makeLinkSet();

		long i,j,inext,jnext;
		int *xOutlets;
		int *yOutlets;
		int numOutlets=0;
		double *x=NULL;
		double *y=NULL;
		int *ids=NULL;
		// Read outlets 
		if( useOutlets == 1) {
			if(rank==0){
				if(readoutlets(outletshapefile, &numOutlets, x, y,ids) !=0){
					printf("Exiting \n");
					MPI_Abort(MCW,5);
				}else {
					MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);
					MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
					MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
					MPI_Bcast(ids, numOutlets, MPI_INT, 0, MCW);
				}
			}
			else {
				MPI_Bcast(&numOutlets, 1, MPI_INT, 0, MCW);

				//x = (double*) malloc( sizeof( double ) * numOutlets );
				//y = (double*) malloc( sizeof( double ) * numOutlets );
				x = new double[numOutlets];
				y = new double[numOutlets];
				ids = new int[numOutlets];

				MPI_Bcast(x, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(y, numOutlets, MPI_DOUBLE, 0, MCW);
				MPI_Bcast(ids, numOutlets, MPI_INT, 0, MCW);
			}


			xOutlets = new int[numOutlets];
			yOutlets = new int[numOutlets];
		//  Loop over all points in shapefile.  If they are in the partition (after conversion from 
		//   geographic to global, then global to partition coordinates) set the idGrid to hold ids value
		//   leaving the remainder of idGrid to no data
			for(i = 0; i<numOutlets;i++){
				ad8IO.geoToGlobalXY(x[i], y[i], xOutlets[i], yOutlets[i]);
				int xlocal, ylocal;
				idGrid->globalToLocal(xOutlets[i], yOutlets[i],xlocal,ylocal);
				if(idGrid->isInPartition(xlocal,ylocal)){   //xOutlets[i], yOutlets[i])){
					idGrid->setData(xlocal,ylocal,(long)ids[i]);  //xOutlets[i], yOutlets[i], (long)ids[i]);
				}
			}
			idGrid->share();
		}

		// Timer read dime
		double readt = MPI_Wtime();
		if(verbose)  // debug writes
		{
			printf("Files read\n");
			printf("Process: %d, TotalX: %d, TotalY: %d\n",rank,TotalX,TotalY);
			printf("Process: %d, nx: %d, ny: %d\n",rank,nx,ny);
			printf("Process: %d, xstart: %d, ystart: %d\n",rank,xstart,ystart);
			printf("Process: %d, numOutlets: %d\n",rank,numOutlets);
			fflush(stdout);
		}

//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//  Block to evaluate distance to outlet

		//Create empty partition to store lengths
		tdpartition *lengths;
		lengths = CreateNewPartition(FLOAT_TYPE, TotalX, TotalY, dx, dy, MISSINGFLOAT);

		src->share();
		flowDir->share();

//  Initialize queue and contribs partition	
		queue <node> que;
		node t;
		int p;
		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				// If I am on stream and my downslope neighbor is on stream contribs is 1
				// If I am on stream and my downslope neighbor is off stream contribs is 0 because 
				//  I am at the end of a stream
				if(!src->isNodata(i,j) && src->getData(i,j,tempShort)>0 && !flowDir->isNodata(i,j))
				{
					p=flowDir->getData(i,j,tempShort);
					inext=i+d1[p];
					jnext=j+d2[p];
					if(!src->isNodata(inext,jnext) && src->getData(inext,jnext,tempShort)>0 && !flowDir->isNodata(inext,jnext))
						contribs->setData(i,j,(short)1);//addData? What if it is a junction and has more floing into it?
					else
					{
						contribs->setData(i,j,(short)0);
						t.x=i;
						t.y=j;
						que.push(t);
					}
				}
			}
		}
// while loop where each process empties its que, then shares border info, and repeats till everyone is done
		bool finished=false;
		int m;
		//int totalP=0;
		while(!finished){
			contribs->clearBorders();  
			while(!que.empty()){
				t=que.front(); 
				i=t.x; 
				j=t.y;
				que.pop();
				p=flowDir->getData(i,j,tempShort);
				float llength=0.;
				inext=i+d1[p];
				jnext=j+d2[p];
				if(!lengths->isNodata(inext,jnext))
				{
					llength += lengths->getData(inext,jnext,tempFloat);
					if(p==1||p==5)llength=llength+dx;
					if(p==3||p==7)llength=llength+dy;
					if(p%2==0)llength=llength+diag;
				}
				lengths->setData(i,j,llength);
				//totalP++;
				//  Find if neighbor points to me and reduce its dependency by 1
				for(m=1;m<=8;++m){
					inext=i+d1[m];
					jnext=j+d2[m];
					if(pointsToMe(i,j,inext,jnext,flowDir) && !src->isNodata(inext,jnext)  && src->getData(inext,jnext,tempShort)>0)
					{
						contribs->addToData(inext,jnext,(short)(-1));
						if(contribs->isInPartition(inext,jnext) && contribs->getData(inext,jnext,tempShort)==0)
						{
							t.x=inext;
							t.y=jnext;
							que.push(t);
						}
					}
				}
			}
			//Pass information across partitions

			contribs->addBorders();
			lengths->share();

			//If this created a cell with no contributing neighbors, put it on the queue
			for(i=0; i<nx; i++){
				if(contribs->getData(i, -1, tempShort)!=0 && contribs->getData(i, 0, tempShort)==0)
				{
					t.x = i;
					t.y = 0;
					que.push(t);
				}
				if(contribs->getData(i, ny, tempShort)!=0 && contribs->getData(i, ny-1, tempShort)==0)
				{
					t.x = i;
					t.y = ny-1;
					que.push(t); 
				}
			}

			//Check if done
			finished = que.empty();
			finished = contribs->ringTerm(finished);
		}

		// Timer lengtht
		double lengthct = MPI_Wtime();
		if(verbose)
			cout << rank << " Starting block to create links"  << endl;

//  Now length partition is evaluated

//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//  Block to create links

//  Reinitialize contribs partition
		delete contribs;
		contribs = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dx, dy, MISSINGSHORT);
		long k = 0;

		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				// If I am on stream add 1 for each neighbor on stream that contribs to me
				if(!src->isNodata(i,j) && src->getData(i,j,tempShort)>0 && !flowDir->isNodata(i,j))
				{
					short tempc=0;  
					for(k=1;k<=8;k++)
					{
						inext=i+d1[k];
						jnext=j+d2[k];
						if(pointsToMe(i,j,inext,jnext,flowDir) && !src->isNodata(inext,jnext)  && src->getData(inext,jnext,tempShort)>0)
						{
							tempc=tempc+1;  
						}
					}
					contribs->setData(i,j,tempc);
					if(tempc==0)
					{
						t.x=i;
						t.y=j;
						que.push(t);
					}
				}
			}
		}
		if(verbose)
			cout << rank << "	Starting main link creation loop"  << endl;
//XXXXXXXXXXXX  MAIN WHILE LOOP
// While loop where each process empties its que, then shares border info, and repeats till everyone is done
		finished=false;
		while(!finished){
			bool linksToReceive=true;
			contribs->clearBorders();
			while(!que.empty()){
				t=que.front();
				i=t.x;
				j=t.y;
				que.pop();
				// begin cell processing
				short nOrder[8];  // neighborOrders
				short inneighbors[8];  // inflowing neighbors
				bool junction; // junction set to true/false in newOrder
				long Id;

				//  Count number of grid cells on stream that point to me.  
				// (There will be no more than 8, usually only 0, 1,2 or 3.  The queue and 
				// processing order ensures that the necessary results have already been 
				//  evaluated for these.  In this case it should be a partially complete link 
				//  object referenced by a value in a link object partition).  Number that flow 
				//  to cell is k.  Record the order and direction to each. 
				k=0;
				for(m=1;m<=8;++m){
					inext=i+d1[m];
					jnext=j+d2[m];
					if(pointsToMe(i,j,inext,jnext,flowDir) && !src->isNodata(inext,jnext) && src->getData(inext,jnext,tempShort)>0 && !idGrid->isNodata(inext,jnext)){//!idGrid->isNodata(inext,jnext)){
						inneighbors[k]=m;  
						//  stream orders are stored in lengths grid to save memory
						nOrder[k]= (short)lengths->getData(inext,jnext,tempFloat); ///  Assign value//  Get the order from the link referenced by the value in idGrid and //   save it in nOrder[m-1]
						k=k+1;  // count inflows
					}
				}

				// Determine downslope neighbor
				long nextx,nexty;
				p=flowDir->getData(i,j,tempShort);
				nextx=t.x+d1[p];
				nexty=t.y+d2[p];

				// Determine if mandatory junction indicated by a value in idGrid
				bool mandatoryJunction=false;
				long shapeID;
				if(!idGrid->isNodata(i,j))
				{
					mandatoryJunction=true;
					shapeID=idGrid->getData(i,j,shapeID);
				}

				//-	determine if terminal link by if downslope neighbor is out of domain
				bool terminal = false;
				if(!src->hasAccess(nextx,nexty)  || src->isNodata(nextx,nexty) 
					|| (src->getData(nextx,nexty,tempShort) != 1))
					terminal = true;

				//  instantiate point
				point* addPoint;
				addPoint = initPoint(elev,areaD8,lengths,i,j);

				//  Case for start of flow path
				if(k==0){
					//cout << "k=0"  << endl;
					//  This is the start of a stream - instantiate new link
					streamlink *thisLink;
					thisLink = createLink(-1, -1, -1, addPoint,1); //, dx,dy);
					long u1= thisLink->Id;
					idGrid->setData(i,j, u1);
					wsGrid->setData(i,j, u1);
					lengths->setData(i,j,(float)thisLink->order);

					//  Handle mandatory junction
					if(mandatoryJunction){
						appendPoint(u1, addPoint);
						thisLink->shapeId = shapeID;
						if(!terminal){
							streamlink *newLink;
							newLink = createLink(u1,-1,-1, addPoint, 1); //, dx,dy);
							newLink->order =  1;	
							setDownLinkId(u1,newLink->Id);		
							newLink->magnitude = 1;
							terminateLink(u1);
							idGrid->setData(i,j,newLink->Id);
							//  Here idGrid and wsGrid intentionally out of sync
							//  idGrid to be used to retrieve upstream link for next cell down
							//  wsGrid used to hold the id for delineation of watersheds which 
							//  is the link that ends at this point
						}
					}else if(terminal){  // handle terminal that is not mandatory junction
						appendPoint(u1, addPoint);
					}
				}

				//  Case for ongoing flow path with single inflow
				else if(k==1){
					long u1=idGrid->getData(i+d1[inneighbors[0]],j+d2[inneighbors[0]],tempLong);
					appendPoint(u1, addPoint);
					streamlink *thisLink;
					thisLink=FindLink(u1);
					//cout << "append done"  << endl;
					idGrid->setData(i,j,u1);
					wsGrid->setData(i,j,u1);
					lengths->setData(i,j,(float)nOrder[0]);

					//  Handle mandatory junction case
					if(mandatoryJunction){
						thisLink->shapeId = shapeID;
						if(!terminal){
							streamlink *newLink;
							newLink = createLink(u1,-1,-1, addPoint, 1); //, dx,dy);
							newLink->order =  thisLink->order;	
							setDownLinkId(u1,newLink->Id);		
							newLink->magnitude = thisLink->magnitude;
							terminateLink(u1);
							idGrid->setData(i,j,newLink->Id);
							//  TODO Here need to assign WSNO = -1
						}
					}
				}

				// Case for multiple inflows
				else if(k>1){
					//	rank incoming flow paths from highest to lowest order and process in this order
					//cout << "k>1 junction: "  << k << endl;
					//Dans Code to sort the two largest to the bottom of the list
					short temp;
					for(int a=0;a<2;++a){
						for(int b=0;b<k-1;++b){
							if(nOrder[b]>nOrder[b+1]){
								temp=nOrder[b];
								nOrder[b]=nOrder[b+1];
								nOrder[b+1]=temp;

								temp = inneighbors[b];
								inneighbors[b] = inneighbors[b+1];
								inneighbors[b+1] = temp;//keep track of where they came from. 
							}
						}
					}	
					// Determine order out
					short oOut;//if the last two have the same order, there is a junction, so bump the order
					if(nOrder[k-2]==nOrder[k-1]){
							//junction=true;
						oOut=nOrder[k-1]+1;
					}else{
						oOut=nOrder[k-1];
					}
					long u1, u2;
					u1 = idGrid->getData(i+d1[inneighbors[k-1]],j+d2[inneighbors[k-1]],tempLong);
					u2 = idGrid->getData(i+d1[inneighbors[k-2]],j+d2[inneighbors[k-2]],tempLong);
					appendPoint(u1, addPoint);
					appendPoint(u2, addPoint);
					streamlink *newLink;
					newLink = createLink(u1,u2,-1,addPoint,1); //,dx,dy);
					newLink->order = oOut;
					setDownLinkId(u1,newLink->Id);
					setDownLinkId(u2,newLink->Id);
					newLink->magnitude = getMagnitude(u1) + getMagnitude(u2);
					
					//  TODO add WSNO = -1

					terminateLink(u1);
					terminateLink(u2);

					//  Handle remaining incoming flow paths
					for(m=2; m<k; m++)   //  Loop is only entered if k>=3
					{
						u1 = newLink->Id;
						u2 = idGrid->getData(i+d1[inneighbors[k-m-1]],j+d2[inneighbors[k-m-1]],tempLong);

						appendPoint(u1, addPoint);
						appendPoint(u2, addPoint);

						// create a new link
						newLink = createLink(u1,u2,-1,addPoint,1);  //,dx,dy);
						//  set order of new link as order out
						newLink->order = oOut;
						//  set u1 to previously created link
						setDownLinkId(u1, newLink->Id);
						setDownLinkId(u2, newLink->Id);
						newLink->magnitude = getMagnitude(u1) + getMagnitude(u2);
						
						//  TODO add WSNO = -1

						terminateLink(u1);
						terminateLink(u2);
					}
					idGrid->setData(i,j,newLink->Id);// Set the idGrid to the new link ID?
					wsGrid->setData(i,j,newLink->Id);
					lengths->setData(i,j,(float)oOut);
					if(mandatoryJunction){
						// create a zero length link to the mandatory junction
						u1 = newLink->Id;
						appendPoint(u1, addPoint);
						newLink->shapeId=shapeID;

						//  TODO assign u1 to WSNO
						if(!terminal){
							newLink = createLink(u1,-1,-1,addPoint,1); //,dx,dy);
							//  set order of new link as order out
							newLink->order = oOut;
							//  set u1 to previously created link
							setDownLinkId(u1, newLink->Id);
							newLink->magnitude = getMagnitude(u1);
							idGrid->setData(i,j,newLink->Id);// Set the idGrid to the new link ID?
							//  TODO assign -1 to WNSO
						}
					}else if(terminal){
						u1 = newLink->Id;
						appendPoint(u1, addPoint);
						//  TODO assign u1 to WSNO
					}
				}
				//Check if down neighbor needs to be added to que
				contribs->addToData(nextx,nexty,(short)-1);
				if(contribs->isInPartition(nextx,nexty))
				{
					if(contribs->getData(nextx, nexty, tempShort) == 0 ){
						t.x=nextx;
						t.y=nexty;
						que.push(t);
					}
				}
				//  IF link is to be sent
				//     ID which process to go to (rank - 1 or rank + 1)
				//     Add id to a list for that process
				//     count number for that process
				else 
				{
					//  Package up the link that either began or is in process at cell i,j and send to downstream partition
					//  Link has to be sent
					if(nexty<0 && rank>0)  // up
					{
						sendLink(idGrid->getData(i,j,tempLong),rank-1);
						//idToSendUp[numToSendUp]=idGrid->getData(i,j,tempLong);
						//numToSendUp++;
					}
					if(nexty>=ny && rank < size-1)
					{
						sendLink(idGrid->getData(i,j,tempLong),rank+1);
						//idToSendDown[numToSendDown]=idGrid->getData(i,j,tempLong);
						//numToSendDown++;
					}
				}
				//  Receive any waiting links
				//  Only call this if flag to receive links is true because if flag has been set
				//  to false then both adjacent processors have indicated no more to come and an additional
				//  call would set true again
				if(linksToReceive)  
					linksToReceive=ReceiveWaitingLinks(rank,size);
			}
			if(verbose)
				cout << rank << " End link creation pass - about to share" << endl;
			if(size>1)
			{
				//  DGT Attempt for avoiding links building up in buffer
				int temp;
				if(rank<size -1)
					MPI_Send(&temp,1,MPI_INT,rank+1,66,MCW);//send message saying done
				if(rank>0)
					MPI_Send(&temp,1,MPI_INT,rank-1,66,MCW);//send message saying done
				if(verbose)
					cout << rank << " Done messages sent" << endl;
				while(linksToReceive)
				{
					//  This is a spinning loop to receive links that terminates as false when 
					//  message 66 is received from adjacent processes
					linksToReceive=ReceiveWaitingLinks(rank,size);
				}
				if(verbose)
					cout << rank << " Done link receives" << endl;

				//Pass information
				//any touched border cells have negative numbers in them.
				//push them back to their owners, add the negatives to local
				//cells, and put the zero cells on the local que.
				//cout << "loop finished "  << endl;
				idGrid->share();
					//cout << rank << " Done idGrid share" << endl;
				wsGrid->share();
					//cout << rank << " Done wsGrid share" << endl;
				lengths->share();

				//MPI_Barrier(MCW);  //DGT thinks this barrier not needed if above works

				//linksToReceive=ReceiveWaitingLinks(rank,size);

				//SendAndReciveLinks(nx,ny,idGrid,contribs,flowDir,src);
				
				//cout << rank << " about to add " << endl;
				//cout << "top border:  " << contribs->getData(448,-1,tempShort) <<endl;
				//cout << "top row: " << contribs->getData(448,0,tempShort) <<endl;
				//cout << " bottom row: " << contribs->getData(448,ny-1,tempShort) <<endl;
				//cout << " bottom border: " << contribs->getData(448,ny,tempShort) <<endl;
				//cout << endl;
				//	
				contribs->addBorders();

				//cout << rank << " After add borders " << endl;
				//cout << "top border:  " << contribs->getData(448,-1,tempShort) <<endl;
				//cout << "top row: " << contribs->getData(448,0,tempShort) <<endl;
				//cout << " bottom row: " << contribs->getData(448,ny-1,tempShort) <<endl;
				//cout << " bottom border: " << contribs->getData(448,ny,tempShort) <<endl;
				//cout << endl;

				for(i=0; i<nx; i++){//todo fix this to add passed to queue
					if(contribs->getData(i, 0, tempShort) == 0 && contribs->getData(i,-1,tempShort)!=0) //may receive flow from more than 1
					{
						t.x = i;
						t.y = 0;
						que.push(t);
					}
					if(contribs->getData(i, ny-1, tempShort) == 0 && contribs->getData(i,ny,tempShort)!=0)//only looks at one direction of flow, out of a partition && idGrid->isNodata(i,ny-1) && src->getData(i,ny-1,tempShort)==1
					{
						t.x = i;
						t.y = ny-1;
						que.push(t); 
					}
				}
				finished = que.empty();
				MPI_Barrier(MCW);
				int total = 0;
				int AllSum = 0;
				if(!finished)
					total = que.size();
				MPI_Allreduce(&total,&AllSum,1,MPI_INT,MPI_SUM,MCW);
				if(AllSum == 0)
					finished = true;
				else
					finished = false;
				if(verbose)
				{
					cout << rank << " Remaining in partition queue: " << total << endl;
					cout << rank << " Remaining total: " << AllSum << endl;
				}
			}
			else
				finished=true;  // For size == 1 no partition loop
		}	

//  Now all links have been created
		// Timer
		double linkct = MPI_Wtime();
		if(verbose)
			cout << rank << " Starting to write links"  << endl;
	//	exit(1);

//  XXXXXXXXX
//  Block to write link data
		long myNumLinks;
		long myNumPoints;
		long totalNumLinks;
		long totalNumPoints;
		long relativePointStart;//if 9 then line 9 is first to be filled by me

		double **PointXY;
		float **PointElevArea;

		long **LinkIdU1U2DMagShapeidCoords;
		double **LinkElevUElevDLength;
		long *buf;
		long *ptr;
		int place;
		int bsize = sizeof(long)+ MPI_BSEND_OVERHEAD;  
		buf = new long[bsize];
		getNumLinksAndPoints(myNumLinks,myNumPoints);
		//MPI_Barrier(MCW);

		i = 0;
		if(myNumLinks > 0){
			LinkIdU1U2DMagShapeidCoords = new long*[myNumLinks];
			for(i=0;i<myNumLinks;i++)
				LinkIdU1U2DMagShapeidCoords[i] = new long[9];

			LinkElevUElevDLength = new double*[myNumLinks];
			for(i = 0;i<myNumLinks;i++)	
				LinkElevUElevDLength[i] = new double[3];

			if(myNumPoints>0)
			{
				PointXY = new double*[myNumPoints];
				for(i=0;i<myNumPoints;i++)
					PointXY[i] = new double[2];

				PointElevArea = new float*[myNumPoints];
				for(i=0;i<myNumPoints;i++)
					PointElevArea[i] = new float[3];

				setLinkInfo(LinkIdU1U2DMagShapeidCoords,LinkElevUElevDLength,PointXY,PointElevArea,elev,&elevIO);
			}
		}

		int ibuf;
		int procNumPoints;
		int totalPoints = 0;
		float pbuf[3];
		double pbuf1[2];
		MPI_Status mystatus;
		
		int numPointsPrinted =0;
		int procNumLinks = 0;
		long treeBuf[9];

		int ilink, ipoint;
		if(rank==0){//open output point file
			FILE *fTreeOut;
			fTreeOut = fopen(treefile,"w");// process 0 writes all of its stuff
			FILE *fout;
			fout = fopen(coordfile,"w");

			//  Open shapefile
			createStreamNetShapefile(streamnetshp);

			for(ilink=0;ilink<myNumLinks;ilink++){//only once per link
				fprintf(fTreeOut,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",LinkIdU1U2DMagShapeidCoords[ilink][0],LinkIdU1U2DMagShapeidCoords[ilink][1],LinkIdU1U2DMagShapeidCoords[ilink][2],LinkIdU1U2DMagShapeidCoords[ilink][3],LinkIdU1U2DMagShapeidCoords[ilink][4],LinkIdU1U2DMagShapeidCoords[ilink][5],LinkIdU1U2DMagShapeidCoords[ilink][6],LinkIdU1U2DMagShapeidCoords[ilink][7],LinkIdU1U2DMagShapeidCoords[ilink][8]);
				//fflush(fTreeOut);
				long i1=LinkIdU1U2DMagShapeidCoords[ilink][1];
				long i2=LinkIdU1U2DMagShapeidCoords[ilink][2];
				float *lengthd, *elev, *area;
				double *pointx, *pointy;
				lengthd = new float[i2-i1+1];
				elev = new float[i2-i1+1];
				area = new float[i2-i1+1];
				pointx = new double[i2-i1+1];
				pointy = new double[i2-i1+1];
				for(ipoint=i1;ipoint<=i2;ipoint++){
					fprintf(fout,"\t%f\t%f\t%f\t%f\t%f\n",PointXY[ipoint][0],PointXY[ipoint][1],PointElevArea[ipoint][0],PointElevArea[ipoint][1],PointElevArea[ipoint][2]);
					lengthd[ipoint-i1]=PointElevArea[ipoint][0];
					elev[ipoint-i1]=PointElevArea[ipoint][1];
					area[ipoint-i1]=PointElevArea[ipoint][2];
					pointx[ipoint-i1]=PointXY[ipoint][0];
					pointy[ipoint-i1]=PointXY[ipoint][1];
				}
				//  Write shape
				long cnet[9];
				for(int iref=0; iref<9; iref++)
					cnet[iref]=LinkIdU1U2DMagShapeidCoords[ilink][iref];
				reachshape(cnet,lengthd,elev,area,pointx,pointy,i2-i1+1);
				delete lengthd; // DGT to free memory
				delete elev;
				delete area;
				delete pointx;
				delete pointy;

			} // process 0 recvs data from other processes, writes to file
			if(myNumLinks == 0)
				numPointsPrinted = 0;
			else
				numPointsPrinted = LinkIdU1U2DMagShapeidCoords[myNumLinks-1][2]+1;
			for(i=1;i<size;i++){// send message to next process to wake it up
				MPI_Send(&ibuf,1,MPI_INT,i,0,MCW);// get numpoints from that process
				MPI_Recv(&procNumLinks,1,MPI_INT,i,0,MCW,&mystatus);//get points one at a time and print them to file
				for(ilink=0;ilink<procNumLinks;++ilink){
					MPI_Recv(&treeBuf,9,MPI_LONG,i,1,MCW,&mystatus);
					fprintf(fTreeOut,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",treeBuf[0],treeBuf[1]+numPointsPrinted,treeBuf[2]+numPointsPrinted,treeBuf[3],treeBuf[4],treeBuf[5],treeBuf[6],treeBuf[7],treeBuf[8]);

					MPI_Recv(&procNumPoints,1,MPI_INT,i,0,MCW,&mystatus);//get points one at a time and print them to file
			// Variables for shape
					float *lengthd, *elev, *area;
					double *pointx, *pointy;
					lengthd = new float[procNumPoints];
					elev = new float[procNumPoints];
					area = new float[procNumPoints];
					pointx = new double[procNumPoints];
					pointy = new double[procNumPoints];
	
					for(ipoint=0;ipoint<procNumPoints;++ipoint){
						MPI_Recv(&pbuf1,2,MPI_DOUBLE,i,1,MCW,&mystatus);
						MPI_Recv(&pbuf,3,MPI_FLOAT,i,1,MCW,&mystatus);
						fprintf(fout,"\t%f\t%f\t%f\t%f\t%f\n",pbuf1[0],pbuf1[1],pbuf[0],pbuf[1],pbuf[2]); 

						lengthd[ipoint]=pbuf[0];
						elev[ipoint]=pbuf[1];
						area[ipoint]=pbuf[2];
						pointx[ipoint]=pbuf1[0];
						pointy[ipoint]=pbuf1[1];
					}
					//  Write shape
					reachshape(treeBuf,lengthd,elev,area,pointx,pointy,procNumPoints);
					delete lengthd; // DGT to free memory
					delete elev;
					delete area;
					delete pointx;
					delete pointy;

					numPointsPrinted += treeBuf[2]+1;//might need adjustmetn JJN
				}
			} 
			fclose(fTreeOut);
			fclose(fout);
			shp1.close(streamnetshp);
		}else{//other processes send their stuff to process 0
			//first, wait to recieve word from process 0
			MPI_Recv(&ibuf,1,MPI_INT,0,0,MCW,&mystatus);//then, send myNumPoints
			MPI_Send(&myNumLinks,1,MPI_INT,0,0,MCW);//pack up each point into a buffer, and send it
			for(ilink=0;ilink<myNumLinks;++ilink){
				treeBuf[0] = LinkIdU1U2DMagShapeidCoords[ilink][0];
				treeBuf[1] = LinkIdU1U2DMagShapeidCoords[ilink][1];
				treeBuf[2] = LinkIdU1U2DMagShapeidCoords[ilink][2];
				treeBuf[3] = LinkIdU1U2DMagShapeidCoords[ilink][3];
				treeBuf[4] = LinkIdU1U2DMagShapeidCoords[ilink][4];
				treeBuf[5] = LinkIdU1U2DMagShapeidCoords[ilink][5];
				treeBuf[6] = LinkIdU1U2DMagShapeidCoords[ilink][6];
				treeBuf[7] = LinkIdU1U2DMagShapeidCoords[ilink][7];
				treeBuf[8] = LinkIdU1U2DMagShapeidCoords[ilink][8];
				MPI_Send(&treeBuf,9,MPI_LONG,0,1,MCW);
				int ptsInLink=treeBuf[2]-treeBuf[1]+1;
				MPI_Send(&ptsInLink,1,MPI_INT,0,0,MCW);//pack up each point into a buffer, and send it
				for(ipoint=treeBuf[1];ipoint<=treeBuf[2];++ipoint){
					pbuf1[0]=PointXY[ipoint][0];
					pbuf1[1]=PointXY[ipoint][1];
					pbuf[0]=PointElevArea[ipoint][0];
					pbuf[1]=PointElevArea[ipoint][1];
					pbuf[2]=PointElevArea[ipoint][2];
					MPI_Send(&pbuf1,2,MPI_DOUBLE,0,1,MCW);
					MPI_Send(&pbuf,3,MPI_FLOAT,0,1,MCW);
			//		MPI_Recv(&ibuf,1,MPI_INT,0,0,MCW,&mystatus);
				}
			}
		}  // just be sure we're all here
		//MPI_Barrier(MCW);

		// Timer - link write time
		double linkwt = MPI_Wtime();
		if(verbose)
			cout << rank << " Starting to calculate watershed"  << endl;

// XXXXXXXXXXXXXXXXXXXXXXXXX
//  Block to calculate watersheds

		//Calculate watersheds....
		//  Reinitialize partition to delete leftovers from process above
		delete contribs;
		contribs = CreateNewPartition(SHORT_TYPE, TotalX, TotalY, dx, dy, MISSINGSHORT);
		//MPI_Barrier(MCW);//fails here...
		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				// If my downslope neighbor has an id, contribs is 0 and I am ready to evaluate so go on q, else contribs is 1
				if(!flowDir->isNodata(i,j))
				{   //  Only put on queue to do if it is off the stream network
					if(src->isNodata(i,j) || src->getData(i,j,tempShort) < 1) //idGrid->isNodata(i,j))  //  Only if it is no data do we have to do anything 
					{
						p=flowDir->getData(i,j,tempShort);
						inext=i+d1[p];
						jnext=j+d2[p];
						if(src->hasAccess(inext,jnext))  //  Only do anything with cells for which we have access to downslope cell
						{
							if(src->isNodata(inext,jnext) || src->getData(inext,jnext,tempShort) < 1){ //idGrid->isNodata(inext,jnext))
								contribs->setData(i,j,(short)1);  //  downslope cell not on stream
								wsGrid->setToNodata(i,j);  // overwrite grid value that may have been inherited from outlet not on stream
							}
							else
							{
								contribs->setData(i,j,(short)0); //  downslope cell on stream
								t.x=i;
								t.y=j;
								que.push(t);
							}
						}
					}//  Here on stream
					else if(ordert==0)  // Ordert = 0 indicates that all watersheds are to be labeled with the number 1
					{
						tempLong=1;  
						wsGrid->setData(i,j,tempLong);
					}
				}
			}
		}
		// each process empties its que, then shares border info, and repeats till everyone is done
		finished=false;
		while(!finished){
			contribs->clearBorders();
			while(!que.empty()){
				t=que.front();
				que.pop();
				i = t.x;
				j = t.y;
				p=flowDir->getData(i,j,tempShort);
				inext=i+d1[p];
				jnext=j+d2[p];
				if(ordert > 0)  // Ordert > 0 indicates that all watersheds are to be labeled with different numbers
					wsGrid->setData(i,j,wsGrid->getData(inext,jnext,tempLong));
				else
				{
					tempLong=1;  // Ordert = 0 indicates that all watersheds are to be labeled with the number 1
					wsGrid->setData(i,j,tempLong);
				}
				//  Now find all cells that point to me and decrease contribs and put on queue if necessary
				for(m=1;m<=8;++m){
					inext=i+d1[m];
					jnext=j+d2[m];
					if(pointsToMe(i,j,inext,jnext,flowDir))
					{
						contribs->addToData(inext,jnext,(short)(-1));
						if(contribs->isInPartition(inext,jnext) && contribs->getData(inext,jnext,tempShort)==0)
						{
							t.x=inext;
							t.y=jnext;
							que.push(t);
						}
					}
				}
			}
			//Pass information across partitions
			contribs->addBorders();
			wsGrid->share();
			for(i=0; i<nx; i++){
				if(contribs->getData(i, -1, tempShort)!=0 && contribs->getData(i, 0, tempShort)==0)//only looks at one direction of flow, out of a partition
				{
					t.x = i;
					t.y = 0;
					que.push(t);
				}
				if(contribs->getData(i, ny, tempShort) != 0 && contribs->getData(i, ny-1, tempShort)==0)//only looks at one direction of flow, out of a partition
				{
					t.x = i;
					t.y = ny-1;
					que.push(t); 
				}
			}
			//Check if done
			finished = que.empty();
			finished = contribs->ringTerm(finished);
		}
		// Timer - watershed label time
		double wshedlabt = MPI_Wtime();

		long wsGridNodata=MISSINGLONG;
		short ordNodata=MISSINGSHORT;
		tiffIO wsIO(wfile, LONG_TYPE,&wsGridNodata,ad8IO);
		wsIO.write(xstart, ystart, ny, nx, wsGrid->getGridPointer());

		//  Use contribs as a short to write out order data that was held in lengths
		tempShort=0;
		for(i=0; i<nx; i++)
			for(j=0; j<ny; j++)
			{
				contribs->setData(i,j,tempShort);
				if(!lengths->isNodata(i,j))
					contribs->setData(i,j,(short)lengths->getData(i,j,tempFloat));
			}
		tiffIO ordIO(ordfile, SHORT_TYPE,&ordNodata,ad8IO);
		ordIO.write(xstart, ystart, ny, nx, contribs->getGridPointer());
		
		// Timer - write time
		double writet = MPI_Wtime();
        //cout << "Ready to end"  << endl;
		if( rank == 0)
			printf("Processors: %d\nRead time: %f\nLength compute time: %f\nLink compute time: %f\nLink write time: %f\nWatershed compute time: %f\nWrite time: %f\nTotal time: %f\n",size,readt-begint, lengthct-readt,linkct-lengthct,linkwt-linkct,wshedlabt-linkwt, writet-wshedlabt, writet-begint);


	}MPI_Finalize();
	return 0;
}

