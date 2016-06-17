#ifndef FLOW_DIRECTION_H
#define FLOW_DIRECTION_H

#include <algorithm>
#include <vector>

#include "commonLib.h"
#include "linearpart.h"
#include "sparsepartition.h"

using std::vector;

// Checks if cells cross
bool cellsCross(int k, int i, int j, linearpart<short>& flowDir)
{
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

    int in1=i+d1[n1];
    int jn1=j+d2[n1];
    int in2=i+d1[n2];
    int jn2=j+d2[n2];

    if (flowDir.getData(in1,jn1) == c1 || flowDir.getData(in2,jn2) == c2)
    {
        return true;
    }

    return false;
}

// FIXME: D-inf, should we care if cells cross?
bool cellsCross(int k, int i, int j, linearpart<float>& flowDir)
{
    return false;
}

template<typename Algo, typename E>
int markPits(E& elev, linearpart<typename Algo::FlowType>& flowDir, vector<vector<node>>& islands, SparsePartition<int>& inc);

template<typename Algo>
size_t propagateIncrements(linearpart<typename Algo::FlowType>& flowDir, SparsePartition<int>& inc, vector<node>& queue) {
    size_t numInc = 0;
    int st = 1;
    
    vector<node> newFlats;
    while (!queue.empty()) {
        for(node flat : queue) {
            // Duplicate. already set
            if (inc.getData(flat.x, flat.y) > 0)
                continue;

            for (int k = 1; k <= 8; k++) {
                if (cellsCross(k, flat.x, flat.y, flowDir))
                    continue;

                int in = flat.x + d1[k];
                int jn = flat.y + d2[k];

                if (!flowDir.isInPartition(in, jn)) 
                    continue;

                typename Algo::FlowType flow = flowDir.getData(in,jn);

                if (!Algo::HasFlow(flow) && inc.getData(in, jn) == 0) {
                    newFlats.push_back(node(in, jn));
                    inc.setData(in, jn, -1);
                }
            }

            numInc++;
            inc.setData(flat.x, flat.y, st);
        }

        queue.clear();
        queue.swap(newFlats);
        st++;
    }

    if (st < 0) {
        printf("WARNING: increment overflow during propagation (st = %d)\n", st);
    }

    return numInc;
}

template<typename Algo>
size_t propagateBorderIncrements_async(linearpart<typename Algo::FlowType>& flowDir, SparsePartition<int>& inc, bool top, vector<node>& changes, bool& top_updated, bool& bottom_updated)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    vector<node> queue;

    // Find the starting nodes at the edge of the raster
    int ignoredGhostCells = 0;

    for (auto flat : changes) {
        int st = inc.getData(flat.x, flat.y);

        if (st == 0)
            continue;

        if (st == INT_MAX) {
            ignoredGhostCells++;
            continue;
        }

        auto jn = flat.y == -1 ? 0 : ny - 1;

        for (auto in : {flat.x-1, flat.x, flat.x+1}) {
            if (!flowDir.isInPartition(in, jn))
                continue;

            bool noFlow = !Algo::HasFlow(flowDir.getData(in, jn));
            auto neighInc = inc.getData(in, jn);

            if (noFlow && (neighInc == 0 || std::abs(neighInc) > st + 1)) {
                // If neighbor increment is positive, we are overriding a larger increment
                // and it is not yet in the queue
                if (neighInc >= 0) {
                    queue.emplace_back(in, jn);
                }

                // Here we set a negative increment if it's still not set
                //
                // Another flat might be neighboring the same cell with a lower increment,
                // which has to override the higher increment (that hasn't been set yet but is in the queue).
                inc.setData(in, jn, -(st + 1));
            }
        }
    }

    if (ignoredGhostCells > 0) {
       printf("warning: ignored %d ghost cells which were at upper limit (%d)\n", ignoredGhostCells, SHRT_MAX);
    }

    size_t numChanged = 0;
    size_t abandonedCells = 0;
    vector<node> newFlats;

    while (!queue.empty()) {
        for(node flat : queue) {
            // Increments are stored as negative for the cells that have been added to the queue
            // (to signify that they need to be explored)
            auto st = -inc.getData(flat.x, flat.y);

            // I don't think this is possible anymore, but just in case.
            if (st <= 0) {
                printf("warning: unexpected non-negative increment @ (%d, %d) - %d\n", flat.x, flat.y, -st);
                continue;
            }

            inc.setData(flat.x, flat.y, st);
            numChanged++;

            if (flat.y == 0) top_updated = true;
            if (flat.y == ny - 1) bottom_updated = true;

            if (st == INT_MAX) {
                abandonedCells++;
                continue;
            }

            for (int k = 1; k <= 8; k++) {
                if (cellsCross(k, flat.x, flat.y, flowDir))
                    continue;

                int in = flat.x + d1[k];
                int jn = flat.y + d2[k];

                if (!flowDir.isInPartition(in, jn))
                    continue;

                bool noFlow = !Algo::HasFlow(flowDir.getData(in, jn));
                auto neighInc = inc.getData(in, jn);

                if (noFlow && (neighInc == 0 || std::abs(neighInc) > st + 1)) {
                    // If neighbor increment is positive, we are overriding a larger increment
                    // and it is not yet in the queue
                    if (neighInc >= 0) {
                        newFlats.emplace_back(in, jn);
                    }

                    inc.setData(in, jn, -(st + 1));
                }
            }
        }

        queue.clear();
        queue.swap(newFlats);
    }

    if (abandonedCells > 0) {
        printf("warning: gave up propagating %zu cells because they were at upper limit (%d)\n", abandonedCells, INT_MAX);
    }

    return numChanged;
}

template<typename Algo, typename E>
void flowTowardsLower(E& elev, linearpart<typename Algo::FlowType>& flowDir, vector<vector<node>>& islands, SparsePartition<int>& inc)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    vector<node> lowBoundaries;

    // Find low boundaries. 
    for(auto& island : islands) {
        for(node flat : island) {
            float flatElev = elev.getData(flat.x, flat.y);

            for (int k = 1; k <= 8; k++) {
                if (cellsCross(k, flat.x, flat.y, flowDir))
                    continue;

                int in = flat.x + d1[k];
                int jn = flat.y + d2[k];

                if (!flowDir.hasAccess(in, jn)) {
                    continue;
                }

                auto elevDiff = flatElev - elev.getData(in,jn);
                typename Algo::FlowType flow = flowDir.getData(in,jn);

                // Adjacent cell drains and is equal or lower in elevation so this is a low boundary
                if (elevDiff >= 0 && Algo::HasFlow(flow)) {
                    lowBoundaries.push_back(flat);
                    inc.setData(flat.x, flat.y, -1);

                    // No need to check the other neighbors
                    break;
                } 
            }
        }
    }

    size_t numInc = propagateIncrements<Algo>(flowDir, inc, lowBoundaries);
}

template<typename Algo, typename E>
void flowFromHigher(E& elev, linearpart<typename Algo::FlowType>& flowDir, vector<vector<node>>& islands, SparsePartition<int>& inc)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    vector<node> highBoundaries;

    // Find high boundaries
    for (auto& island : islands) {
        for (node flat : island) {
            float flatElev = elev.getData(flat.x, flat.y);
            bool highBoundary = false;

            for (int k = 1; k <= 8; k++) {
                if (cellsCross(k, flat.x, flat.y, flowDir))
                    continue;

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

            if (highBoundary) {
                inc.setData(flat.x, flat.y, -1);
                highBoundaries.push_back(flat);
            }
        }
    }

    propagateIncrements<Algo>(flowDir, inc, highBoundaries);
}

template<typename Algo, typename E>
int markPits(E& elev, linearpart<typename Algo::FlowType>& flowDir, vector<vector<node>>& islands, SparsePartition<int>& inc)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    int numPits = 0;

    //There are pits remaining - set direction to no data
    for (auto& island : islands) {
        for (node flat : island) {
            bool skip = false;

            for (int k=1; k<=8; k++) {
                if (cellsCross(k, flat.x, flat.y, flowDir))
                    continue;

                int in = flat.x + d1[k];
                int jn = flat.y + d2[k];

                if (!flowDir.hasAccess(in, jn)) 
                    continue;

                auto elevDiff = elev.getData(flat.x, flat.y) - elev.getData(in, jn);
                typename Algo::FlowType flow = flowDir.getData(in,jn);

                // Adjacent cell drains and is equal or lower in elevation so this is a low boundary
                if (elevDiff >= 0 && Algo::HasFlow(flow)) {
                    skip = true;
                    break;
                } else if (!Algo::HasFlow(flow)) {
                    // If neighbor is in flat

                    if (inc.getData(in,jn) >= 0){ 
                        skip = true;
                        break;
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

template<typename Algo>
void findIslands(linearpart<typename Algo::FlowType>& flowDir, vector<vector<node>>& localIslands, vector<vector<node>>& sharedIslands)
{
    int width = flowDir.getnx();
    int height = flowDir.getny();

    int numIslands = 0;
    SparsePartition<int> islandLabel(width, height, 0);
    vector<node> q, tempVector;

    for (int j=0; j<height; j++) {
        for(int i=0; i<width; i++) {
            if (Algo::HasFlow(flowDir.getData(i, j))) {
                continue;
            }

            node flat(i, j);

            if (islandLabel.getData(flat.x, flat.y) != 0) {
                continue;
            }

            bool shared = false;
            int label = ++numIslands;
            
            q.push_back(flat);

            while(!q.empty()) {
                node flat = q.back();
                q.pop_back();

                if (islandLabel.getData(flat.x, flat.y) != 0) {
                    continue;
                }

                islandLabel.setData(flat.x, flat.y, label);
                tempVector.push_back(flat);

                for (int k=1; k<=8; k++) {
                    int in = flat.x + d1[k];
                    int jn = flat.y + d2[k];

                    if ((jn == -1 || jn == height) && flowDir.hasAccess(in, jn)) {
                        if (!Algo::HasFlow(flowDir.getData(in, jn)))
                        {
                            shared = true;
                        }
                    }

                    if (!flowDir.isInPartition(in, jn))
                        continue;

                    if (!Algo::HasFlow(flowDir.getData(in, jn)))
                        q.push_back(node(in, jn));
                }
            }

            if (!shared) {
                localIslands.push_back(vector<node>(tempVector));
            } else {
                sharedIslands.push_back(vector<node>(tempVector));
            }

            tempVector.clear();
        }
    }
}

template<typename Algo, typename E>
long resolveFlats(E& elev, SparsePartition<int>& inc, linearpart<typename Algo::FlowType>& flowDir, std::vector<std::vector<node>>& islands)
{
    long nx = flowDir.getnx();
    long ny = flowDir.getny();

    int rank;
    MPI_Comm_rank(MCW, &rank);
    
    if (rank==0) {
        fprintf(stderr,"Resolving flats\n");
        fflush(stderr);
    }

    SparsePartition<int> s(nx, ny, 0);
    
    flowTowardsLower<Algo>(elev, flowDir, islands, inc);

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    markPits<Algo>(elev, flowDir, islands, inc);

    // Drain flats away from higher adjacent terrain
    flowFromHigher<Algo>(elev, flowDir, islands, s);

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
            Algo::SetFlow(flat.x, flat.y, flowDir, elev, inc);

            if (!Algo::HasFlow(flowDir.getData(flat.x, flat.y))) {
                flatsRemaining++;
            }
        }
    }

    auto hasFlowDirection = [&](const node& n) { return Algo::HasFlow(flowDir.getData(n.x, n.y)); };
    auto isEmpty = [&](const std::vector<node>& i) { return i.empty(); };
    
    // Remove flats which have flow direction set
    for (auto& island : islands) {
        island.erase(std::remove_if(island.begin(), island.end(), hasFlowDirection), island.end());
    }

    // Remove empty islands
    islands.erase(std::remove_if(islands.begin(), islands.end(), isEmpty), islands.end());

    return flatsRemaining;
}

class AsyncRasterProcessor
{
    public:
        typedef std::function<void(bool, std::vector<node>&, bool&, bool&)> update_fn;

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

            // FIXME: pick reasonable size
            // FIXME: actually, get rid of buffered sends
            size_t buffer_size = size * num_borders * (max_border_size + MPI_BSEND_OVERHEAD);
            mpi_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[buffer_size]);
            MPI_Buffer_attach(mpi_buffer.get(), buffer_size);

            if (rank == 0) {
                printf("ASYNC: Allocating %s for MPI buffer\n", humanReadableSize(buffer_size).c_str());
                std::fill(outstanding_updates.get(), outstanding_updates.get() + size, rasters.size());
            }

            // Tags:
            //  100 - control message
            //  101+ - borders messages
            MPI_Request req;

            // Send initial border data if it is modified (or for all cases?)
            for (int x = 0; x < rasters.size(); x++) {
                int tag = 101 + x;

                // FIXME: send initial border only if updated
                auto* r = rasters[x];

                int updates[2] = { -1, -1 };

                uint8_t* buf = (uint8_t *) r->get_async_buf();
                int buf_size = r->get_async_buf_size();

                if (rank > 0) {
                    r->async_store_buf(true);

                    bool empty = std::all_of(buf, buf + buf_size, [](uint8_t x) { return x == 0; }); 

                    if (!empty) {
                        MPI_Ibsend(buf, buf_size, MPI_BYTE, rank - 1, tag, MPI_COMM_WORLD, &req);
                        MPI_Request_free(&req);

                        updates[0] = rank - 1;
                    }
                }

                if (rank < size - 1) {
                    r->async_store_buf(false);

                    bool empty = std::all_of(buf, buf + buf_size, [](uint8_t x) { return x == 0; }); 

                    if (!empty) {
                        MPI_Ibsend(buf, buf_size, MPI_BYTE, rank + 1, tag, MPI_COMM_WORLD, &req);
                        MPI_Request_free(&req);

                        updates[1] = rank + 1;
                    }
                }

                MPI_Ibsend(updates, 2, MPI_INT, 0, 100, MCW, &req);
                MPI_Request_free(&req);
            }

            std::vector<node> border_changes;

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
                            MPI_Request req;
                            int unused[2] = {0, 0};

                            for (int x = 1; x < size; x++) {
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
                    int raster_n = (status.MPI_TAG - 101);
                    int top = status.MPI_SOURCE + 1 == rank;

                    //printf("%d: Got border upd to %d %d\n", rank, raster_n, top);

                    auto* r = rasters[raster_n];

                    MPI_Recv(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE,
                            status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    border_changes.clear();
                    r->async_load_buf(top, border_changes);

                    comm_bytes_needed += border_changes.size() * sizeof(int);

                    bool top_modified = false, bottom_modified = false;
                    raster_update_fns[raster_n](top, border_changes, top_modified, bottom_modified);

                    int tag = 101 + raster_n;
                    MPI_Request req;
                    int updates[2] = { -1, -1 };

                    // Top neighbor exists
                    if (rank != 0 && top_modified) {
                        r->async_store_buf(true);
                        MPI_Ibsend(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE, rank - 1, tag, MPI_COMM_WORLD, &req);
                        MPI_Request_free(&req);

                        updates[0] = rank - 1;
                    }

                    // Bottom neighbor exists
                    if (rank != size - 1 && bottom_modified) {
                        r->async_store_buf(false);
                        MPI_Ibsend(r->get_async_buf(), r->get_async_buf_size(), MPI_BYTE, rank + 1, tag, MPI_COMM_WORLD, &req);
                        MPI_Request_free(&req);

                        updates[1] = rank + 1;
                    }

                    MPI_Ibsend(updates, 2, MPI_INT, 0, 100, MCW, &req);
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
            int global_bytes_needed = 0;
            
            MPI_Reduce(&total_comms, &global_num_comms, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
            MPI_Reduce(&comm_bytes_needed, &global_bytes_needed, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

            if (rank == 0) {
                auto raw_comm_bytes = global_num_comms * max_border_size;
                auto overhead = 100 - 100. * global_bytes_needed / raw_comm_bytes;

                printf("ASYNC: took %d border transfers - %s (used %s - overhead %.1f%%)\n", global_num_comms, humanReadableSize(raw_comm_bytes).c_str(), humanReadableSize(global_bytes_needed).c_str(), overhead);
            }
        }

    private:
        int rank, size;
        int total_comms = 0;
        int comm_bytes_needed = 0;

        std::unique_ptr<uint8_t[]> mpi_buffer;
        std::unique_ptr<int[]> outstanding_updates;

        vector<AsyncPartition*> rasters;
        vector<update_fn> raster_update_fns;
};

template<typename Algo, typename E>
long resolveFlats_parallel_async(E& elev, SparsePartition<int>& inc, linearpart<typename Algo::FlowType>& flowDir, vector<vector<node>>& islands)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    int rank;
    MPI_Comm_rank(MCW, &rank);

    SparsePartition<int> higherGradient(nx, ny, 0);

    flowTowardsLower<Algo>(elev, flowDir, islands, inc);
    flowFromHigher<Algo>(elev, flowDir, islands, higherGradient);

    {
        AsyncRasterProcessor arp;

        arp.add(&inc, [&flowDir, &inc](bool new_top, std::vector<node>& border_diff, bool& top_updated, bool& bottom_updated) {
            //printf("rank %d: got low gradient update - %d cells\n", rank, border_diff.size());

            propagateBorderIncrements_async<Algo>(flowDir, inc, new_top, border_diff, top_updated, bottom_updated);
        });

        arp.add(&higherGradient, [&flowDir, &higherGradient](bool new_top, std::vector<node>& border_diff, bool& top_updated, bool& bottom_updated) {
            //printf("rank %d: got high gradient update - %d cells\n", rank, border_diff.size());

            propagateBorderIncrements_async<Algo>(flowDir, higherGradient, new_top, border_diff, top_updated, bottom_updated);
        });

        arp.run();
     }

    // Not all grid cells were resolved - pits remain
    // Remaining grid cells are unresolvable pits
    markPits<Algo>(elev, flowDir, islands, inc);

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
            Algo::SetFlow(flat.x, flat.y, flowDir, elev, inc);
    
            if (!Algo::HasFlow(flowDir.getData(flat.x, flat.y))) {
                localFlatsRemaining++;
            }
        }
    }

    flowDir.share();
    MPI_Allreduce(&localFlatsRemaining, &globalFlatsRemaining, 1, MPI_UINT64_T, MPI_SUM, MCW);

    auto hasFlowDirection = [&](const node& n) { return Algo::HasFlow(flowDir.getData(n.x, n.y)); };
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
