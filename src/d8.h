#ifndef D8_H
#define D8_H

#include <vector>

#include "sparsepartition.h"
#include "linearpart.h"

//Write the slope information
void writeSlope(tdpartition *flowDir, tdpartition *elevDEM, tdpartition* slopefile);

//Open files, initialize grid memory....
int setdird8(char* demfile, char* pointfile, char *slopefile, char *flowfile, int useflowfile);

long setPosDir(linearpart<float>& elevDEM, linearpart<short>& flowDir);

#endif
