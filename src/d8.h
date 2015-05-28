//#include "linearpart.h"

//Write the slope information
void writeSlope(tdpartition *flowDir, tdpartition *elevDEM, tdpartition* slopefile);

//Open files, initialize grid memory....
int setdird8( char* demfile, char* pointfile, char *slopefile, char *flowfile, int useflowfile);

long setPosDir( tdpartition *elevDEM, tdpartition *flowDir, tdpartition *flow, int useflowfile);
long resolveflats( tdpartition *elevDEM, tdpartition *flowDir, queue<node> *que, bool &first);
//int resolveflats( tdpartition *elevDEM, tdpartition *flowDir);
