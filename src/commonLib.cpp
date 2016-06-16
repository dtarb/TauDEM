/*  Taudem common library functions 

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


#include <cmath>
#include <cstdio>
#include <cstring>

#include <cstdint>
#include <cinttypes>
#include <queue>

#include "commonLib.h"

using std::queue;

//==================================

/*  Nameadd(..)  Utility for adding suffixes to file names prior to
   "." extension   */
int nameadd(char *full, char *arg, const char *suff)
{
    const char *ext, *extsuff;
    int nmain;

    ext=strrchr(arg,'.');
    extsuff=strrchr(suff,'.');
    
    if(ext == NULL)
    {
        nmain= strlen(arg);
        sprintf(full,"%s%s",arg,suff);
    }
    else
    {
        nmain = strlen(arg)-strlen(ext);
        strcpy(full,"");
        strncat(full,arg,nmain);
        strcat(full,suff);
        
        //  Only append original extension if suffix does not have an extension already
        if(extsuff == NULL)
            strcat(full,ext);  
    }

    return nmain;
}

//TODO - does this function go here, or in areadinf?

double prop(float a, int k, double dx1, double dy1) {

    double aref[10] = {-atan2(dy1, dx1), 0., -aref[0], (double) (0.5 * PI), PI - aref[2], (double) PI,
        PI + aref[2], (double) (1.5 * PI), 2. * PI - aref[2], (double) (2. * PI)};
    double p = 0.;
    if (k <= 0)k = k + 8; // DGT to guard against remainder calculations that give k=0
    if (k == 1 && a > PI)a = (float) (a - 2.0 * PI);
    if (a > aref[k - 1] && a < aref[k + 1]) {
        if (a > aref[k])
            p = (aref[k + 1] - a) / (aref[k + 1] - aref[k]);
        else
            p = (a - aref[k - 1]) / (aref[k] - aref[k - 1]);
    }
    if (p < 1e-5) return -1.;
    else return (p);
}

void initNeighborDinfup(tdpartition* neighbor, tdpartition* flowData, queue<node> *que,
        int nx, int ny, int useOutlets, int *outletsX, int *outletsY, long numOutlets) {
    //  Function to initialize the neighbor partition either whole partition or just upstream of outlets
    //  and place locations with no neighbors that drain to them on the que
    int i, j, k, in, jn;
    short tempShort;
    float angle, p;
    double tempdxc, tempdyc;


    node temp;
    if (useOutlets != 1) {
        //Count the contributing neighbors and put on queue
        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {


                //Initialize neighbor count to no data, but then 0 if flow direction is defined
                neighbor->setToNodata(i, j);
                if (!flowData->isNodata(i, j)) {
                    //Set contributing neighbors to 0 
                    neighbor->setData(i, j, (short) 0);
                    //Count number of contributing neighbors
                    for (k = 1; k <= 8; k++) {
                        in = i + d1[k];
                        jn = j + d2[k];
                        if (flowData->hasAccess(in, jn) && !flowData->isNodata(in, jn)) {
                            flowData->getData(in, jn, angle);
                            flowData->getdxdyc(jn, tempdxc, tempdyc);

                            p = prop(angle, (k + 4) % 8, tempdxc, tempdyc); //  if neighbor drains to me
                            if (p > 0.0)
                                neighbor->addToData(i, j, (short) 1);
                        }
                    }
                    if (neighbor->getData(i, j, tempShort) == 0) {
                        //Push nodes with no contributing neighbors on queue
                        temp.x = i;
                        temp.y = j;
                        que->push(temp);
                    }
                }
            }
        }
    }// If Outlets are specified
    else {
        //Put outlets on queue to be evalutated
        queue<node> toBeEvaled;
        for (i = 0; i < numOutlets; i++) {
            flowData->globalToLocal(outletsX[i], outletsY[i], temp.x, temp.y);
            if (flowData->isInPartition(temp.x, temp.y))
                toBeEvaled.push(temp);
        }

        //TODO - this is 100% linear partition dependent.
        //Create a packet for message passing
        int *bufferAbove = NULL;
        int *bufferBelow = NULL;
        int *bufferLeft = NULL;
        int *bufferRight = NULL;
        int* bufferTopLeft = NULL;
        int* bufferTopRight = NULL;
        int* bufferBottomLeft = NULL;
        int* bufferBottomRight = NULL;
        int* countA = new int;
        int* countB = new int;
        int* countL = new int;
        int* countR = new int;
        int* countTL = new int;
        int* countTR = new int;
        int* countBL = new int;
        int* countBR = new int;
        int *bufferAboveVisited = NULL;
        int *bufferBelowVisited = NULL;
        int *bufferLeftVisited = NULL;
        int *bufferRightVisited = NULL;
        int* bufferTopLeftVisited = NULL;
        int* bufferTopRightVisited = NULL;
        int* bufferBottomLeftVisited = NULL;
        int* bufferBottomRightVisited = NULL;

        int neighbourCount = neighbor->getNeighbourCount();
        int ** neighbourBuffers = NULL;
        int ** neighbourCountArr = NULL;
        int ** neighbourBuffersVisited = NULL;

        if (neighbourCount > 0) {
            neighbourBuffers = new int*[neighbourCount];
            neighbourCountArr = new int*[neighbourCount];
            neighbourBuffersVisited = new int*[neighbourCount];

            int neighbourIndex = 0;

            if (neighbor->hasTopNeighbour()) {
                bufferAbove = new int[nx];
                bufferAboveVisited = new int[nx];
                if (!bufferAbove || !bufferAboveVisited) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferAbove;
                neighbourCountArr[neighbourIndex] = countA;
                neighbourBuffersVisited[neighbourIndex] = bufferAboveVisited;

                ++neighbourIndex;
            }

            if (neighbor->hasBottomNeighbour()) {
                bufferBelow = new int[nx];
                bufferBelowVisited = new int[nx];
                if (!bufferBelow || !bufferBelowVisited) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferBelow;
                neighbourCountArr[neighbourIndex] = countB;
                neighbourBuffersVisited[neighbourIndex] = bufferBelowVisited;

                ++neighbourIndex;
            }

            if (neighbor->hasLeftNeighbour()) {
                bufferLeft = new int[ny];
                bufferLeftVisited = new int[ny];
                if (!bufferLeft || !bufferLeftVisited) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferLeft;
                neighbourCountArr[neighbourIndex] = countL;
                neighbourBuffersVisited[neighbourIndex] = bufferLeftVisited;

                ++neighbourIndex;
            }

            if (neighbor->hasRightNeighbour()) {
                bufferRight = new int[ny];
                bufferRightVisited = new int[ny];
                if (!bufferRight || !bufferRightVisited) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferRight;
                neighbourCountArr[neighbourIndex] = countR;
                neighbourBuffersVisited[neighbourIndex] = bufferRightVisited;

                ++neighbourIndex;
            }

            if (neighbor->hasTopLeftNeighbour()) {
                bufferTopLeft = new int;
                bufferTopLeftVisited = new int;
                neighbourBuffers[neighbourIndex] = bufferTopLeft;
                neighbourCountArr[neighbourIndex] = countTL;
                neighbourBuffersVisited[neighbourIndex] = bufferTopLeftVisited;

                ++neighbourIndex;
            }

            if (neighbor->hasTopRightNeighbour()) {
                bufferTopRight = new int;
                bufferTopRightVisited = new int;
                neighbourBuffers[neighbourIndex] = bufferTopRight;
                neighbourCountArr[neighbourIndex] = countTR;
                neighbourBuffersVisited[neighbourIndex] = bufferTopRightVisited;

                ++neighbourIndex;
            }


            if (neighbor->hasBottomLeftNeighbour()) {
                bufferBottomLeft = new int;
                bufferBottomLeftVisited = new int;
                neighbourBuffers[neighbourIndex] = bufferBottomLeft;
                neighbourCountArr[neighbourIndex] = countBL;
                neighbourBuffersVisited[neighbourIndex] = bufferBottomLeftVisited;

                ++neighbourIndex;
            }

            if (neighbor->hasBottomRightNeighbour()) {
                bufferBottomRight = new int;
                bufferBottomRightVisited = new int;
                neighbourBuffers[neighbourIndex] = bufferBottomRight;
                neighbourCountArr[neighbourIndex] = countBR;
                neighbourBuffersVisited[neighbourIndex] = bufferBottomRightVisited;
            }
        }

        bool finished = false;
        while (!finished) {
            *countA = 0;
            *countB = 0;
            *countL = 0;
            *countR = 0;
            *countTL = 0;
            *countTR = 0;
            *countBL = 0;
            *countBR = 0;

            if (bufferAboveVisited)
                memset(bufferAboveVisited, 0, sizeof (int) * nx);

            if (bufferBelowVisited)
                memset(bufferBelowVisited, 0, sizeof (int) * nx);

            if (bufferLeftVisited)
                memset(bufferLeftVisited, 0, sizeof (int) * ny);

            if (bufferRightVisited)
                memset(bufferRightVisited, 0, sizeof (int) * ny);

            if (bufferTopLeftVisited)
                *bufferTopLeftVisited = 0;

            if (bufferTopRightVisited)
                *bufferTopRightVisited = 0;

            if (bufferBottomLeftVisited)
                *bufferBottomLeftVisited = 0;

            if (bufferBottomRightVisited)
                *bufferBottomRightVisited = 0;

            while (!toBeEvaled.empty()) {
                temp = toBeEvaled.front();
                toBeEvaled.pop();
                i = temp.x;
                j = temp.y;
                // Only evaluate if cell hasn't been evaled yet
                if (neighbor->isNodata(i, j)) {
                    //Set contributing neighbors to 0
                    neighbor->setData(i, j, (short) 0);
                    //Count number of contributing neighbors
                    for (k = 1; k <= 8; k++) {
                        in = i + d1[k];
                        jn = j + d2[k];
                        if (flowData->hasAccess(in, jn) && !flowData->isNodata(in, jn)) {
                            flowData->getData(in, jn, angle);
                            flowData->getdxdyc(jn, tempdxc, tempdyc);
                            p = prop(angle, (k + 4) % 8, tempdxc, tempdyc);
                            if (p > 0.) {
                                if (in == -1 && jn == -1) {
                                    if (*bufferTopLeftVisited == 0) {
                                        bufferTopLeft[0] = -1;
                                        *countTL = 1;
                                        *bufferTopLeftVisited = 1;
                                    }
                                } else if (jn == -1 && in == nx) {
                                    if (*bufferTopRightVisited == 0) {
                                        bufferTopRight[0] = -1;
                                        *countTR = 1;
                                        *bufferTopRightVisited = 1;
                                    }
                                } else if (jn == ny && in == -1) {
                                    if (*bufferBottomLeftVisited == 0) {
                                        bufferBottomLeft[0] = -1;
                                        *countBL = 1;
                                        *bufferBottomLeftVisited = 1;
                                    }
                                } else if (jn == ny && in == nx) {
                                    if (*bufferBottomRightVisited == 0) {
                                        bufferBottomRight[0] = -1;
                                        *countBR = 1;
                                        *bufferBottomRightVisited = 1;
                                    }
                                } else if (jn == -1) {
                                    if (bufferAboveVisited[in] == 0) {
                                        bufferAbove[*countA] = in;
                                        *countA = *countA + 1;
                                        bufferAboveVisited[in] = 1;
                                    }
                                } else if (jn == ny) {
                                    if (bufferBelowVisited[in] == 0) {
                                        bufferBelow[*countB] = in;
                                        *countB = *countB + 1;
                                        bufferBelowVisited[in] = 1;
                                    }
                                } else if (in == -1) {
                                    if (bufferLeftVisited[jn] == 0) {
                                        bufferLeft[*countL] = jn;
                                        *countL = *countL + 1;
                                        bufferLeftVisited[jn] = 1;
                                    }
                                } else if (in == nx) {
                                    if (bufferRightVisited[jn] == 0) {
                                        bufferRight[*countR] = jn;
                                        *countR = *countR + 1;
                                        bufferRightVisited[jn] = 1;
                                    }
                                } else {
                                    temp.x = in;
                                    temp.y = jn;
                                    toBeEvaled.push(temp);
                                }

                                neighbor->addToData(i, j, (short) 1);
                            }
                        }
                    }
                    if (neighbor->getData(i, j, tempShort) == 0) {
                        //Push nodes with no contributing neighbors on queue
                        temp.x = i;
                        temp.y = j;
                        que->push(temp);
                    }
                }
            }

            finished = true;

            neighbor->transferPack(neighbourBuffers, neighbourCountArr);

            for (int i = 0; i < neighbourCount; i++) {
                if (*neighbourCountArr[i] > 0) {
                    finished = false;
                    break;
                }
            }

            // added to prevent duplications in toBeEvaled list
            bool isTopLeftAdded = false;
            bool isTopRightAdded = false;
            bool isBottomLeftAdded = false;
            bool isBottomRightAdded = false;

            if (neighbor->hasTopNeighbour()) {
                for (int k = 0; k < *countA; k++) {
                    temp.x = bufferAbove[k];
                    temp.y = 0;
                    toBeEvaled.push(temp);

                    if (temp.x == 0)
                        isTopLeftAdded = true;

                    if (temp.x == nx - 1)
                        isTopRightAdded = true;
                }
            }

            if (neighbor->hasBottomNeighbour()) {

                for (int k = 0; k < *countB; k++) {
                    temp.x = bufferBelow[k];
                    temp.y = ny - 1;
                    toBeEvaled.push(temp);

                    if (temp.x == 0)
                        isBottomLeftAdded = true;

                    if (temp.x == nx - 1)
                        isBottomRightAdded = true;
                }
            }
            
            if (neighbor->hasLeftNeighbour()) {
                for (int k = 0; k < *countL; k++) {
                    temp.x = 0;
                    temp.y = bufferLeft[k];

                    if (temp.y == 0 && !isTopLeftAdded) {
                        toBeEvaled.push(temp);
                        isTopLeftAdded = true;
                    } else if (temp.y == ny - 1 && !isBottomLeftAdded) {
                        toBeEvaled.push(temp);
                        isBottomLeftAdded = true;
                    } else if (temp.y != 0 && temp.y != ny - 1) {
                        toBeEvaled.push(temp);
                    }
                }
            }

            if (neighbor->hasRightNeighbour()) {
                for (int k = 0; k < *countR; k++) {
                    temp.x = nx - 1;
                    temp.y = bufferRight[k];

                    if (temp.y == 0 && !isTopRightAdded) {
                        toBeEvaled.push(temp);
                        isTopRightAdded = true;
                    } else if (temp.y == ny - 1 && !isBottomRightAdded) {
                        toBeEvaled.push(temp);
                        isBottomRightAdded = true;
                    } else if (temp.y != 0 && temp.y != ny - 1) {
                        toBeEvaled.push(temp);
                    }
                }
            }

            if (neighbor->hasTopLeftNeighbour()) {
                if (*countTL == 1 && !isTopLeftAdded) {
                    temp.x = 0;
                    temp.y = 0;
                    toBeEvaled.push(temp);
                    isTopLeftAdded = true;
                }
            }

            if (neighbor->hasTopRightNeighbour()) {
                if (*countTR == 1 && !isTopRightAdded) {
                    temp.x = nx - 1;
                    temp.y = 0;
                    toBeEvaled.push(temp);
                    isTopRightAdded = true;
                }
            }

            if (neighbor->hasBottomLeftNeighbour()) {
                if (*countBL == 1 && !isBottomLeftAdded) {
                    temp.x = 0;
                    temp.y = ny - 1;
                    toBeEvaled.push(temp);
                    isBottomLeftAdded = true;
                }
            }

            if (neighbor->hasBottomRightNeighbour()) {
                if (*countBR == 1 && !isBottomRightAdded) {
                    temp.x = nx - 1;
                    temp.y = ny - 1;
                    toBeEvaled.push(temp);
                    isBottomRightAdded = true;
                }
            }

            finished = neighbor->ringTerm(finished);
        }

        if (neighbourBuffers) {
            for (int i = 0; i < neighbourCount; i++) {
                delete [] neighbourBuffers[i];
            }

            delete[] neighbourBuffers;
        }

        if (neighbourCountArr) {
            delete[] neighbourCountArr;
        }

        delete countA;
        delete countB;
        delete countL;
        delete countR;
        delete countTL;
        delete countTR;
        delete countBL;
        delete countBR;
    }
}

void initNeighborD8up(tdpartition* neighbor, tdpartition* flowData, queue<node> *que,
        int nx, int ny, int useOutlets, int *outletsX, int *outletsY, long numOutlets) {
    //  Function to initialize the neighbor partition either whole partition or just upstream of outlets
    //  and place locations with no neighbors that drain to them on the que
    int i, j, k, in, jn;
    short tempShort;
    //float tempFloat,angle,p;
    node temp;
    if (useOutlets != 1) {
        //Count the contributing neighbors and put on queue
        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                //Initialize neighbor count to no data, but then 0 if flow direction is defined
                neighbor->setToNodata(i, j);
                if (!flowData->isNodata(i, j)) {
                    //Set contributing neighbors to 0 
                    neighbor->setData(i, j, (short) 0);
                    //Count number of contributing neighbors
                    for (k = 1; k <= 8; k++) {
                        in = i + d1[k];
                        jn = j + d2[k];
                        if (flowData->hasAccess(in, jn) && !flowData->isNodata(in, jn)) {
                            flowData->getData(in, jn, tempShort);
                            if (tempShort - k == 4 || tempShort - k == -4)
                                neighbor->addToData(i, j, (short) 1);
                        }
                    }
                    if (neighbor->getData(i, j, tempShort) == 0) {
                        //Push nodes with no contributing neighbors on queue
                        temp.x = i;
                        temp.y = j;
                        que->push(temp);
                    }
                }
            }
        }
    }// If Outlets are specified
    else {
        //Put outlets on queue to be evalutated
        queue<node> toBeEvaled;
        for (i = 0; i < numOutlets; i++) {
            flowData->globalToLocal(outletsX[i], outletsY[i], temp.x, temp.y);
            if (flowData->isInPartition(temp.x, temp.y))
                toBeEvaled.push(temp);
        }

        //TODO - this is 100% linear partition dependent.
        //Create a packet for message passing
        int *bufferAbove = NULL;
        int *bufferBelow = NULL;
        int *bufferLeft = NULL;
        int *bufferRight = NULL;
        int* bufferTopLeft = NULL;
        int* bufferTopRight = NULL;
        int* bufferBottomLeft = NULL;
        int* bufferBottomRight = NULL;
        int countA, countB, countL, countR, countTL, countTR, countBL, countBR;

        int neighbourCount = neighbor->getNeighbourCount();
        int ** neighbourBuffers = NULL;
        int ** neighbourCountArr = NULL;

        if (neighbourCount > 0) {
            neighbourBuffers = new int*[neighbourCount];
            neighbourCountArr = new int*[neighbourCount];

            int neighbourIndex = 0;

            if (neighbor->hasTopNeighbour()) {
                bufferAbove = new int[nx];
                if (!bufferAbove) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferAbove;
                neighbourCountArr[neighbourIndex] = &countA;

                ++neighbourIndex;
            }

            if (neighbor->hasBottomNeighbour()) {
                bufferBelow = new int[nx];
                if (!bufferBelow) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferBelow;
                neighbourCountArr[neighbourIndex] = &countB;

                ++neighbourIndex;
            }

            if (neighbor->hasLeftNeighbour()) {
                bufferLeft = new int[ny];
                if (!bufferLeft) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferLeft;
                neighbourCountArr[neighbourIndex] = &countL;

                ++neighbourIndex;
            }

            if (neighbor->hasRightNeighbour()) {
                bufferRight = new int[ny];
                if (!bufferRight) {
                    printf("Error allocating memory\n");
                    MPI_Abort(MCW, 5);
                }
                neighbourBuffers[neighbourIndex] = bufferRight;
                neighbourCountArr[neighbourIndex] = &countR;

                ++neighbourIndex;
            }

            if (neighbor->hasTopLeftNeighbour()) {
                bufferTopLeft = new int;
                neighbourBuffers[neighbourIndex] = bufferTopLeft;
                neighbourCountArr[neighbourIndex] = &countTL;

                ++neighbourIndex;
            }

            if (neighbor->hasTopRightNeighbour()) {
                bufferTopRight = new int;
                neighbourBuffers[neighbourIndex] = bufferTopRight;
                neighbourCountArr[neighbourIndex] = &countTR;

                ++neighbourIndex;
            }


            if (neighbor->hasBottomLeftNeighbour()) {
                bufferBottomLeft = new int;
                neighbourBuffers[neighbourIndex] = bufferBottomLeft;
                neighbourCountArr[neighbourIndex] = &countBL;

                ++neighbourIndex;
            }

            if (neighbor->hasBottomRightNeighbour()) {
                bufferBottomRight = new int;
                neighbourBuffers[neighbourIndex] = bufferBottomRight;
                neighbourCountArr[neighbourIndex] = &countBR;
            }
        }

        bool finished = false;
        while (!finished) {
            countA = 0;
            countB = 0;
            countL = 0;
            countR = 0;
            countTL = 0;
            countTR = 0;
            countBL = 0;
            countBR = 0;

            while (!toBeEvaled.empty()) {
                temp = toBeEvaled.front();
                toBeEvaled.pop();
                i = temp.x;
                j = temp.y;
                // Only evaluate if cell hasn't been evaled yet
                if (neighbor->isNodata(i, j)) {
                    //Set contributing neighbors to 0
                    neighbor->setData(i, j, (short) 0);
                    //Count number of contributing neighbors
                    for (k = 1; k <= 8; k++) {
                        in = i + d1[k];
                        jn = j + d2[k];
                        if (flowData->hasAccess(in, jn) && !flowData->isNodata(in, jn)) {
                            flowData->getData(in, jn, tempShort);
                            //  Does neighbor drain to me
                            if (tempShort - k == 4 || tempShort - k == -4) {
                                if (in == -1 && jn == -1) {
                                    bufferTopLeft[0] = -1;
                                    countTL = 1;
                                } else if (jn == -1 && in == nx) {
                                    bufferTopRight[0] = -1;
                                    countTR = 1;
                                } else if (jn == ny && in == -1) {
                                    bufferBottomLeft[0] = -1;
                                    countBL = 1;
                                } else if (jn == ny && in == nx) {
                                    bufferBottomRight[0] = -1;
                                    countBR = 1;
                                } else if (jn == -1) {
                                    bufferAbove[countA] = in;
                                    countA += 1;
                                } else if (jn == ny) {
                                    bufferBelow[countB] = in;
                                    countB += 1;
                                } else if (in == -1) {
                                    bufferLeft[countL] = jn;
                                    countL += 1;
                                } else if (in == nx) {
                                    bufferRight[countR] = jn;
                                    countR += 1;
                                } else {
                                    temp.x = in;
                                    temp.y = jn;
                                    toBeEvaled.push(temp);
                                }

                                neighbor->addToData(i, j, (short) 1);
                            }
                        }
                    }
                    if (neighbor->getData(i, j, tempShort) == 0) {
                        //Push nodes with no contributing neighbors on queue
                        temp.x = i;
                        temp.y = j;
                        que->push(temp);
                    }
                }
            }
            finished = true;

            neighbor->transferPack(neighbourBuffers, neighbourCountArr);

            for (int i = 0; i < neighbourCount; i++) {
                if (*neighbourCountArr[i] > 0) {
                    finished = false;
                    break;
                }
            }

            bool isTopLeftAdded = false;
            bool isTopRightAdded = false;
            bool isBottomLeftAdded = false;
            bool isBottomRightAdded = false;

            if (neighbor->hasTopNeighbour()) {
                for (k = 0; k < countA; k++) {
                    temp.x = bufferAbove[k];
                    temp.y = 0;
                    toBeEvaled.push(temp);

                    if (temp.x == 0)
                        isTopLeftAdded = true;

                    if (temp.x == nx - 1)
                        isTopRightAdded = true;
                }
            }

            if (neighbor->hasBottomNeighbour()) {
                for (k = 0; k < countB; k++) {
                    temp.x = bufferBelow[k];
                    temp.y = ny - 1;
                    toBeEvaled.push(temp);

                    if (temp.x == 0)
                        isBottomLeftAdded = true;

                    if (temp.x == nx - 1)
                        isBottomRightAdded = true;
                }
            }

            if (neighbor->hasLeftNeighbour()) {
                for (k = 0; k < countL; k++) {
                    temp.x = 0;
                    temp.y = bufferLeft[k];

                    if (temp.y == 0 && !isTopLeftAdded) {
                        toBeEvaled.push(temp);
                        isTopLeftAdded = true;
                    } else if (temp.y == ny - 1 && !isBottomLeftAdded) {
                        toBeEvaled.push(temp);
                        isBottomLeftAdded = true;
                    } else if (temp.y != 0 && temp.y != ny - 1) {
                        toBeEvaled.push(temp);
                    }
                }
            }

            if (neighbor->hasRightNeighbour()) {
                for (k = 0; k < countR; k++) {
                    temp.x = nx - 1;
                    temp.y = bufferRight[k];

                    if (temp.y == 0 && !isTopRightAdded) {
                        toBeEvaled.push(temp);
                        isTopRightAdded = true;
                    } else if (temp.y == ny - 1 && !isBottomRightAdded) {
                        toBeEvaled.push(temp);
                        isBottomRightAdded = true;
                    } else if (temp.y != 0 && temp.y != ny - 1) {
                        toBeEvaled.push(temp);
                    }
                }
            }

            if (neighbor->hasTopLeftNeighbour()) {
                if (countTL == 1 && !isTopLeftAdded) {
                    temp.x = 0;
                    temp.y = 0;
                    toBeEvaled.push(temp);
                    isTopLeftAdded = true;
                }
            }

            if (neighbor->hasTopRightNeighbour()) {
                if (countTR == 1 && !isTopRightAdded) {
                    temp.x = nx - 1;
                    temp.y = 0;
                    toBeEvaled.push(temp);
                    isTopRightAdded = true;
                }
            }

            if (neighbor->hasBottomLeftNeighbour()) {
                if (countBL == 1 && !isBottomLeftAdded) {
                    temp.x = 0;
                    temp.y = ny - 1;
                    toBeEvaled.push(temp);
                    isBottomLeftAdded = true;
                }
            }

            if (neighbor->hasBottomRightNeighbour()) {
                if (countBR == 1 && !isBottomRightAdded) {
                    temp.x = nx - 1;
                    temp.y = ny - 1;
                    toBeEvaled.push(temp);
                    isBottomRightAdded = true;
                }
            }

            finished = neighbor->ringTerm(finished);
        }

        if (neighbourBuffers) {
            for (int i = 0; i < neighbourCount; i++) {
                delete [] neighbourBuffers[i];
            }

            delete[] neighbourBuffers;
        }

        if (neighbourCountArr) {
            delete[] neighbourCountArr;
        }
    }
}

//returns true iff cell at [nrow][ncol] points to cell at [row][col]

bool pointsToMe(long col, long row, long ncol, long nrow, tdpartition *dirData) {
    short d;
    if (!dirData->hasAccess(ncol, nrow) || dirData->isNodata(ncol, nrow)) {
        return false;
    }
    d = dirData->getData(ncol, nrow, d);
    if (nrow + d2[d] == row && ncol + d1[d] == col) {
        return true;
    }
    return false;
}

//get extension from OGR vector file
//get layername if not provided by user
std::string getLayername(std::string input_path) {
    const size_t slash_pos = input_path.find_last_of("/\\");
    const size_t dot_pos = input_path.find_last_of(".");

    return input_path.substr(slash_pos + 1, dot_pos);
}

//get ogr driver index for writing shapefile
const char *getOGRdrivername(char *datasrcnew){
    const char *ogrextension_list[5] = {".sqlite",".shp",".json",".kml",".geojson"};  // extension list --can add more 
    const char *ogrdriver_code[5] = {"SQLite","ESRI Shapefile","GeoJSON","KML","GeoJSON"};   //  code list -- can add more
    size_t extension_num=5;
    char *ext; 
    int index = 1; //default is ESRI shapefile
    ext = strrchr(datasrcnew, '.'); 
    if(!ext){

        index=1; //  if no extension then writing will be ESRI shapefile
    }
    else
    {
        //  convert to lower case for matching
        for(int i = 0; ext[i]; i++){
            ext[i] = tolower(ext[i]);
        }
        // if extension matches then set driver
        for (size_t i = 0; i < extension_num; i++) {
            if (strcmp(ext,ogrextension_list[i])==0) {
                index=i; //get the index where extension of the outputfile matches with the extensionlist 
                break;
            }
        }

    }

    const  char *drivername;
    drivername=ogrdriver_code[index];
    return drivername;
}

void getlayerfail(OGRDataSourceH hDS1,char * outletsds, int outletslyr){
    int nlayer = OGR_DS_GetLayerCount(hDS1);
    const char * lname;
    printf("Error opening datasource layer in %s\n",outletsds);
    printf("This datasource contains the following %d layers.\n",nlayer);
    for(int i=0;i<nlayer;i++){
        OGRLayerH hLayer1 = OGR_DS_GetLayer(hDS1,i);
        lname=OGR_L_GetName(hLayer1);
        OGRwkbGeometryType gtype;
        gtype=OGR_L_GetGeomType(hLayer1);
        const char *gtype_name[8] = {"Unknown","Point","LineString","Polygon","MultiPoint","MultiLineString","MultiPolygon","GeometryCollection"};  // extension list --can add more 
        printf("%d: %s, %s\n",i,lname,gtype_name[gtype]);
        // TODO print interpretive name
    }
    exit(1);
}

std::string humanReadableSize(uint64_t size)
{
    static const char     *sizes[]  = { "PB", "TB", "GB", "MB", "KB", "B" };
    static const uint64_t petabytes = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;

    std::string result;
    result.reserve(20);

    uint64_t multiplier = petabytes;

    for (size_t i = 0; i < std::extent<decltype(sizes)>::value; i++, multiplier /= 1024)
    {   
        if (size < multiplier)
            continue;

        if (size % multiplier == 0)
            sprintf(&result[0], "%" PRIu64 " %s", size / multiplier, sizes[i]);
        else
            sprintf(&result[0], "%.1f %s", (float) size / multiplier, sizes[i]);
        return result;
    }

    result.append("0");

    return result;
}
