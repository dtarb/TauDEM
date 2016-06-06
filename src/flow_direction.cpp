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

size_t propagateIncrements(linearpart<short>& flowDir, SparsePartition<short>& inc, std::vector<node>& queue) {
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

    return numInc;
}

size_t propagateBorderIncrements(linearpart<short>& flowDir, SparsePartition<short>& inc)
{
    int nx = flowDir.getnx();
    int ny = flowDir.getny();

    struct pnode {
        int x;
        int y;
        short inc;

        pnode(int x, int y, short inc) : x(x), y(y), inc(inc) {}
        bool operator<(const struct pnode& b) const {
            return inc < b.inc;
        }
    };
    
    std::vector<pnode> queue;

    // Find the starting nodes at the edge of the raster
    //
    // FIXME: don't scan border if not needed
    int ignoredGhostCells = 0;

    for (auto y : {-1, ny}) {
        for(int x = 0; x < nx; x++) {
            short st = inc.getData(x, y);

            if (st == 0)
                continue;

            if (st == SHRT_MAX) {
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
                    queue.emplace_back(in, jn, st + 1);

                    // Here we set a negative increment if it's still pending
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

    // Sort queue by lowest increment
    std::sort(queue.begin(), queue.end());
    std::vector<pnode> newFlats;

    while (!queue.empty()) {
        for(pnode flat : queue) {
            // Skip if the increment was already set and it is lower 
            auto st = inc.getData(flat.x, flat.y);
            if (st > 0 && st <= flat.inc) {
                continue;
            }

            if (st < 0 && -st < flat.inc) {
                flat.inc = -st;
            }

            inc.setData(flat.x, flat.y, flat.inc);
            numChanged++;

            if (flat.inc == SHRT_MAX) {
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

                    if (flow == 0 && (neighInc == 0 || std::abs(neighInc) > flat.inc + 1)) {
                        newFlats.emplace_back(in, jn, flat.inc + 1);
                        inc.setData(in, jn, -(flat.inc + 1));
                    }
                }
            }
        }

        std::sort(newFlats.begin(), newFlats.end());
        queue.clear();
        queue.swap(newFlats);
    }

    if (abandonedCells > 0) {
        printf("warning: gave up propagating %zu cells because they were at upper limit (%d)\n", abandonedCells, SHRT_MAX);
    }

    return numChanged;
}
