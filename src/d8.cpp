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

#include <algorithm>
#include <cinttypes>
#include <set>

#include <mpi.h>

#include "d8.h"
#include "linearpart.h"
#include "commonLib.h"
#include "tiffIO.h"
#include "Node.h"

#include "mpitimer.h"

#include "flow_direction.h"

double **fact;

struct D8 {
    typedef short FlowType;

    static bool HasFlow(FlowType x) {
        return x != 0;
    }

    template<typename E>
    static void SetFlow(int x, int y, linearpart<FlowType>& flowDir, E& elev, SparsePartition<int>& inc) {
        setFlow2(x, y, flowDir, elev, inc);
    }
};

//Set positive flowdirections of elevDEM
void setFlow(int i, int j, linearpart<short>& flowDir, linearpart<float>& elevDEM)
{
    int in,jn;
    int amax=0;
    float smax=0;

    float elev = elevDEM.getData(i, j);

    for (short k=1; k<=8 && !flowDir.isNodata(i,j); k+=2) {
        in=i+d1[k];
        jn=j+d2[k];

        float slope = fact[j][k] * (elev - elevDEM.getData(in,jn));

        if (slope > smax) {
            smax=slope;
            short dirnb=flowDir.getData(in,jn);

            if (dirnb > 0 && abs(dirnb-k) == 4) {
                flowDir.setToNodata(i,j);
            } else {
                flowDir.setData(i,j,k);
            }
        }
    }

    for (short k=2; k<=8 && !flowDir.isNodata(i,j); k+=2) {
        in=i+d1[k];
        jn=j+d2[k];

        float slope = fact[j][k] * (elev - elevDEM.getData(in,jn));

        if (slope > smax && !cellsCross(k,i,j,flowDir)) {
            smax = slope;
            short dirnb = flowDir.getData(in,jn);

            if (dirnb > 0 && abs(dirnb-k) == 4) { 
                flowDir.setToNodata(i,j);
            } else {
                flowDir.setData(i,j,k);
            }
        }
    }
}

//Calculate the slope information of flowDir to slope
void calcSlope(linearpart<short>& flowDir, linearpart<float>& elevDEM, linearpart<float>& slope)
{
    int nx = elevDEM.getnx();
    int ny = elevDEM.getny();

    for (int j=0; j < ny; j++) {
        for (int i=0; i < nx; i++) {
            // If i,j is on the border or flowDir has no data, set slope(i,j) to slopeNoData
            if (flowDir.isNodata(i,j) || !flowDir.hasAccess(i-1,j) || !flowDir.hasAccess(i+1,j) ||
                    !flowDir.hasAccess(i,j-1) || !flowDir.hasAccess(i,j+1)) {
                slope.setToNodata(i, j);
            } else {
                short flowDirection = flowDir.getData(i,j);
  
                int in = i + d1[flowDirection];
                int jn = j + d2[flowDirection];

                float elevDiff = elevDEM.getData(i,j) - elevDEM.getData(in,jn);
                slope.setData(i,j, elevDiff*fact[j][flowDirection]);
            }
        }
    }
}

//Open files, Initialize grid memory, makes function calls to set flowDir, slope, and resolvflats, writes files
int setdird8(char* demfile, char* pointfile, char *slopefile, char *flowfile, int useflowfile)
{
    MPI_Init(NULL,NULL);

    int rank,size;
    MPI_Comm_rank(MCW,&rank);
    MPI_Comm_size(MCW,&size);

    if (rank==0) {
        printf("D8FlowDir version %s\n",TDVERSION);

        if (strlen(pointfile) == 0) {
            printf("WARNING: no output p file specified\n");
        }

        if (strlen(slopefile) == 0) {
            printf("WARNING: no output sd8 file specified\n");
        }

        fflush(stdout);
    }

    MPITimer t;

    t.start("Total");
    t.start("Header read");

    //Read DEM from file
    tiffIO dem(demfile, FLOAT_TYPE);

    long totalX = dem.getTotalX();
    long totalY = dem.getTotalY();
    double dx = dem.getdxA();
    double dy = dem.getdyA();

    linearpart<float> elevDEM(totalX, totalY, dx, dy, MPI_FLOAT, *(float*) dem.getNodata());

    int xstart, ystart;
    int nx = elevDEM.getnx();
    int ny = elevDEM.getny();
    elevDEM.localToGlobal(0, 0, xstart, ystart);
    elevDEM.savedxdyc(dem);

    t.end("Header read");

    if (rank==0) {
        float timeestimate=(2.8e-9*pow((double)(totalX*totalY),1.55)/pow((double) size,0.65))/60+1;  // Time estimate in minutes
        //fprintf(stderr,"%d %d %d\n",totalX,totalY,size);
        fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
        fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
        fflush(stderr);
    }

    uint64_t bytes_to_read = (uint64_t) nx * ny * sizeof(float);
    if (rank == 0) { 
        fprintf(stderr, "Reading input data (%s)... ", humanReadableSize(bytes_to_read).c_str());
    }

    t.start("Data read");

    dem.read(xstart, ystart, ny, nx, elevDEM.getGridPointer());
    elevDEM.share();
    double data_read_time = t.end("Data read");
   
    if (rank == 0) {
        fprintf(stderr, "done (%s/s).\n", humanReadableSize(bytes_to_read / data_read_time).c_str());
    }

    //Creates empty partition to store new flow direction
    short flowDirNodata = MISSINGSHORT;

    linearpart<short> flowDir(totalX, totalY, dx, dy, MPI_SHORT, flowDirNodata);

    //If using a flowfile, read it in
    if (useflowfile == 1) {
        tiffIO flow(flowfile, SHORT_TYPE);

        linearpart<short> imposedflow(flow.getTotalX(), flow.getTotalY(),
                flow.getdxA(), flow.getdyA(), MPI_SHORT, *(short*) flow.getNodata());

        if (!dem.compareTiff(flow)) {
            printf("Error using imposed flow file.\n");
            return 1;
        }

        for (int j=0; j < elevDEM.getny(); j++) {
            for (int i=0; i < elevDEM.getnx(); i++ ) {
                short data = imposedflow.getData(i,j);

                if (imposedflow.isNodata(i,j) || !imposedflow.hasAccess(i-1,j) || !imposedflow.hasAccess(i+1,j) ||
                        !imposedflow.hasAccess(i,j-1) || !imposedflow.hasAccess(i,j+1)) {
                    //Do nothing
                } else if (data > 0 && data <= 8) {
                    flowDir.setData(i,j,data);
                }
            }
        }
    }

    if (rank == 0) fprintf(stderr, "Calculating flow directions... ");
    t.start("Calculate flow directions");
    uint64_t numFlat = setPosDir(elevDEM, flowDir);
    t.end("Calculate flow directions");

    if (strlen(slopefile) > 0)
    {
        t.start("Calculate slope");
        
        //Creates empty partition to store new slopes
        float slopeNodata = -1.0f;
        linearpart<float> slope(totalX, totalY, dx, dy, MPI_FLOAT, slopeNodata);

        calcSlope(flowDir, elevDEM, slope);

        t.end("Calculate slope");

        t.start("Write slope");
        tiffIO slopeIO(slopefile, FLOAT_TYPE, &slopeNodata, dem);
        slopeIO.write(xstart, ystart, ny, nx, slope.getGridPointer());
        t.end("Write slope");
    }

    flowDir.share();

    uint64_t totalNumFlat = 0;
    MPI_Allreduce(&numFlat, &totalNumFlat, 1, MPI_UINT64_T, MPI_SUM, MCW);
   
    if (rank == 0) {
        fprintf(stderr, "done. %" PRIu64 " flats to resolve.\n", totalNumFlat);
        fflush(stderr);
    }

    t.start("Resolve flats");

    if (totalNumFlat > 0) {
        if (rank == 0) {
            fprintf(stderr, "Finding flat islands...\n");
        }

        std::vector<std::vector<node>> islands;
        std::vector<std::vector<node>> borderingIslands;

        t.start("Find islands");
        findIslands<D8>(flowDir, islands, borderingIslands);
        t.end("Find islands");

        uint64_t localSharedFlats = 0, sharedFlats = 0;
        for (auto& island : borderingIslands) {
            localSharedFlats += island.size();
        }

        t.start("Resolve shared flats");
        MPI_Allreduce(&localSharedFlats, &sharedFlats, 1, MPI_UINT64_T, MPI_SUM, MCW);

        if (rank == 0 && size > 1) {
            fprintf(stderr, "Processing partial flats\n");

            printf("PRL: %llu flats shared across processors (%llu local -> %.2f%% shared)\n",
                    sharedFlats, totalNumFlat - sharedFlats, 100. * sharedFlats / totalNumFlat);
        }

        if (sharedFlats > 0) {
            SparsePartition<int> inc(nx, ny, 0);
            size_t lastNumFlat = resolveFlats_parallel_async<D8>(elevDEM, inc, flowDir, borderingIslands);

            if (rank==0) {
                fprintf(stderr, "PRL: Iteration complete. Number of flats remaining: %zu\n", lastNumFlat);
                fflush(stderr);
            }

            // Repeatedly call resolve flats until there is no change across all processors
            while (lastNumFlat > 0) {
                SparsePartition<int> newInc(nx, ny, 0);

                lastNumFlat = resolveFlats_parallel_async<D8>(inc, newInc, flowDir, borderingIslands);
                inc = std::move(newInc);

                if (rank==0) {
                    fprintf(stderr, "PRL: Iteration complete. Number of flats remaining: %zu\n", lastNumFlat);
                    fflush(stderr);
                }
            }
        }
        t.end("Resolve shared flats");

        //printf("rank %d: Done, %d islands. Took %.2f seconds\n", rank, numIslands, MPI_Wtime() - flatFindStart);
        //printf("rank %d: %lu bordering islands with %d flats\n", rank, bordering_islands.size(), localSharedFlats);

        t.start("Resolve local flats");
        if (!islands.empty()) {
            SparsePartition<int> inc(nx, ny, 0);
            size_t lastNumFlat = resolveFlats<D8>(elevDEM, inc, flowDir, islands);

            if (rank==0) {
                fprintf(stderr, "Iteration complete. Number of flats remaining: %zu\n\n", lastNumFlat);
                fflush(stderr);
            }

            // Repeatedly call resolve flats until there is no change
            while (lastNumFlat > 0)
            {
                SparsePartition<int> newInc(nx, ny, 0);

                lastNumFlat = resolveFlats<D8>(inc, newInc, flowDir, islands); 
                inc = std::move(newInc);

                if (rank==0) {
                    fprintf(stderr, "Iteration complete. Number of flats remaining: %zu\n\n", lastNumFlat);
                    fflush(stderr);
                }
            } 
        }
        t.end("Resolve local flats");
    }

    t.end("Resolve flats");

    if (strlen(pointfile) > 0) {
        t.start("Write directions");
        tiffIO pointIO(pointfile, SHORT_TYPE, &flowDirNodata, dem);
        pointIO.write(xstart, ystart, ny, nx, flowDir.getGridPointer());
        t.end("Write directions");
    }

    t.end("Total");
    t.stop();
    //t.save("timing_info");

    MPI_Finalize();
    return 0;
}

// Sets only flowDir only where there is a positive slope
// Returns number of cells which are flat
long setPosDir(linearpart<float>& elevDEM, linearpart<short>& flowDir)
{
    double dxA = elevDEM.getdxA();
    double dyA = elevDEM.getdyA();
    long nx = elevDEM.getnx();
    long ny = elevDEM.getny();
    long numFlat = 0;

    double tempdxc, tempdyc;

    fact = new double*[ny]; // initialize 2d array by Nazmus 2/16

    for(int m = 0; m<ny; m++)
        fact[m] = new double[9];

    for (int m = 0; m<ny; m++) {
        for (int k = 1; k <= 8; k++) {
            elevDEM.getdxdyc(m, tempdxc, tempdyc);
            fact[m][k] = (double) (1./sqrt(d1[k]*d1[k]*tempdxc*tempdxc + d2[k]*d2[k]*tempdyc*tempdyc));
        }
    }

    for (int j = 0; j < ny; j++) {
        for (int i=0; i < nx; i++ ) {
            //FlowDir is nodata if it is on the border OR elevDEM has no data
            if (elevDEM.isNodata(i,j) || !elevDEM.hasAccess(i-1,j) || !elevDEM.hasAccess(i+1,j) ||
                    !elevDEM.hasAccess(i,j-1) || !elevDEM.hasAccess(i,j+1)) {
                //do nothing
                continue;
            }

            //Check if cell is "contaminated" (neighbors have no data)
            //  set flowDir to noData if contaminated
            bool contaminated = false;
            for (int k=1; k<=8; k++) {
                int in=i+d1[k];
                int jn=j+d2[k];

                if (elevDEM.isNodata(in,jn)) {
                    contaminated = true;
                    break;
                }
            }

            if (contaminated) {
                flowDir.setToNodata(i,j);
            } else {
                // If cell is not contaminated,
                flowDir.setData(i, j, 0);
                setFlow(i,j, flowDir, elevDEM);

                if (flowDir.getData(i,j) == 0) {
                    numFlat++;
                }
            }
        }
    }

    return numFlat;
}

// Function to set flow direction based on incremented artificial elevations
template<typename T>
void setFlow2(int i, int j, linearpart<short>& flowDir, T& elev, SparsePartition<int>& inc)
{
    /*  This function sets directions based upon secondary elevations for
      assignment of flow directions across flats according to Garbrecht and Martz
      scheme.  There are two possibilities:
    	A.  The neighbor is outside the flat set
    	B.  The neighbor is in the flat set.
    	In the case of A the input elevations are used and if a draining neighbor is found it is selected.
    	Case B requires slope to be positive.  Remaining flats are removed by iterating this process
    */
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    const short order[8]= {1,3,5,7,2,4,6,8};

    float slopeMax = 0;

    for (short k : order) {
        int in = i+d1[k];
        int jn = j+d2[k];

        if (!flowDir.hasAccess(in, jn))
            continue;

        if (inc.getData(in, jn) > 0) {
            // Neighbor is in flat
            float slope = fact[j][k]*(inc.getData(i, j) - inc.getData(in, jn));

            if (slope > slopeMax) {
                flowDir.setData(i, j, k);
                slopeMax = slope;
            }
        } else {
            // Neighbor is not in flat
            auto ed = elev.getData(i, j) - elev.getData(in, jn);

            if (ed >= 0) {
                // Found a way out - this is outlet
                flowDir.setData(i, j, k);
                break;  
            }
        }
    }
}

//************************************************************************
