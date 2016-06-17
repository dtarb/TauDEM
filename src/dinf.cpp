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

#include "flow_direction.h"

struct Dinf {
    typedef float FlowType;

    static bool HasFlow(FlowType x) {
        return x != -1.0;
    }

    template<typename E>
    static void SetFlow(int x, int y, linearpart<FlowType>& flowDir, E& elev, SparsePartition<int>& inc) {
        double dxc, dyc;
        flowDir.getdxdyc(y, dxc, dyc);

        float DXX[3] = {0, (float) dxc, (float) dyc};
        float DD = sqrt(dxc*dxc+dyc*dyc);

        SET2(y, x, DXX, DD, elev, inc, flowDir);
    }
};

double **fact;

uint64_t setPosDirDinf(linearpart<float>& elevDEM, linearpart<float>& flowDir, linearpart<float>& slope, int useflowfile);
    
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
	
    linearpart<float> elevDEM(totalX, totalY, dxA, dyA, MPI_FLOAT, *(float *) dem.getNodata());

	int xstart, ystart;
	int nx = elevDEM.getnx();
	int ny = elevDEM.getny();
	elevDEM.localToGlobal(0, 0, xstart, ystart);
	elevDEM.savedxdyc(dem);
	double headert = MPI_Wtime();

	if(rank==0)
	{
		float timeestimate=(2.8e-9*pow((double)(totalX*totalY),1.55)/pow((double) size,0.65))/60+1;  // Time estimate in minutes
		//fprintf(stderr,"%d %d %d\n",totalX,totalY,size);
		fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
		fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
		fflush(stderr);
	}

	dem.read(xstart, ystart, ny, nx, elevDEM.getGridPointer());
	elevDEM.share();

	double readt = MPI_Wtime();
	
	//Creates empty partition to store new flow direction 
	linearpart<float> flowDir(totalX, totalY, dxA, dyA, MPI_FLOAT, MISSINGFLOAT);
    flowDir.savedxdyc(dem);

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
        float slopeNodata = -1.0f;

        linearpart<float> slope(totalX, totalY, dxA, dyA, MPI_FLOAT, slopeNodata);
        numFlat = setPosDirDinf(elevDEM, flowDir, slope, useflowfile);

        //Stop timer
        computeSlopet = MPI_Wtime();
        tiffIO slopeIO(slopefile, FLOAT_TYPE, &slopeNodata, dem);
        slopeIO.write(xstart, ystart, ny, nx, slope.getGridPointer());
    }  // This bracket intended to destruct slope partition and release memory

    flowDir.share();

	double writeSlopet = MPI_Wtime();

	MPI_Allreduce(&numFlat,&totalNumFlat,1,MPI_LONG,MPI_SUM,MCW);
	if(rank==0)
	{
		fprintf(stderr,"All slopes evaluated. %ld flats to resolve.\n",totalNumFlat);
		fflush(stderr);
	}

    vector<vector<node>> localIslands, sharedIslands;
    findIslands<Dinf>(flowDir, localIslands, sharedIslands); 

    uint64_t localSharedFlats = 0, sharedFlats = 0;
    for (auto& island : sharedIslands) {
        localSharedFlats += island.size();
    }

    //t.start("Resolve shared flats");
    MPI_Allreduce(&localSharedFlats, &sharedFlats, 1, MPI_UINT64_T, MPI_SUM, MCW);

    if (rank == 0 && size > 1) {
        fprintf(stderr, "Processing partial flats\n");

        printf("PRL: %llu flats shared across processors (%llu local -> %.2f%% shared)\n",
                sharedFlats, totalNumFlat - sharedFlats, 100. * sharedFlats / totalNumFlat);
    }

    if (sharedFlats > 0) {
        SparsePartition<int> inc(nx, ny, 0);
        size_t lastNumFlat = resolveFlats_parallel_async<Dinf>(elevDEM, inc, flowDir, sharedIslands);

        if (rank==0) {
            fprintf(stderr, "PRL: Iteration complete. Number of flats remaining: %zu\n", lastNumFlat);
            fflush(stderr);
        }

        // Repeatedly call resolve flats until there is no change across all processors
        while (lastNumFlat > 0) {
            SparsePartition<int> newInc(nx, ny, 0);

            lastNumFlat = resolveFlats_parallel_async<Dinf>(inc, newInc, flowDir, sharedIslands);
            inc = std::move(newInc);

            if (rank==0) {
                fprintf(stderr, "PRL: Iteration complete. Number of flats remaining: %zu\n", lastNumFlat);
                fflush(stderr);
            }
        }
    }
    //t.end("Resolve shared flats");

    if (!localIslands.empty()) {
        SparsePartition<int> inc(nx, ny, 0);
        size_t lastNumFlat = resolveFlats<Dinf>(elevDEM, inc, flowDir, localIslands);

        if (rank==0) {
            fprintf(stderr, "Iteration complete. Number of flats remaining: %zu\n\n", lastNumFlat);
            fflush(stderr);
        }

        // Repeatedly call resolve flats until there is no change
        while (lastNumFlat > 0)
        {
            SparsePartition<int> newInc(nx, ny, 0);

            lastNumFlat = resolveFlats<Dinf>(inc, newInc, flowDir, localIslands); 
            inc = std::move(newInc);

            if (rank==0) {
                fprintf(stderr, "Iteration complete. Number of flats remaining: %zu\n\n", lastNumFlat);
                fflush(stderr);
            }
        } 
    }

	//Timing info
	double computeFlatt = MPI_Wtime();
	
	float flowDirNodata=MISSINGFLOAT;
	tiffIO flowIO(angfile, FLOAT_TYPE, &flowDirNodata, dem);
	flowIO.write(xstart, ystart, ny, nx, flowDir.getGridPointer());

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

    if (rank == 0)
        //  These times are only for  process 0 - not averaged across processors.  This may be an approximation - but probably do not want to hold processes up to synchronize just so as to get more accurate timing
        printf("Processors: %d\nHeader read time: %f\nData read time: %f\nCompute Slope time: %f\nWrite Slope time: %f\nResolve Flat time: %f\nWrite Flat time: %f\nTotal time: %f\n",
                size,headerRead,dataRead, computeSlope, writeSlope,computeFlat,writeFlat,total);

    }
	MPI_Finalize();
	return 0;
}

// SUBROUTINE TO RETURN THE SLOPE AND ANGLE ASSOCIATED WITH A DEM PANEL 
void VSLOPE(float E0,float E1, float E2, float D1, float D2, float DD,
    float& S,float& A)
{
    float S1, S2;

    if(D1!=0)
        S1=(E0-E1)/D1;

    if(D2!=0)
        S2=(E1-E2)/D2;

    if(S2==0 && S1==0)
        A = 0;
    else
        A= (float) atan2(S2,S1);

    float AD = atan2(D2,D1);
    if(A < 0.)
    {
        A = 0.;
        S = S1;
    }
    else if(A > AD)
    {
        A = AD;
        S = (E0-E2)/DD;
    } else {
        S = sqrt(S1*S1+S2*S2);
    }
}

// Sets only flowDir only where there is a positive slope
// Returns number of cells which are flat
void SET2(int I, int J,float DXX[3], float DD, linearpart<float>& elevDEM, linearpart<float>& flowDir, linearpart<float>& slope)
{
	const int ID1[]= {0,1,2,2,1,1,2,2,1 }; 
	const int ID2[]= {0,2,1,1,2,2,1,1,2};
	const int I1[] = {0,0,-1,-1,0,0,1,1,0 };
	const int I2[] = {0,-1,-1,-1,-1,1,1,1,1};
	const int J1[] = {0,1,0,0,-1,-1,0,0,1};
	const int J2[] = {0,1,1,-1,-1,-1,-1,1,1};
	const float ANGC[] = {0,0.,1.,1.,2.,2.,3.,3.,4.};
	const float ANGF[] = {0,1.,-1.,1.,-1.,1.,-1.,1.,-1.};

    float SK[9] = {0};
	float ANGLE[9] = {0};

	for(int K=1; K<=8; K++)
	{
		VSLOPE(
			elevDEM.getData(J,I),//felevg.d[J][I],
			elevDEM.getData(J+J1[K],I+I1[K]),//[felevg.d[J+J1[K]][I+I1[K]],
			elevDEM.getData(J+J2[K],I+I2[K]),//felevg.d[J+J2[K]][I+I2[K]],
			DXX[ID1[K]],
			DXX[ID2[K]],
			DD,
			SK[K],
			ANGLE[K]
		);
	}

	float SMAX = 0.;
	int KD=0;
	flowDir.setData(J,I, -1.0); //USE -1 TO INDICATE DIRECTION NOT YET SET

	for(int K=1; K<=8; K++)
	{
		if(SK[K] > SMAX)
		{
			SMAX=SK[K];
			KD=K;
		}
	}

	if(KD > 0)
	{
		float flowAngle = ANGC[KD]*(PI/2)+ANGF[KD]*ANGLE[KD];
		flowDir.setData(J,I, flowAngle); //set to angle
	}
	slope.setData(J, I, SMAX);
}

//Overloaded SET2 for use in resolve flats when slope is no longer recorded.  Also uses artificial elevations and actual elevations
template<typename T>
void SET2(int I, int J,float *DXX,float DD, T& elevDEM, SparsePartition<int>& inc, linearpart<float>& flowDir)
{
    const int ID1[]= {0,1,2,2,1,1,2,2,1 }; 
	const int ID2[]= {0,2,1,1,2,2,1,1,2};
	const int I1[] = {0,0,-1,-1,0,0,1,1,0 };
	const int I2[] = {0,-1,-1,-1,-1,1,1,1,1};
	const int J1[] = {0,1,0,0,-1,-1,0,0,1};
	const int J2[] = {0,1,1,-1,-1,-1,-1,1,1};
	const float  ANGC[]={0,0.,1.,1.,2.,2.,3.,3.,4.};
	const float  ANGF[]={0,1.,-1.,1.,-1.,1.,-1.,1.,-1.};

	float SK[9] = {0};
	float ANGLE[9] = {0};
	float SMAX=0.0;
	short tempShort, tempShort1, tempShort2;
	int KD=0;

	bool diagOutFound=false;

	for(int K=1; K<=8; K++)
	{
        if (!flowDir.hasAccess(J + J1[K], I+I1[K])) {
            continue;
        }
        
        if (!flowDir.hasAccess(J + J2[K], I+I2[K])) {
            continue;
        }

        tempShort1 = inc.getData(J+J1[K], I+I1[K]) > 0;
        tempShort2 = inc.getData(J+J2[K], I+I2[K]) > 0;

		if(tempShort1 <= 0 && tempShort2 <= 0) { //Both E1 and E2 are outside the flat get slope and angle
			float a=elevDEM.getData(J,I);
			float b=elevDEM.getData(J+J1[K],I+I1[K]);
			float c=elevDEM.getData(J+J2[K],I+I2[K]);

			VSLOPE(
				a,//E0
				b,//E1
				c,//E2
				DXX[ID1[K]],//dx or dy depending on ID1
				DXX[ID2[K]],//dx or dy depending on ID2
				DD,//Hypotenuse
				SK[K],//Slope Returned
				ANGLE[K]//Angle Returned
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
			float a=elevDEM.getData(J,I);
			float b=elevDEM.getData(J+J1[K],I+I1[K]);

			if(a>=b)
			{
				ANGLE[K]=0.0;
				SK[K]=0.0;
				KD=K;
				break;
			}
			int a1=inc.getData(J,I);
			int c1=inc.getData(J+J2[K],I+I2[K]);
			int b1=std::max(a1,c1);
			VSLOPE(
				(float)a1,//felevg.d[J][I],
				(float)b1,//[felevg.d[J+J1[K]][I+I1[K]],
				(float)c1,//felevg.d[J+J2[K]][I+I2[K]],
				DXX[ID1[K]],//dx or dy
				DXX[ID2[K]],//dx or dy
				DD,//Hypotenuse
				SK[K],//Slope Returned
				ANGLE[K]//Angle Reutnred
			);
			if(SK[K]>SMAX){
				SMAX=SK[K];
				KD=K;
			}
		}else if(tempShort1 > 0 && tempShort2 <= 0){//E2 is out side of the flat and E1 is inside the flat, use DEM elevations
			float a=elevDEM.getData(J,I);
			//float b=elevDEM->getData(J+J1[K],I+I1[K],tempFloat);
			float c=elevDEM.getData(J+J2[K],I+I2[K]);
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
				int a1=inc.getData(J,I);
				int b1=inc.getData(J+J1[K],I+I1[K]);
				int c1=std::max(a1,b1);

				VSLOPE(
					(float)a1,//felevg.d[J][I],
					(float)b1,//[felevg.d[J+J1[K]][I+I1[K]],
					(float)c1,//felevg.d[J+J2[K]][I+I2[K]],
					DXX[ID1[K]],//dx or dy
					DXX[ID2[K]],//dx or dy
					DD,//Hypotenuse
					SK[K],//Slope Returned
					ANGLE[K]//Angle Reutnred
				);
				if(SK[K]>SMAX){
					SMAX=SK[K];
					KD=K;
				}

			}
		}else{//Both E1 and E2 are in the flat. Use artificial elevation to get slope and angle
			int a, b, c;
			a = inc.getData(J,I);
			b = inc.getData(J+J1[K],I+I1[K]);
			c = inc.getData(J+J2[K],I+I2[K]);
			VSLOPE(
				(float)a,//felevg.d[J][I],
				(float)b,//[felevg.d[J+J1[K]][I+I1[K]],
				(float)c,//felevg.d[J+J2[K]][I+I2[K]],
				DXX[ID1[K]],//dx or dy
				DXX[ID2[K]],//dx or dy
				DD,//Hypotenuse
				SK[K],//Slope Returned
				ANGLE[K]//Angle Reutnred
			);
			if(SK[K]>SMAX){
				SMAX=SK[K];
				KD=K;
			}
		}
	}

	//USE -1 TO INDICATE DIRECTION NOT YET SET, 
	// but only for non pit grid cells.  Pits will have flowDir as no data
	if(!flowDir.isNodata(J,I))
	{
		flowDir.setData(J,I, -1.0);  
	}

    // We have a flow direction.
	if (KD > 0)
	{
	    // Calculate the angle
		float finalAngle = ANGC[KD]*(PI/2) + ANGF[KD]*ANGLE[KD];

		if(finalAngle >= 0.0) {
			flowDir.setData(J, I, finalAngle);
        }
	}
}

uint64_t setPosDirDinf(linearpart<float>& elevDEM, linearpart<float>& flowDir, linearpart<float>& slope, int useflowfile) {
    int nx = elevDEM.getnx();
    int ny = elevDEM.getny();
    uint64_t numFlat = 0;

    for(int j = 0; j < ny; j++) {
        for(int i=0; i < nx; i++) {
            // FlowDir is nodata if it is on the border OR elevDEM has no data
            if (elevDEM.isNodata(i,j) || !elevDEM.hasAccess(i-1,j) || !elevDEM.hasAccess(i+1,j) || 
                    !elevDEM.hasAccess(i,j-1) || !elevDEM.hasAccess(i,j+1) )  {
                //do nothing			
                continue;
            }

            // Check if cell is "contaminated" (neighbors have no data)
            // set flowDir to noData if contaminated
            bool contaminated = false;
            for(int k=1; k<=8; k++) {
                int in=i+d1[k];
                int jn=j+d2[k];

                if (elevDEM.isNodata(in,jn)) {
                    flowDir.setToNodata(i, j);
                    contaminated = true;
                    break;
                }
            }

            if (contaminated) continue;

            //If cell is not contaminated,
            flowDir.setData(i, j, -1.0);

            double dxc, dyc;
            elevDEM.getdxdyc(j, dxc, dyc);

            float DXX[3] = {0, (float) dxc, (float) dyc};
            float DD = sqrt(dxc*dxc+dyc*dyc);

            SET2(j,i,DXX,DD, elevDEM, flowDir, slope);//i=y in function form old code j is x switched on purpose
            //  Use SET2 from serial code here modified to get what it has as felevg.d from elevDEM partition
            //  Modify to return 0 if there is a 0 slope.  Modify SET2 to output flowDIR as no data (do nothing 
            //  if verified initialization to nodata) and 
            //  slope as 0 if a positive slope is not found

            if (flowDir.getData(i,j) == -1.0)
                numFlat++;
        }
    }

    return numFlat;
}
