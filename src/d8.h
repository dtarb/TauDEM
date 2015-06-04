#ifndef D8_H
#define D8_H

#include <vector>

#include "blockpartition.h"
#include "linearpart.h"

//Write the slope information
void writeSlope(tdpartition *flowDir, tdpartition *elevDEM, tdpartition* slopefile);

//Open files, initialize grid memory....
int setdird8(char* demfile, char* pointfile, char *slopefile, char *flowfile, int useflowfile, int prow, int pcol);

long setPosDir(linearpart<float>& elevDEM, linearpart<short>& flowDir, BlockPartition<long>& flow, int useflowfile);


#endif
