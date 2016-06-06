#include <algorithm>

#include "flow_direction.h"

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

size_t propagateIncrements(linearpart<short>& flowDir, SparsePartition<int>& inc, std::vector<node>& queue) {
    size_t numInc = 0;
    int st = 1;
    
    std::vector<node> newFlats;
    while (!queue.empty()) {
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

        queue.clear();
        queue.swap(newFlats);
        st++;
    }

    if (st < 0) {
        printf("WARNING: increment overflow during propagation (st = %d)\n", st);
    }

    return numInc;
}

size_t propagateBorderIncrements(linearpart<short>& flowDir, SparsePartition<int>& inc)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    std::vector<node> queue;

    // Find the starting nodes at the edge of the raster
    //
    // FIXME: don't scan border if not needed
    int ignoredGhostCells = 0;

    for (auto y : {-1, ny}) {
        for(int x = 0; x < nx; x++) {
            int st = inc.getData(x, y);

            if (st == 0)
                continue;

            if (st == INT_MAX) {
                ignoredGhostCells++;
                continue;
            }

            auto jn = y == -1 ? 0 : ny - 1;

            for (auto in : {x-1, x, x+1}) {
                if (!flowDir.isInPartition(in, jn))
                    continue;

                bool noFlow = flowDir.getData(in, jn) == 0;
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
    }

    if (ignoredGhostCells > 0) {
       printf("warning: ignored %d ghost cells which were at upper limit (%d)\n", ignoredGhostCells, SHRT_MAX);
    }

    size_t numChanged = 0;
    size_t abandonedCells = 0;

    std::vector<node> newFlats;

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

            if (st == INT_MAX) {
                abandonedCells++;
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

                    if (flow == 0 && (neighInc == 0 || std::abs(neighInc) > st + 1)) {
                        // If neighbor increment is positive, we are overriding a larger increment
                        // and it is not yet in the queue
                        if (neighInc >= 0) {
                           newFlats.emplace_back(in, jn);
                        }

                        inc.setData(in, jn, -(st + 1));
                    }
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
