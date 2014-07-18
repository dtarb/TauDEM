#ifndef D8_H
#define D8_H

#include "linearpart.h"

//Write the slope information
void writeSlope(tdpartition *flowDir, tdpartition *elevDEM, tdpartition* slopefile);

//Open files, initialize grid memory....
int setdird8(char* demfile, char* pointfile, char *slopefile, char *flowfile, int useflowfile, int prow, int pcol);

long setPosDir(linearpart<float>& elevDEM, linearpart<short>& flowDir, linearpart<long>& flow, int useflowfile);
long resolveflats(linearpart<float>& elevDEM, linearpart<short>& flowDir, std::vector<node>& flats);

#endif
