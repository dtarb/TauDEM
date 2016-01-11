/*  DropAnalysis function that applies a series of thresholds (determined from the input parameters) 
  to the input ssa grid and outputs in the drp.txt file the stream drop statistics table.  
  
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


#include <mpi.h>
#include <math.h>
#include <iomanip>
#include <queue>
#include <iostream>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include "DropAnalysis.h"
using namespace std;

//does the appropriate updates when a junction is found
void updateAtJunction(short oOut,long i, long ni,long j, long nj, long nx, long ny, tdpartition *dirData,
                      tdpartition *orderOut, tdpartition *elevOut, 
                      tdpartition *elevData, bool &newstream,
                      float &s1,float &s1sq,float &s2,float &s2sq,long &n1,long &n2){
	short o;
	float drop;
	float e;
	//simply return if at partition side edge
	//TODO use either hasaccess or isinpartition functions
	//if(ni<=0||ni>=nx-1||j<0||j>ny)return;
	if(!orderOut->hasAccess(ni,nj))return;
	//simply return if ni,nj doesn't point to i,j
	if(!pointsToMe(i,j,ni,nj,dirData) || orderOut->isNodata(ni,nj))return;

	//get the order of the pointing cell
	o=orderOut->getData(ni,nj,o);

	// no updates if an order 0 cell points at this cell
	// if(o<=0)return;  //DGT should never happen

	// if the order increases here, there is an end to the stream segment
	if(o<oOut){
		//  This is the terminus of a stream so accumulate drops
		drop = elevOut->getData(ni,nj,e)-elevData->getData(i,j,e);
		// the update for the order1 segments is handled differently...
		if(o==1){
			s1= s1+drop;
			s1sq= s1sq+drop*drop;
			n1= n1+1;
			//printf("Junction: %d %d %d %d\n",i,j,ni,nj);
			//fflush(stdout);
		// ... from the rest of the segments
		}else{
			s2= s2+drop;
			s2sq= s2sq+drop*drop;
			n2= n2+1;
		}
	}else{
		//  This is the continuation of a main stream to pass elevOut on down
		elevOut->setData(i,j,elevOut->getData(ni,nj,e));
		newstream=false;
	}

}

//find the orderOut of a cell based on all the incoming orders
short newOrder(short nOrder[8], bool &junction, bool &source){
	short i,j,temp;
	short oOut=1;  //  initialize to 1
	short ordermax=0;
	short count=0;
	junction=false;
	source=true;
	//  DGT revised logic to be a bit more efficient avoiding sort (and following old code)
	for(i=0; i<8; i++)
	{
		if(nOrder[i]>0)  // Here an inflowing stream
		{
			count=count+1;
			source=false;
			if(count == 1) //  First inflowing stream
			{
				oOut=nOrder[i];
				ordermax=nOrder[i];
			}else  // Here count is > 1
			{
				if(nOrder[i]> oOut)
				{
					ordermax = nOrder[i];
					oOut=nOrder[i];
				}
				else if(nOrder[i]==oOut) oOut=ordermax+1;
				//  This logic to ensure that order is max of highest in or second highest in +1
				junction=true;
			}
		}
	}
	//sort the two largest to the bottom of the list
	/*  Dan's logic DGT skipped - the sort is not necessary 
	for(i=0;i<2;++i){
		for(j=0;j<7;++j){
			if(nOrder[j]>nOrder[j+1]){
				temp=nOrder[j];
				nOrder[j]=nOrder[j+1];
				nOrder[j+1]=temp;
			}
		}
	}
	junction = false;
	if(nOrder[6]>0)  // if 2 or more paths merge then the order of the 6th entry will be 1 or higher so flag as a junction
		junction=true;
	//if the last two have the same order, there is a junction, so bump the order
	if(nOrder[6]==nOrder[7]){
		//junction=true;
		oOut=nOrder[7]+1;
	}else{
		oOut=nOrder[7];
	}  */
	return oOut;
}





int dropan(char *areafile, char *dirfile, char *elevfile, char *ssafile, char *dropfile,
                           char* datasrc,char* lyrname,int uselyrname,int lyrno, float threshmin, float threshmax, int nthresh, int steptype,
                           float *threshopt)
{

	// MPI Init section
	MPI_Init(NULL,NULL);{
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)
	{
		printf("DropAnalysis version %s\n",TDVERSION);
		fflush(stdout);
	}

	// *** initialize thresholds array and directions table
	float s1,s2,s1sq,s2sq;
	float tempFloat;
	short tempShort;double tempdxc,tempdyc;
	bool finished;
	long n1,n2;
	bool optnotset=true;
	
	double begint = MPI_Wtime();

	//  *** initiate fssada grid partition from ssafile
	tiffIO ssa(ssafile, FLOAT_TYPE);  // DGT changed from short type
	long ssaTotalX = ssa.getTotalX();
	long ssaTotalY = ssa.getTotalY();
	double ssadxA = ssa.getdxA();
	double ssadyA = ssa.getdyA();
	OGRSpatialReferenceH hSRSRaster;
	hSRSRaster=ssa.getspatialref();
	
	if(rank==0)
		{
			float timeestimate=(2e-7*ssaTotalX*ssaTotalY*nthresh/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


	//Create partition and read data
	tdpartition *ssaData;
	ssaData = CreateNewPartition(ssa.getDatatype(), ssaTotalX, ssaTotalY, ssadxA, ssadyA, ssa.getNodata());
	int ssanx = ssaData->getnx();
	int ssany = ssaData->getny();
	int ssaxstart, ssaystart;  
	ssaData->localToGlobal(0, 0, ssaxstart, ssaystart);  
	ssaData->savedxdyc(ssa);
	ssa.read((long)ssaxstart, (long)ssaystart, (long)ssany, (long)ssanx, ssaData->getGridPointer());

	float ssadiag;
	//ssadiag=sqrt((ssadx*ssadx)+(ssady*ssady));
			
	//  *** initiate sdir grid partition from dirfile
	//printf("file %s\n",dirfile);
	tiffIO dir(dirfile, SHORT_TYPE);
	long dirTotalX = dir.getTotalX();
	long dirTotalY = dir.getTotalY();
	double dirdxA = dir.getdxA();
	double dirdyA = dir.getdyA();
	//printf("header read\n");
	//short ndv=*(short*) dir.getNodata();
	//printf("No data value %d",ndv);
	//Create partition and read data
	tdpartition *dirData;
	dirData = CreateNewPartition(dir.getDatatype(), dirTotalX, dirTotalY, dirdxA, dirdyA, dir.getNodata());
	int dirnx = dirData->getnx();
	int dirny = dirData->getny();
	int dirxstart, dirystart; 
	dirData->localToGlobal(0, 0, dirxstart, dirystart);
	dir.read((long)dirxstart, (long)dirystart, (long)dirny, (long)dirnx, dirData->getGridPointer());


	//  *** initiate Aread8 grid partition from areafile //DGT changed to float to be flexible for large areas and also to include cell size (at cost of some imprecision)
	tiffIO area(areafile, FLOAT_TYPE);
	long areaTotalX = area.getTotalX();
	long areaTotalY = area.getTotalY();
	double areadxA = area.getdxA();
	double areadyA = area.getdyA();
	//Create partition and read data
	tdpartition *areaData;
	areaData = CreateNewPartition(area.getDatatype(), areaTotalX, areaTotalY, areadxA, areadyA, area.getNodata());
	int areanx = areaData->getnx();
	int areany = areaData->getny();
	int areaxstart, areaystart;
	areaData->localToGlobal(0, 0, areaxstart, areaystart);
	area.read(areaxstart, areaystart, areany, areanx, areaData->getGridPointer());


	if(!dir.compareTiff(ssa)){
		printf("dir and ssa files not the same size. Exiting \n");
		MPI_Abort(MCW,4);
	}
	if(!ssa.compareTiff(area)){
		printf("ssa and area files not the same size. Exiting \n");
		MPI_Abort(MCW,4);
	}

//  Read outlets
	int nxy;
	double *xnode, *ynode;
	int i,j;
	
	if(rank==0){
		if(readoutlets(datasrc,lyrname,uselyrname,lyrno,hSRSRaster, &nxy, xnode, ynode)==0){
			MPI_Bcast(&nxy, 1, MPI_INT, 0, MCW);
			MPI_Bcast(xnode, nxy, MPI_DOUBLE, 0, MCW);
			MPI_Bcast(ynode, nxy, MPI_DOUBLE, 0, MCW);
		}
		else {
			printf("Error opening shapefile. Exiting \n");
			MPI_Abort(MCW,5);
		}
	}
	else {
		MPI_Bcast(&nxy, 1, MPI_INT, 0, MCW);
		xnode = new double[nxy];
		ynode = new double[nxy];
		MPI_Bcast(xnode, nxy, MPI_DOUBLE, 0, MCW);
		MPI_Bcast(ynode, nxy, MPI_DOUBLE, 0, MCW);
	}

	// ***  sum AreaD8 at each terminal outlet this gives total area of 
	// ***  the domain being processed

	float totalAreaProcessed; //total area of the domain being processed
	float ta;  //  DGT changed from long to float for consistency with total area processed
	int tx,ty;
	long nx,ny;
	short nd;
	float na;  //DGT changed from long to float because this has to operate on SSA
	int *outletsX, *outletsY;
	outletsX = new int[nxy];
	outletsY = new int[nxy];
	totalAreaProcessed=0;
	for( i=0; i<nxy; i++){
		area.geoToGlobalXY(xnode[i], ynode[i], outletsX[i], outletsY[i]);
		areaData->globalToLocal(outletsX[i], outletsY[i], tx, ty);
		if(areaData->isInPartition(tx,ty)){
			ta=areaData->getData((long) tx,(long) ty,ta);
		//only add terminal outlet areas
		// check: is terminal outlet iff downstream neighbor has area<=0
			nd = dirData->getData((long) tx,(long) ty,nd);
			//printf("Outlet direction: %d\n",nd);  //  DGT Concern that this may be no data
			nx = tx+d1[nd];
			ny = ty+d2[nd];
			if(ssaData->isNodata(nx,ny)  || (ssaData->getData(nx,ny,na)<=0))
			//na=ssaData->getData((long)nx,(long) ny,na);   //DGT changed to ssaData from areaData - end of stream determined from ssa
				totalAreaProcessed+=ta;
		}
	}
	MPI_Allreduce(&totalAreaProcessed,&ta,1,MPI_FLOAT,MPI_SUM,MCW);
	totalAreaProcessed = ta*areadxA*areadyA;
	// if(!rank)cout << "totalAreaProcessed = " << totalAreaProcessed << endl;

	// *** fareada no longer needed, so it's memory can be free'd up
	delete areaData;

	// *** instantiate felevg grid partition from elevfile
	tiffIO elev(elevfile, FLOAT_TYPE);
	long elevTotalX = elev.getTotalX();
	long elevTotalY = elev.getTotalY();
	double elevdxA = elev.getdxA();
	double elevdyA = elev.getdyA();
	//Create partition and read data
	tdpartition *elevData;
	elevData = CreateNewPartition(elev.getDatatype(), elevTotalX, elevTotalY, elevdxA, elevdyA, elev.getNodata());
	int elevnx = elevData->getnx();
	int elevny = elevData->getny();
	int elevxstart, elevystart;  
	elevData->localToGlobal(0, 0, elevxstart, elevystart);  
	elev.read((long)elevxstart, (long)elevystart, (long)elevny, (long)elevnx, elevData->getGridPointer());


	// compare to ssa size
	if(!elev.compareTiff(ssa)){
		printf("elev and ssa files not the same size. Exiting \n");
		MPI_Abort(MCW,5);
	}

	//  Make ssaData, dirData and elevData available to neighboring partitions once here at beginning as these are not changes
	ssaData->share();
	dirData->share();
	elevData->share();

	double readt = MPI_Wtime();

	// num thresholds must be greater than 1
	float thresh;
	if(nthresh<2){
		printf("Number of thresholds must be greater than 1. \n");
		MPI_Abort(MCW,7);
	}
	// *** loop over all thresholds
	int th;
	FILE *fp;
	//for(thresh=threshmin;thresh<=threshmax;thresh+=((threshmax-threshmin)/(float) (nthresh-1))){
	for(th=0;th<nthresh;++th){
		if(steptype==0){
			float r = exp((log(threshmax) - log(threshmin)) / (nthresh - 1));
			thresh = threshmin*pow(r,th);
		}else{
                	float delta = (threshmax - threshmin) / (nthresh - 1);
			thresh = threshmin + th * delta;
		}

		// *** instantiate arrays to hold results
		queue <node> que;
		node t;
		//Create partition for numcontributers
		tdpartition *contribs;
		tdpartition *orderOut;
		//tdpartition *length;
		tdpartition *elevOut;
		contribs = CreateNewPartition(SHORT_TYPE, ssaTotalX, ssaTotalY, ssadxA, ssadyA, MISSINGSHORT);
		//Create partitions for orderOut,length,elevOut
		orderOut = CreateNewPartition(SHORT_TYPE, ssaTotalX, ssaTotalY, ssadxA, ssadyA, MISSINGSHORT);
		//length = CreateNewPartition(FLOAT_TYPE, ssaTotalX, ssaTotalY, ssadx, ssady, MISSINGFLOAT);
		//  Dont need length partition - just accumulate in each partition
		double length=0.0;  // Double so as not to lose little bits when the number is big due to rounding
		elevOut = CreateNewPartition(FLOAT_TYPE, ssaTotalX, ssaTotalY, ssadxA, ssadyA, MISSINGFLOAT);

		s1=0.0;
		s2=0.0;
		s1sq=0.0;
		s2sq=0.0;
		n1=0;
		n2=0;

		// find the number of equal/above-threshold contributers for each cell
		long i,j,m;
		short d;	
		float s;	//DGT changed from short to float.  ssa is float
		long nx = ssanx;
		long ny = ssany;
		short k=0;
		long nexti,nextj;
		float e;
		// share borders
		contribs->clearBorders();   //  DGT changed from share
		for(j=0;j<=ny-1;++j){
			for(i=0;i<nx;++i){
				// each cells looks all around and counts the arrows pointing to it
				k=0;
				for(m=1;m<=8;++m){
					nexti=i+d1[m];
					nextj=j+d2[m];
					if(pointsToMe(i,j,nexti,nextj,dirData) && ssaData->getData(nexti,nextj,s)>=thresh)k++;
				}
				//  Only work with cells on current stream mask
				if(!ssaData->isNodata(i,j) && ssaData->getData(i,j,tempFloat)>=thresh)
					contribs->setData(i,j,k);
				// while we are at it, init data for each cell
				// init streamOrder for each cell to -1
				// DGT does not like this initialization here - they are initialized to no data upon creation
				//  and should be assigned values when they are processed
				//orderOut->setData(i,j,(short)-1);
				//length->setData(i,j,(float)0.0);
				//elevOut->setData(i,j,(float)0.0);
			}
		}

		// put the ones with no contributers onto the que for processing.
		for(j=0;j<ny;++j){
			for(i=0;i<nx;++i){
				if(!contribs->getData(i,j,k) && ssaData->getData(i,j,s)>=thresh){
					t.x=i;
					t.y=j;
					que.push(t);
				}
				// each initial cell gets streamOrder=1
				//  DGT commented out below - will be handled during processing
				//orderOut->setData(i,j,(short)1) ;
				//elevOut->setData(i,j,elevData->getData(i,j,e)) ;
			}
		}

		// each process empties its que, then shares border info, and repeats till everyone is done
	//	int globalwork=1; //int not bool for MPI_reduce
	//	int localwork=1; //int not bool for MPI_reduce
		//  DGT replaced with finished logic from other functions
		finished=false;
		while(!finished){
			//globalwork=0;
			//contribs->clearBorders();
			while(!que.empty()){
				t=que.front();
				que.pop();
				d=dirData->getData((long)t.x,(long)t.y,d);

				// begin cell processing
				long i,j,pi,pj,pd,k;
				short o;
				float e;
				i=t.x;
				j=t.y;
//				if(i==228 && j==0 && rank==1)
//					i=i;
				short nOrder[8];  // neighborOrders
				bool junction; // junction set to true/false in newOrder

				//put the order of all the neighboring cells into an array, and save the i,j, and elev from contributor.
				//pi,pj, and pd are useful only if there is no junction, where there is necessarily one and only
                                //one contributor.
				// TODO:  check against dropanrecursive function
				for(k=0;k<8;++k)nOrder[k]=0;
				//float l=0.0;
				//length->setData(i,j,l);  // Initialize length to 0
				for(m=1;m<=8;++m){
					nexti=i+d1[m];
					nextj=j+d2[m];
					if(pointsToMe(i,j,nexti,nextj,dirData) && !orderOut->isNodata(nexti,nextj)){
						nOrder[m-1]=orderOut->getData(nexti,nextj,o);
						//  Accumulate length
						pd=m;  //DGT changed from d to m to record direction of neighbor
						pi=nexti;
						pj=nextj;
						ssaData->getdxdyc(j,tempdxc,tempdyc);
						if(pd==1||pd==5)length=length+tempdxc;
						if(pd==3||pd==7)length=length+tempdyc;
						if(pd%2==0)length=length+sqrt(tempdxc*tempdxc+tempdyc*tempdyc);
					}
				}

				// Determine the order of this cell
				short oOut;
				bool source;
				oOut = newOrder(nOrder,junction,source);
				orderOut->setData(i,j,oOut);
				if(source){
					elevOut->setData(i,j,elevData->getData(i,j,e));
					//printf("Source(rank:th:i:j), %d, %d, %d, %d\n",rank,th,i,j);
					//fflush(stdout);
				}else if(!junction){
					// if not a junction, transfer elevOut
					elevOut->setData(i,j,elevOut->getData(pi,pj,e));
				}else{
					// if it is a junction, update global values
					//  oOut = orderOut->getData(i,j,oOut);  // DGT redundant - was just set above
					bool newstream=true;  // Flag to indicate whether the junction results in a new Strahler stream
					for(k=1;k<=8;++k){
						//  d=dirData->getData(i+d1[k],j+d2[k],d);  //DGT redundant and overwrites value of d that needed to be saved for below
						updateAtJunction(oOut,i,i+d1[k],j,j+d2[k],nx,ny,dirData,
						                 orderOut,elevOut,elevData,newstream,s1,s1sq,s2,s2sq,n1,n2);

					}
					if(newstream)  // Here all paths terminated at the junction so this is a new stream
					{
						elevOut->setData(i,j,elevData->getData(i,j,e));
					}
				}
				// end cell processing
//				if((i==227 && j==127) || (i==228 && j==127))
//					i=i;

				short c;
				long nextx,nexty;
				nextx=t.x+d1[d];
				nexty=t.y+d2[d];
				contribs->addToData(nextx,nexty,(short)-1);
				//Check if neighbor needs to be added to que
				if(elevOut->isInPartition(nextx,nexty) && contribs->getData(nextx, nexty, tempShort) == 0 ){
					t.x=nextx;
					t.y=nexty;
					que.push(t);
				}
			}
			//Pass information

			//any touched border cells have negative numbers in them.
			//push them back to their owners, add the negatives to local
			//cells, and put the zero cells on the local que.
			contribs->addBorders();
			
			//also push back the data for each border cell
			//length->share();
			elevOut->share();
			orderOut->share();
			//If this created a cell with no contributing neighbors, put it on the queue
			for(i=0; i<nx; i++){
//				if(i == 228)
//					i=i;
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

			contribs->clearBorders();
		
			//Check if done
			finished = que.empty();
			finished = orderOut->ringTerm(finished);
		}
	
		// *** calculate and write results
		MPI_Barrier(MCW);
		float gs1,gs2,gs1sq,gs2sq;
		double glen;
		int gn1,gn2;
		
		MPI_Reduce(&s1,&gs1,1,MPI_FLOAT,MPI_SUM,0,MCW);
		MPI_Reduce(&s2,&gs2,1,MPI_FLOAT,MPI_SUM,0,MCW);
		MPI_Reduce(&s1sq,&gs1sq,1,MPI_FLOAT,MPI_SUM,0,MCW);
		MPI_Reduce(&s2sq,&gs2sq,1,MPI_FLOAT,MPI_SUM,0,MCW);
		MPI_Reduce(&n1,&gn1,1,MPI_INT,MPI_SUM,0,MCW);
		MPI_Reduce(&n2,&gn2,1,MPI_INT,MPI_SUM,0,MCW);
		MPI_Reduce(&length,&glen,1,MPI_DOUBLE,MPI_SUM,0,MCW);
		float drainden=glen/totalAreaProcessed;

		if(!rank && th==0){
			cout << "Threshold";
			cout << " DrainDen";
			cout << " NoFirstOrd";
			cout << " NoHighOrd";
			cout << " MeanDFirstOrd";
			cout << " MeanDHighOrd";
			cout << " StdDevFirstOrd";
			cout << " StdDevHighOrd";
			cout << " Tval";
			cout << endl;
	
			fp = fopen(dropfile,"w");
			fprintf(fp,"Threshold, DrainDen, NoFirstOrd,NoHighOrd, MeanDFirstOrd, MeanDHighOrd, StdDevFirstOrd, StdDevHighOrd, T\n");

		}
		if(!rank){
			cout << setiosflags(ios::fixed) << setprecision(6) << thresh;
			cout << " ";
			cout << drainden;
			cout << " ";
			cout << gn1;
			cout << " ";
			cout << gn2;
			cout << " ";
			float md1 = gs1/gn1; 
			if(gn1>0)cout << md1; else cout << " - ";
			cout << " ";
			float mdh = gs2/gn2; 
			if(gn2>0)cout << mdh; else cout << " - ";
			cout << " ";
			float sd1 = sqrt((gs1sq-gn1*md1*md1)/(gn1-1)); 
			if(gn1 > 1)cout << sd1;  else cout << " - ";
			cout << " ";
			float sdh = sqrt((gs2sq-gn2*mdh*mdh)/(gn2-1)); 
			if(gn2 > 1) cout << sdh; else cout << " - ";
			cout << " ";
			float t = (md1-mdh)/(sqrt(((gn1-1)*sd1*sd1+(gn2-1)*sdh*sdh) / (gn1+gn2-2))*sqrt(1./gn1+1./gn2));
			if(gn2 > 1) cout << t; else cout << " - ";
			cout << endl;
			
			if(fabs(t) < 2. && optnotset){ // Find first occurrence of t value with absolute value less than 2.
					//  This is the optimum
				*threshopt=thresh;
				optnotset=false;
			}

			           //  write results
			if(gn1 > 1 && gn2 > 1)
			{
				fprintf(fp,"%f, ",thresh);
				fprintf(fp,"%e, ",drainden);
				fprintf(fp,"%d, ",gn1);
				fprintf(fp,"%d, ",gn2);
				fprintf(fp,"%f, ",md1);
				fprintf(fp,"%f, ",mdh);
				fprintf(fp,"%f, ",sd1);
				fprintf(fp,"%f, ",sdh);
				fprintf(fp,"%f\n",t);
			}
		}
		//  dgt freeing memory
		delete  contribs;
		delete  orderOut;
		delete  elevOut;
	}
	
	float topt = *threshopt;
	MPI_Bcast(&topt,1,MPI_FLOAT,0,MCW);
	*threshopt = topt;

	if(!rank)
	{
		printf("%f  Value for optimum that drop analysis selected - see output file for details.\n", *threshopt);
		//cout << "Optimum Threshold Value: " << *threshopt << endl;
		fprintf(fp,"Optimum Threshold Value: %f\n",*threshopt);
		fclose(fp);
	}
	float computeDropt = MPI_Wtime();
	double dataRead, compute, total,temp;
        dataRead = readt-begint;
        compute = computeDropt-readt;
        total = computeDropt - begint;

        MPI_Allreduce (&dataRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = temp/size;
        MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = temp/size;
        MPI_Allreduce (&total, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = temp/size;

        if( rank == 0)
                printf("Processes: %d\nRead time: %f\nCompute time: %f\nTotal time: %f\n",
                  size, dataRead, compute,total);
	
	// *** free memory and go home
	delete  ssaData;
	delete  dirData;
	delete  xnode;
	delete  ynode;
	delete  elevData;

	}MPI_Finalize();

	return 0;
}


