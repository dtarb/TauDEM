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
#include <set>

#include <mpi.h>

#include "d8.h"
#include "linearpart.h"
#include "createpart.h"
#include "commonLib.h"
#include "tiffIO.h"
#include "Node.h"

#include "mpitimer.h"

template<typename T> long resolveFlats(T& elev, SparsePartition<short>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands);
template<typename T> long resolveFlats_parallel(T& elevDEM, SparsePartition<short>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands);

template<typename T> void flowTowardsLower(T& elev, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<short>& inc);
template<typename T> void flowFromHigher(T& elevDEM, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<short>& inc);
template<typename T> int markPits(T& elevDEM, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<short>& inc);

int propagateIncrements(linearpart<short>& flowDir, SparsePartition<short>& inc, std::vector<node>& queue);
int propagateBorderIncrements(linearpart<short>& flowDir, SparsePartition<short>& inc);

double **fact;

//Checks if cells cross
int dontCross(int k, int i, int j, linearpart<short>& flowDir)
{
    long in1, jn1, in2, jn2;
    int n1, c1, n2, c2;

    switch(k) {
    case 2:
        n1=1;
        c1=4;
        n2=3;
        c2=8;
        break;
    case 4:
        n1=3;
        c1=6;
        n2=5;
        c2=2;
        break;
    case 6:
        n1=7;
        c1=4;
        n2=5;
        c2=8;
        break;
    case 8:
        n1=1;
        c1=6;
        n2=7;
        c2=2;
        break;
    default:
        return 0;
    }

    in1=i+d1[n1];
    jn1=j+d2[n1];
    in2=i+d1[n2];
    jn2=j+d2[n2];

    if (flowDir.getData(in1,jn1) == c1 || flowDir.getData(in2,jn2) == c2)
    {
        return 1;
    }

    return 0;
}

//Set positive flowdirections of elevDEM
void setFlow(int i, int j, linearpart<short>& flowDir, linearpart<float>& elevDEM, SparsePartition<long>& area, int useflowfile)
{
    int in,jn;
    int amax=0;
    float smax=0;

    float elev = elevDEM.getData(i, j);

    for (short k=1; k<=8 && !flowDir.isNodata(i,j); k+=2) {
        in=i+d1[k];
        jn=j+d2[k];

        float slope = fact[j][k] * (elev - elevDEM.getData(in,jn));

        if (useflowfile == 1) {
            int aneigh = area.getData(in,jn);

            if (aneigh > amax && slope >= 0) {
                amax = aneigh;

                short dirnb = flowDir.getData(in,jn);
                if (dirnb > 0 && abs(dirnb - k) != 4) {
                    flowDir.setData(i, j, k);
                }
            }
        }

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

        if (slope > smax && dontCross(k,i,j,flowDir)==0) {
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
        fflush(stdout);
    }

    MPITimer t;

    double begint = MPI_Wtime();

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

    double headert = MPI_Wtime();

    if (rank==0) {
        float timeestimate=(2.8e-9*pow((double)(totalX*totalY),1.55)/pow((double) size,0.65))/60+1;  // Time estimate in minutes
        //fprintf(stderr,"%d %d %d\n",totalX,totalY,size);
        fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
        fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
        fflush(stderr);
    }

    auto bytes_to_read = nx * ny * sizeof(float);
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
    //linearpart<long> area(totalX, totalY, dx, dy, MPI_LONG, -1);
    SparsePartition<long> area(totalX, totalY, -1);

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

        //TODO - why is this here?
        //darea( &flowDir, &area, NULL, NULL, 0, 1, NULL, 0, 0 );
    }

    if (rank == 0) fprintf(stderr, "Calculating flow directions... ");
    t.start("Calculate flow directions");

    long numFlat = setPosDir(elevDEM, flowDir, area, useflowfile);

    t.end("Calculate flow directions");
    if (rank == 0) fprintf(stderr, "done. %lu flats to resolve.\n", numFlat);

    if (slopefile != NULL)
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

    size_t totalNumFlat = 0;
    MPI_Allreduce(&numFlat, &totalNumFlat, 1, MPI_LONG, MPI_SUM, MCW);
   
    if (rank == 0) {
        fprintf(stderr, "done. %ld flats to resolve.\n", totalNumFlat);
        fflush(stderr);
    }

    t.start("Resolve flats");

    if (totalNumFlat > 0) {
        std::vector<node> flats;

        t.start("Add flats");

        // FIXME: Should do this during slope calculation
        for (int j=0; j<ny; j++) {
            for (int i=0; i<nx; i++) {
                if (flowDir.getData(i,j)==0) {
                    flats.push_back(node(i, j));
                }
            }
        }

        t.end("Add flats");

        if (rank == 0) {
            fprintf(stderr, "Finding flat islands...\n");
        }

        double flatFindStart = MPI_Wtime();
        int numIslands = 0;

        std::vector<std::vector<node>> islands;
        std::set<int> bordering_island_labels;

        t.start("Find islands");
        {
            SparsePartition<int> island_marker(nx, ny, 0);
            std::vector<node> q;

            for(node flat : flats)
            {
                if (island_marker.getData(flat.x, flat.y) != 0) {
                    continue;
                }

                q.push_back(flat);

                int label = ++numIslands;
                islands.push_back(std::vector<node>());

                while(!q.empty()) {
                    node flat = q.back();
                    q.pop_back();

                    if (island_marker.getData(flat.x, flat.y) != 0) {
                        continue;
                    }

                    island_marker.setData(flat.x, flat.y, label);
                    islands[label - 1].push_back(flat);

                    for (int k=1; k<=8; k++) {
                        //if neighbor is in flat
                        int in = flat.x + d1[k];
                        int jn = flat.y + d2[k];

                        if ((jn == -1 || jn == ny) && flowDir.hasAccess(in, jn)) {
                            if (flowDir.getData(in, jn) == 0)
                            {
                                bordering_island_labels.insert(label);
                            }
                        }

                        if (!flowDir.isInPartition(in, jn))
                            continue;

                        if (flowDir.getData(in, jn) == 0)
                            q.push_back(node(in, jn));
                    }
                }
            }
        }
        t.end("Find islands");

        std::vector<std::vector<node>> borderingIslands;
        int localSharedFlats = 0, sharedFlats = 0;

        for (auto& label : bordering_island_labels) {
            std::vector<node> island = std::move(islands[label - 1]);

            localSharedFlats += island.size(); 
            borderingIslands.push_back(island);
        }

        t.start("Resolve shared flats");
        MPI_Allreduce(&localSharedFlats, &sharedFlats, 1, MPI_INT, MPI_SUM, MCW);

        if (rank == 0) {
            fprintf(stderr, "Processing partial flats\n");
            printf("PRL: %d flats shared across processors (%d local -> %.2f%% shared)\n", sharedFlats, totalNumFlat - sharedFlats, 100. * sharedFlats / totalNumFlat);
        }

        if (sharedFlats > 0) {
            SparsePartition<short> inc(nx, ny, 0);
            size_t lastNumFlat = resolveFlats_parallel(elevDEM, inc, flowDir, borderingIslands);

            if (rank==0) {
                fprintf(stderr, "PRL: Iteration complete. Number of flats remaining: %ld\n", lastNumFlat);
                fflush(stderr);
            }

            // Repeatedly call resolve flats until there is no change across all processors
            while (lastNumFlat > 0) {
                SparsePartition<short> newInc(nx, ny, 0);

                lastNumFlat = resolveFlats_parallel(inc, newInc, flowDir, borderingIslands); 
                inc = std::move(newInc);

                if (rank==0) {
                    fprintf(stderr, "PRL: Iteration complete. Number of flats remaining: %ld\n", lastNumFlat);
                    fflush(stderr);
                }
            }
        }
        t.end("Resolve shared flats");

        //printf("rank %d: Done, %d islands. Took %.2f seconds\n", rank, numIslands, MPI_Wtime() - flatFindStart);
        //printf("rank %d: %lu bordering islands with %d flats\n", rank, bordering_islands.size(), localSharedFlats);

        t.start("Resolve local flats");
        if (!islands.empty()) {
            SparsePartition<short> inc(nx, ny, 0);
            size_t lastNumFlat = resolveFlats(elevDEM, inc, flowDir, islands);

            if (rank==0) {
                fprintf(stderr, "Iteration complete. Number of flats remaining: %ld\n\n", lastNumFlat);
                fflush(stderr);
            }

            // Repeatedly call resolve flats until there is no change
            while (lastNumFlat > 0)
            {
                SparsePartition<short> newInc(nx, ny, 0);

                lastNumFlat = resolveFlats(inc, newInc, flowDir, islands); 
                inc = std::move(newInc);

                if (rank==0) {
                    fprintf(stderr, "Iteration complete. Number of flats remaining: %ld\n\n", lastNumFlat);
                    fflush(stderr);
                }
            } 
        }
        t.end("Resolve local flats");
    }

    t.end("Resolve flats");

    t.start("Write directions");
    tiffIO pointIO(pointfile, SHORT_TYPE, &flowDirNodata, dem);
    pointIO.write(xstart, ystart, ny, nx, flowDir.getGridPointer());
    t.end("Write directions");

    t.end("Total");
    t.stop();
    t.save("timing_info");

    MPI_Finalize();
    return 0;
}

// Sets only flowDir only where there is a positive slope
// Returns number of cells which are flat
long setPosDir(linearpart<float>& elevDEM, linearpart<short>& flowDir, SparsePartition<long>& area, int useflowfile)
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
                setFlow(i,j, flowDir, elevDEM, area, useflowfile);

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
void setFlow2(int i, int j, linearpart<short>& flowDir, T& elev, SparsePartition<short>& inc)
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

    for (int k : order) {
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

template<typename T>
void flowTowardsLower(T& elev, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<short>& inc)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    std::vector<node> lowBoundaries;

    // Find low boundaries. 
    for(auto& island : islands) {
        for(node flat : island) {
            float flatElev = elev.getData(flat.x, flat.y);

            for (int k = 1; k <= 8; k++) {
                if (dontCross(k, flat.x, flat.y, flowDir) == 0) {
                    int in = flat.x + d1[k];
                    int jn = flat.y + d2[k];

                    if (!flowDir.hasAccess(in, jn))
                        continue;

                    auto elevDiff = flatElev - elev.getData(in,jn);
                    short flow = flowDir.getData(in,jn);
                    
                    bool edgeDrain = flowDir.isNodata(in, jn);

                    // Adjacent cell drains and is equal or lower in elevation so this is a low boundary
                    if ((elevDiff >= 0 && flow > 0) || edgeDrain) {
                        lowBoundaries.push_back(flat);
                        inc.setData(flat.x, flat.y, -1);

                        // No need to check the other neighbors
                        break;
                    } 
                }
            }
        }
    }

    int numInc = propagateIncrements(flowDir, inc, lowBoundaries);

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    if (numInc > 0)          
    {
        markPits(elev, flowDir, islands, inc);
    }
}

template<typename T>
void flowFromHigher(T& elev, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<short>& inc)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    // Find high boundaries
    for (auto& island : islands) {
        std::vector<node> highBoundaries;

        for (node flat : island) {
            float flatElev = elev.getData(flat.x, flat.y);
            bool highBoundary = false;

            for (int k = 1; k <= 8; k++) {
                if (dontCross(k, flat.x, flat.y, flowDir) == 0) {
                    int in = flat.x + d1[k];
                    int jn = flat.y + d2[k];

                    if (!flowDir.hasAccess(in, jn))
                        continue;

                    auto elevDiff = flatElev - elev.getData(in, jn);
                    
                    if (elevDiff < 0) {
                        // Adjacent cell has higher elevation so this is a high boundary
                        highBoundary = true;
                        break;
                    }
                }
            }

            if (highBoundary) {
                inc.setData(flat.x, flat.y, -1);
                highBoundaries.push_back(flat);
            }
        }

        propagateIncrements(flowDir, inc, highBoundaries);
    }
}

template<typename T>
int markPits(T& elevDEM, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<short>& inc)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    int numPits = 0;

    //There are pits remaining - set direction to no data
    for (auto& island : islands) {
        for (node flat : island) {
            bool skip = false;

            for (int k=1; k<=8; k++) {
                if (dontCross(k, flat.x, flat.y, flowDir)==0) {
                    int jn = flat.y + d2[k];
                    int in = flat.x + d1[k];

                    if (!flowDir.hasAccess(in, jn)) 
                        continue;

                    auto elevDiff = elevDEM.getData(flat.x, flat.y) - elevDEM.getData(in, jn);
                    short flow = flowDir.getData(in,jn);
                    
                    // Adjacent cell drains and is equal or lower in elevation so this is a low boundary
                    if (elevDiff >= 0 && flow > 0) {
                        skip = true;
                        break;
                    } else if (flow == 0) {
                        // If neighbor is in flat

                        // FIXME: check if this is correct
                        if (inc.getData(in,jn) >= 0){ // && inc.getData(in,jn)<st) {
                            skip = true;
                            break;
                        }
                    }
                }
            }
            
            // mark pit
            if (!skip) {
                numPits++;
                flowDir.setToNodata(flat.x, flat.y);
            }  
        }
    }

    return numPits;
}


template<typename T>
long resolveFlats(T& elevDEM, SparsePartition<short>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    int rank;
    MPI_Comm_rank(MCW, &rank);
    
    if (rank==0) {
        fprintf(stderr,"Resolving flats\n");
        fflush(stderr);
    }

    flowTowardsLower(elevDEM, flowDir, islands, inc);

    // Drain flats away from higher adjacent terrain
    SparsePartition<short> s(nx, ny, 0);
    
    flowFromHigher(elevDEM, flowDir, islands, s);

    // High flow must be inverted before it is combined
    //
    // higherFlowMax has to be greater than all of the increments
    // higherFlowMax can be maximum value of the data type but it will cause overflow problems if more than one iteration is needed
    short higherFlowMax = 0;

    for (auto& island : islands) {
        for (node flat : island) {    
            short val = s.getData(flat.x, flat.y);

            if (val > higherFlowMax)
                higherFlowMax = val;
        }
    }

    for (auto& island : islands) {
        for (auto flat : island) {
            inc.addToData(flat.x, flat.y, higherFlowMax - s.getData(flat.x, flat.y));
        }
    }

    if (rank==0) {
        fprintf(stderr,"Setting directions\n");
        fflush(stderr);
    }

    long flatsRemaining = 0;
    for (auto& island : islands) {
        for (node flat : island) {
            setFlow2(flat.x, flat.y, flowDir, elevDEM, inc);

            if (flowDir.getData(flat.x, flat.y) == 0) {
                flatsRemaining++;
            }
        }
    }

    auto hasFlowDirection = [&](const node& n) { return flowDir.getData(n.x, n.y) != 0; };
    auto isEmpty = [&](const std::vector<node>& i) { return i.empty(); };
    
    // Remove flats which have flow direction set
    for (auto& island : islands) {
        island.erase(std::remove_if(island.begin(), island.end(), hasFlowDirection), island.end());
    }

    // Remove empty islands
    islands.erase(std::remove_if(islands.begin(), islands.end(), isEmpty), islands.end());

    return flatsRemaining;
}

template<typename T>
long resolveFlats_parallel(T& elev, SparsePartition<short>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    int rank;
    MPI_Comm_rank(MCW, &rank);

    int numFlatsChanged = 0, totalNumFlatsChanged = 0;

    flowTowardsLower(elev, flowDir, islands, inc);

    do {
        inc.share();
        numFlatsChanged = propagateBorderIncrements(flowDir, inc);

        MPI_Allreduce(&numFlatsChanged, &totalNumFlatsChanged, 1, MPI_INT, MPI_SUM, MCW);

        if (rank == 0) {
            printf("PRL: Lower gradient processed %d flats this iteration\n", totalNumFlatsChanged);
        }
    } while(totalNumFlatsChanged > 0);

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    markPits(elev, flowDir, islands, inc);

    // Drain flats away from higher adjacent terrain
    SparsePartition<short> higherGradient(nx, ny, 0);
   
    flowFromHigher(elev, flowDir, islands, higherGradient);

    do {
        higherGradient.share();
        numFlatsChanged = propagateBorderIncrements(flowDir, higherGradient);

        MPI_Allreduce(&numFlatsChanged, &totalNumFlatsChanged, 1, MPI_INT, MPI_SUM, MCW);

        if (rank == 0) {
            printf("PRL: Higher gradient processed %d flats this iteration\n", totalNumFlatsChanged);
        }
    } while(totalNumFlatsChanged > 0);

    // High flow must be inverted before it is combined
    //
    // higherFlowMax has to be greater than all of the increments
    // higherFlowMax can be maximum value of the data type (e.g. 65535) but it will cause overflow problems if more than one iteration is needed
    short higherFlowMax = 0;

    for (auto& island : islands) {
        for (auto& flat : island) {
            short val = higherGradient.getData(flat.x, flat.y);
        
            if (val > higherFlowMax)
                higherFlowMax = val;
        }
    }

    // FIXME: Is this needed? would it affect directions at the border?
    short globalHigherFlowmax = 0;
    MPI_Allreduce(&higherFlowMax, &globalHigherFlowmax, 1, MPI_SHORT, MPI_MAX, MCW);

    for (auto& island : islands) {
        for (auto flat : island) {
            inc.addToData(flat.x, flat.y, globalHigherFlowmax - higherGradient.getData(flat.x, flat.y));
        }
    }

    inc.share();

    if (rank==0) {
        fprintf(stderr,"\nPRL: Setting directions\n");
        fflush(stderr);
    }

    long localFlatsRemaining = 0, globalFlatsRemaining = 0;

    for (auto& island : islands) {
        for (node flat : island) {
            setFlow2(flat.x, flat.y, flowDir, elev, inc);
    
            if (flowDir.getData(flat.x, flat.y) == 0) {
                localFlatsRemaining++;
            }
        }
    }

    flowDir.share();
    MPI_Allreduce(&localFlatsRemaining, &globalFlatsRemaining, 1, MPI_LONG, MPI_SUM, MCW); 

    auto hasFlowDirection = [&](const node& n) { return flowDir.getData(n.x, n.y) != 0; };
    auto isEmpty = [&](const std::vector<node>& i) { return i.empty(); };
    
    // Remove flats which have flow direction set
    for (auto& island : islands) {
        island.erase(std::remove_if(island.begin(), island.end(), hasFlowDirection), island.end());
    }

    // Remove empty islands
    islands.erase(std::remove_if(islands.begin(), islands.end(), isEmpty), islands.end());

    return globalFlatsRemaining;
}

int propagateIncrements(linearpart<short>& flowDir, SparsePartition<short>& inc, std::vector<node>& queue) {
    int numInc = 0;
    int st = 1;
    
    while (!queue.empty()) {
        std::vector<node> newFlats;

        for(node flat : queue) {
            // Duplicate. already set
            if (inc.getData(flat.x, flat.y) > 0)
                continue;

            for (int k = 1; k <= 8; k++) {
                if (dontCross(k, flat.x, flat.y, flowDir) == 0) {
                    int in = flat.x + d1[k];
                    int jn = flat.y + d2[k];

                    if (!flowDir.isInPartition(in, jn)) 
                        continue;

                    short flow = flowDir.getData(in,jn);

                    if (flow == 0 && inc.getData(in, jn) == 0) {
                        newFlats.push_back(node(in, jn));
                        inc.setData(in, jn, -1);
                    }
                }
            }

            numInc++;
            inc.setData(flat.x, flat.y, st);
        }

        queue.swap(newFlats);
        st++;
    }

    return numInc;
}

int propagateBorderIncrements(linearpart<short>& flowDir, SparsePartition<short>& inc)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    struct pnode {
        int x;
        int y;
        int inc;

        bool operator<(const struct pnode& b) const {
            return inc < b.inc;
        }
    };
    
    std::vector<pnode> queue;

    // Find the starting nodes at the edge of the raster
    //
    // FIXME: oob access
    for (auto y : {-1, ny}) {
        for(int x = 0; x < nx; x++) {
            int st = inc.getData(x, y);

            if (st == 0)
                continue;

            auto jn = y == -1 ? 0 : ny - 1;

            for (auto in : {x-1, x, x+1}) {
                if (!flowDir.isInPartition(in, jn))
                    continue;

                short flow = flowDir.getData(in, jn);
                auto neighSt = inc.getData(in, jn);

                if (flow == 0 && (neighSt == 0 || neighSt > st + 1 || -neighSt > st + 1)) {
                    queue.push_back({in, jn, st + 1});

                    // Here we set a negative increment if it's still pending
                    //
                    // Another flat might be neighboring the same cell with a lower increment,
                    // which has to override the higher increment.
                    inc.setData(in, jn, -(st + 1));
                }
            }
        }
    }

    // Sort queue by lowest increment
    std::sort(queue.begin(), queue.end());

    int numInc = 0;

    while (!queue.empty()) {
        std::vector<pnode> newFlats;

        for(pnode flat : queue) {
            // Skip if the increment was already set and it is lower 
            auto st = inc.getData(flat.x, flat.y);
            if (st > 0 && st <= flat.inc) {
                continue;
            }

            for (int k = 1; k <= 8; k++) {
                if (dontCross(k, flat.x, flat.y, flowDir) == 0) {
                    int in = flat.x + d1[k];
                    int jn = flat.y + d2[k];

                    if (!flowDir.isInPartition(in, jn)) 
                        continue;

                    short flow = flowDir.getData(in, jn);
                    auto neighInc = inc.getData(in, jn);

                    if (flow == 0 && (neighInc == 0 || neighInc > flat.inc + 1 || -neighInc > flat.inc + 1)) {
                        newFlats.push_back({in, jn, flat.inc + 1});
                        inc.setData(in, jn, -(flat.inc + 1));
                    }
                }
            }

            inc.setData(flat.x, flat.y, flat.inc);
            numInc++;
        }

        std::sort(newFlats.begin(), newFlats.end());
        queue.swap(newFlats);
    }

    return numInc;
}
