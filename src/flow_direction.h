#ifndef FLOW_DIRECTION_H
#define FLOW_DIRECTION_H

#include <algorithm>
#include <vector>

#include "commonLib.h"
#include "linearpart.h"
#include "sparsepartition.h"

int dontCross(int k, int i, int j, linearpart<short>& flowDir);

size_t propagateIncrements(linearpart<short>& flowDir, SparsePartition<int>& inc, std::vector<node>& queue);
size_t propagateBorderIncrements(linearpart<short>& flowDir, SparsePartition<int>& inc);
size_t propagateBorderIncrements_async(linearpart<short>& flowDir, SparsePartition<int>& inc, bool top, std::vector<int>& changes, bool& top_updated, bool& bottom_updated);

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

class AsyncRasterProcessor
{
    public:
        typedef std::function<void(bool, std::vector<int>&, bool&, bool&)> update_fn;

        AsyncRasterProcessor() {
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
       
            if (rank == 0) {
                outstanding_updates = std::unique_ptr<int[]>(new int[size]);
            }
        }

        void add(AsyncPartition* raster, update_fn fn) {
            rasters.push_back(raster);
            raster_update_fns.push_back(fn);
        }

        void run() {
            int num_borders = rasters.size() * 2;

            size_t max_border_size = 0;
            for (auto* r : rasters) {
                max_border_size = std::max(max_border_size, r->get_async_buf_size());
            }

            // FIXME:
            size_t buffer_size = size * num_borders * (max_border_size + MPI_BSEND_OVERHEAD);
            mpi_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[buffer_size]);
            MPI_Buffer_attach(mpi_buffer.get(), buffer_size);

            if (rank == 0) {
                printf("PRL: Allocating %s for MPI buffer\n", humanReadableSize(buffer_size).c_str());
                std::fill(outstanding_updates.get(), outstanding_updates.get() + size, rasters.size());
            }

            // Tags:
            //  100 - control message
            //  101+ - borders messages

            // Send initial border data if it is modified (or for all cases?)
            for (int x = 0; x < rasters.size(); x++) {
                int tag = 101 + x * 2;

                // FIXME: send initial border only if updated
                auto* r = rasters[x];

                int updates[2] = { -1, -1 };

                if (rank > 0) {
                    r->async_store_buf(true);

                    MPI_Request req;
                    MPI_Ibsend(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE, rank - 1, tag, MPI_COMM_WORLD, &req);
                    MPI_Request_free(&req);

                    updates[0] = rank - 1;
                }

                if (rank < size - 1) {
                    r->async_store_buf(false);

                    MPI_Request req;
                    MPI_Ibsend(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE, rank + 1, tag + 1, MPI_COMM_WORLD, &req);
                    MPI_Request_free(&req);

                    updates[1] = rank + 1;
                }

                MPI_Request req;
                MPI_Ibsend(updates, 2, MPI_INT, 0, 100, MCW, &req);
                MPI_Request_free(&req);
            }

            std::vector<int> border_changes;

            while (true) {
                MPI_Status status;
                int err = MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                if (err != MPI_SUCCESS) {
                    printf("MPI_Probe failed on rank %d - %d\n", rank, err);
                }

                //printf("Got message from %d tag %d\n", status.MPI_SOURCE, status.MPI_TAG);

                if (status.MPI_TAG == 100) {
                    int updates[2] = {-1, -1};
                    MPI_Recv(updates, 2, MPI_INT, status.MPI_SOURCE, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (rank == 0) {
                        // Process status notifications from other ranks

                        outstanding_updates[status.MPI_SOURCE]--;

                        if (updates[0] != -1) { outstanding_updates[updates[0]]++; }
                        if (updates[1] != -1) { outstanding_updates[updates[1]]++; }

                        bool done = true;

                        for (int x = 0; x < size; x++) {
                            if (outstanding_updates[x] != 0) {
                                done = false;
                                break;
                            }
                        }

                        if (done) {
                            int unused[2] = {0, 0};

                            for (int x = 1; x < size; x++) {
                                MPI_Request req;
                                MPI_Ibsend(unused, 2, MPI_INT, x, 100, MPI_COMM_WORLD, &req);
                                MPI_Request_free(&req);
                            }

                            break;
                        }
                    } else {
                        // Root signaled global completion - we are done.
                        break;
                    }
                } else if (status.MPI_TAG > 100) {
                    // Border update
                    total_comms++;
                    int raster_n = (status.MPI_TAG - 101) / 2;
                    int top = (status.MPI_TAG - 101) % 2;

                    //printf("%d: Got border upd to %d %d\n", rank, raster_n, top);

                    auto* r = rasters[raster_n];

                    MPI_Recv(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE,
                            status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    border_changes.clear();
                    r->async_load_buf(top, border_changes);

                    comm_bytes_needed += border_changes.size() * sizeof(int);

                    bool top_modified = false, bottom_modified = false;
                    raster_update_fns[raster_n](top, border_changes, top_modified, bottom_modified);

                    int tag = 101 + raster_n * 2;

                    int updates[2] = { -1, -1 };

                    // Top neighbor exists
                    if (rank != 0 && top_modified) {
                        r->async_store_buf(true);
                        MPI_Request req;
                        MPI_Ibsend(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE, rank - 1, tag, MPI_COMM_WORLD, &req);
                        MPI_Request_free(&req);

                        updates[0] = rank - 1;
                    }

                    // Bottom neighbor exists
                    if (rank != size - 1 && bottom_modified) {
                        r->async_store_buf(false);
                        MPI_Request req;
                        MPI_Ibsend(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE, rank + 1, tag + 1, MPI_COMM_WORLD, &req);
                        MPI_Request_free(&req);

                        updates[1] = rank + 1;
                    }

                    MPI_Request req;
                    MPI_Ibsend(&updates, 2, MPI_INT, 0, 100, MCW, &req);
                    MPI_Request_free(&req);
                } else {
                    printf("%d: unrecognized tag %d from %d\n", rank, status.MPI_TAG, status.MPI_SOURCE);
                }
            }

            // Detach our MPI buffer
            void* buf_addr;
            int buf_size;
            MPI_Buffer_detach(&buf_addr, &buf_size);

            int global_num_comms = 0;
            MPI_Reduce(&total_comms, &global_num_comms, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

            if (rank == 0) {
                printf("PRL: took %d border transfers - %s (min needed %s)\n", global_num_comms, humanReadableSize(global_num_comms * max_border_size).c_str(), humanReadableSize(comm_bytes_needed).c_str());
            }
        }

    private:
        int rank, size;
        int total_comms = 0;
        int comm_bytes_needed = 0;

        std::unique_ptr<uint8_t[]> mpi_buffer;
        std::unique_ptr<int[]> outstanding_updates;

        std::vector<AsyncPartition*> rasters;
        std::vector<update_fn> raster_update_fns;
};

template<typename T>
long resolveFlats_parallel_async(T& elev, SparsePartition<int>& inc, linearpart<short>& flowDir, std::vector<std::vector<node>>& islands)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    int rank;
    MPI_Comm_rank(MCW, &rank);

    SparsePartition<int> higherGradient(nx, ny, 0);

    flowTowardsLower(elev, flowDir, islands, inc);
    flowFromHigher(elev, flowDir, islands, higherGradient);

    {
        AsyncRasterProcessor arp;

        arp.add(&inc, [&flowDir, &inc](bool new_top, std::vector<int>& border_diff, bool& top_updated, bool& bottom_updated) {
            //printf("got low update - %d\n", border_diff.size());

            propagateBorderIncrements_async(flowDir, inc, new_top, border_diff, top_updated, bottom_updated);
        });

        arp.add(&higherGradient, [&flowDir, &higherGradient](bool new_top, std::vector<int>& border_diff, bool& top_updated, bool& bottom_updated) {
            //printf("got high update - %d\n", border_diff.size());

            propagateBorderIncrements_async(flowDir, higherGradient, new_top, border_diff, top_updated, bottom_updated);
        });

        arp.run();
     }

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    markPits(elev, flowDir, islands, inc);

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
