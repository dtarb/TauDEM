#ifndef FLOW_DIRECTION_H
#define FLOW_DIRECTION_H

#include <algorithm>
#include <vector>

#include "linearpart.h"
#include "sparsepartition.h"

int dontCross(int k, int i, int j, linearpart<short>& flowDir);

size_t propagateIncrements(linearpart<short>& flowDir, SparsePartition<int>& inc, std::vector<node>& queue);
size_t propagateBorderIncrements(linearpart<short>& flowDir, SparsePartition<int>& inc);

template<typename T>
void flowTowardsLower(T& elev, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<int>& inc)
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

    size_t numInc = propagateIncrements(flowDir, inc, lowBoundaries);

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    if (numInc > 0)          
    {
        markPits(elev, flowDir, islands, inc);
    }
}

template<typename T>
void flowFromHigher(T& elev, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<int>& inc)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    std::vector<node> highBoundaries;

    // Find high boundaries
    for (auto& island : islands) {
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
    }

    propagateIncrements(flowDir, inc, highBoundaries);
}

template<typename T>
int markPits(T& elevDEM, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands, SparsePartition<int>& inc)
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
long resolveFlats(T& elevDEM, SparsePartition<int>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands)
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
    SparsePartition<int> s(nx, ny, 0);
    
    flowFromHigher(elevDEM, flowDir, islands, s);

    // High flow must be inverted before it is combined
    //
    // higherFlowMax has to be greater than all of the increments
    // higherFlowMax can be maximum value of the data type but it will cause overflow problems if more than one iteration is needed
    int higherFlowMax = 0;

    for (auto& island : islands) {
        for (node flat : island) {    
            int val = s.getData(flat.x, flat.y);

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
long resolveFlats_parallel(T& elev, SparsePartition<int>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    int rank;
    MPI_Comm_rank(MCW, &rank);

    uint64_t numFlatsChanged = 0, totalNumFlatsChanged = 0;

    flowTowardsLower(elev, flowDir, islands, inc);

    do {
        inc.share();
        numFlatsChanged = propagateBorderIncrements(flowDir, inc);

        MPI_Allreduce(&numFlatsChanged, &totalNumFlatsChanged, 1, MPI_UINT64_T, MPI_SUM, MCW);

        if (rank == 0) {
            printf("PRL: Lower gradient processed %llu flats this iteration\n", totalNumFlatsChanged);
        }
    } while(totalNumFlatsChanged > 0);

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    markPits(elev, flowDir, islands, inc);

    // Drain flats away from higher adjacent terrain
    SparsePartition<int> higherGradient(nx, ny, 0);
   
    flowFromHigher(elev, flowDir, islands, higherGradient);

    do {
        higherGradient.share();
        numFlatsChanged = propagateBorderIncrements(flowDir, higherGradient);

        MPI_Allreduce(&numFlatsChanged, &totalNumFlatsChanged, 1, MPI_UINT64_T, MPI_SUM, MCW);

        if (rank == 0) {
            printf("PRL: Higher gradient processed %llu flats this iteration\n", totalNumFlatsChanged);
        }
    } while(totalNumFlatsChanged > 0);

    // High flow must be inverted before it is combined
    //
    // higherFlowMax has to be greater than all of the increments
    // higherFlowMax can be maximum value of the data type (e.g. 65535) but it will cause overflow problems if more than one iteration is needed
    int higherFlowMax = 0;

    for (auto& island : islands) {
        for (auto& flat : island) {
            int val = higherGradient.getData(flat.x, flat.y);
        
            if (val > higherFlowMax)
                higherFlowMax = val;
        }
    }

    // FIXME: Is this needed? would it affect directions at the border?
    // It is local to a flat area, but can that be reduced further to minimize comm?
    int globalHigherFlowmax = 0;
    MPI_Allreduce(&higherFlowMax, &globalHigherFlowmax, 1, MPI_INT, MPI_MAX, MCW);

    size_t badCells = 0;

    for (auto& island : islands) {
        for (auto flat : island) {
            auto val = inc.getData(flat.x, flat.y);
            auto highFlow = higherGradient.getData(flat.x, flat.y);

            inc.setData(flat.x, flat.y, val + (globalHigherFlowmax - highFlow));

            if (val < 0 || val == INT_MAX || highFlow < 0 || highFlow == INT_MAX) {
                badCells++;
            }
        }
    }

    if (badCells > 0) {
        printf("warning rank %d: %d increment values either incorrect or overflown\n", rank, badCells);
    }

    inc.share();

    if (rank==0) {
        fprintf(stderr,"\nPRL: Setting directions\n");
        fflush(stderr);
    }

    uint64_t localFlatsRemaining = 0, globalFlatsRemaining = 0;

    for (auto& island : islands) {
        for (node flat : island) {
            setFlow2(flat.x, flat.y, flowDir, elev, inc);
    
            if (flowDir.getData(flat.x, flat.y) == 0) {
                localFlatsRemaining++;
            }
        }
    }

    flowDir.share();
    MPI_Allreduce(&localFlatsRemaining, &globalFlatsRemaining, 1, MPI_UINT64_T, MPI_SUM, MCW);

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


#endif
