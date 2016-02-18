/***********************************************************/
/*                                                         */
/* tardem.h                                                */
/*                                                         */
/* TARDEM callable functions -- header file                */
/*                                                         */
/*                                                         */
/* David Tarboton   May 23, 2010                           */
/* Utah Water Research Laboratory                          */
/* Utah State University                                   */
/* Logan, UT 84322-8200                                    */
/* http://www.engineering.usu.edu/dtarb/                   */
/*                                                         */
/***********************************************************/


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


//  Tardem functions    //

//int nameadd(char *full,char *arg,char *suff);
/*  Adds the suffix from the 3rd argument (input) to the name in the second argument (input), 
returned as first argument (output). Suffix is added prior to the extension preceded by the 
last "." if any  */

//int flood(char *demfile, char *newfile, char * flowfile, short useflowfile, 
//		  char *newflowfile, bool LoadIntoRam = true);
int flood(char *demfile, char *newfile, char * flowfile, short useflowfile);
/*  Removes pits using the flooding algorithm.  Original data in demfile (input), 
  pit filled elevations in newfile (output). */

int setdird8(char *demfile, char *pointfile, char *slopefile, char * flowFile,
			 short useflowfile);
//int setdird8(char *demfile, char *pointfile, char *slopefile, char * flowFile,
//			 short useflowfile, bool LoadIntoRam = true); 
/*  Sets D8 flow directions.  Elevation data in demfile (input),  D8 flow directions in 
pointfile (output).  D8 slopes on slopefile (output).  */ 

int setdir( char* demfile, char* angfile, char *slopefile, char *flowfile, int useflowfile);
//int setdir(char *demfile, char *angfile, char *slopefile, char * flowfile, 
//			 short useflowfile, bool LoadIntoMemory = true);
/*  Sets Dinf flow directions.  Elevation data in demfile (input),  Dinf flow angles in 
angfile (output).  Dinf slopes on slopefile (output).  */ 

//int aread8(char *pfile, char *afile, double *x, double *y, long nxy, int doall, 
//		   char *wfile, int usew, int contcheck);
int aread8(char *pfile,char *afile, char *shfile, char *wfile, int useOutlets,
		   int usew, int contcheck);
/*  Computes D8 contributing areas, using D8 pointers from pfile (input).  Result is returned
in afile (output).  x and y are outlet coordinates which may be optionally supplied.  
doall is flag:  0 means use outlet coordinates as outlet, 1 means compute whole grid.
wfile is optional weight file for area computations.  
usew is flag:  0 means do not use weight file, 1 means use weight file
contcheck is flag:  0 means do not check for edge contamination, 
                    1 means check for edge contamination  */

//int area(char *pfile, char *afile, double *x, double *y, long nxy, int doall,
//		 char *wfile, int usew, int contcheck,bool inRam = true);
int area(char *pfile, char *afile, char *shfile, char *wfile, int useOutlets, 
		  int usew, int contcheck);
/*  Computes Dinf contributing areas, using Dinf angles from pfile (input).  Result is returned
in afile (output).  x and y are outlet coordinates which may be optionally supplied.  
doall is flag:  0 means use outlet coordinates as outlet, 1 means compute whole grid.
wfile is optional weight file for area computations.  
usew is flag:  0 means do not use weight file, 1 means use weight file
contcheck is flag:  0 means do not check for edge contamination, 
                    1 means check for edge contamination  */

int gridnet(char *pfile, char *plenfile, char *tlenfile, char *gordfile, char *maskfile,
		char* datasrc,char* lyrname,int uselyrname,int lyrno, int useMask, int useOutlets, int thresh);
//int gridnet(char *pfile,char *plenfile,char *tlenfile,char *gordfile,char *maskfile,
//			double *x, double *y,long nxy, int useMask, int useOutlets, int thresh);
/*  Uses D8 flow drections in pfile (input) with optional basin maskfile (input)
to compute grid of longest flow length upstream of each point, plenfile (output),
total path length upstream of each point, tlenfile (output) and grid strahler order
gordfile (output).
x, y are optional outlet coordinates.
mask is a flag with value 0 if there is no mask file, 1 if there is.
outlet is a flag with value 0 if no outlet is specifies, 1 if it is.
thresh is the mask threshold used in >= test.  */

int gagewatershed(char *pfile, char *wfile, char* datasrc,char* lyrname,int uselyrname,int lyrno, char *idfile, int writeid);


int source(char *areafile,char *slopefile,char *plenfile,char *dirfile, 
		   char *srcfile, char *elvfile, char *gordfile, char *scafile,
		   char *fdrfile, int ipar,float *p, int nxy, double *x, double *y, 
		   int contcheck, int dropan, int masksca);
/*  Defines a channel network raster grid based on a channel network delineation method
areafile (input): grid with D8 contributing area 'ad8'.
slopefile (input): slopes used only in method 2 ('slp' recommended)
plenfile (input): path length grid used only in method 3 'plen'
pfile (input): D8 flow directions file 'p'
srcfile (output):  Grid that is raster channel network
elvfile (input):  Elevation data used only in method 4 'fel'
gordfile (input):  Grid order file used only in method 5 'gord'
scafile (input):   Grid with DInf contributing area 'sca' used in methods 2 and 3.
fdrfile (input):   Existing channel network file used in method 6 'fdrn'
method (input):  integer designating method.  
  1. Catchment area threshold A >= p[0]. 
  2. Area-Slope threshold A S^p[1] >= p[0]. 
  3. Length-Area threshold A >= p[0] L^p[1]. Here L is the maximum drainage length to each cell 
  4. Accumulation area of upward curved grid cells.  The DEM is first smoothed by a kernel 
  with value p[0] at its center, p[1] on its edges, and p[2] on diagonals.  The Peuker and 
  Douglas (1975) method is then used to identify upwards curved grid cells and contributing 
  area computed using only these cells.  A threshold, Auc >= p[3] on these cells is used to 
  map the channel network. 
  5. Grid order threshold  O >= p[0]. 
  6. Use existing channel network
p[4] (input):  4 element array containing parameters
nout (input):  number of outlets
xr, yr (input):  arrays of x and y coordinates of outlet.   
contcheck:  flag to indicate edge contamination checking  
dropan:  flag to indicate call by dropan or not
masksca:  flag to indicate that locations with sca no data are to be excluded
*/
//int netsetup(char *fnprefix, char *pfile, char *srcfile, char *ordfile, 
//			 char *ad8file, char *elevfile, char *treefile, char *coordfile, 
//			 double *xnode, double *ynode, int nxy, long usetrace, long *idnodes);

int  netsetup(char *pfile,char *srcfile,char *ordfile,char *ad8file,char *elevfile,char *treefile, char *coordfile, 
			 char *outletsds, char *lyrname,int uselayername, int lyrno, char *wfile, char *streamnetsrc, char *streamnetlyr,long useOutlets, long ordert, bool verbose) ; 

//int netsetup(char *demfile, int method,
//			 float p1,float p2,float p3,float p4, long xr,long yr, int contcheck);
/*  Extracts channel network from grids and saves in vector format.
areafile (input):  grid with D8 contributing area 'ad8'.
slopefile (input):  slopes used only in method 2 ('slp' recommended)
plenfile (input):  path length grid used only in method 3 'plen'
pfile (input):  D8 flow directions file 'p'
srcfile (output):  Grid of contributing area of channel network sources
elevfile (input):  Elevation data used only in method 4 'fel'
gordfile (input):  Grid order file used only in method 5 'gord'
treefile (output):  list of links in channel network tree
coordfile (output):  list of coordinates in channel network tree
ordfile (output): grid of channel network Strahler order
scafile (input): grid with Dinf contributing area 'sca' used in methods 2 and 3.
method (input): integer designating method.  
  1. Catchment area threshold A >= p[0]. 
  2. Area-Slope threshold A S^p[1] >= p[0]. 
  3. Length-Area threshold A >= p[0] L^p[1]. Here L is the maximum drainage length to each cell 
  4. Accumulation area of upward curved grid cells.  The DEM is first smoothed by a kernel 
  with value p[0] at its center, p[1] on its edges, and p[2] on diagonals.  The Peuker and 
  Douglas (1975) method is then used to identify upwards curved grid cells and contributing 
  area computed using only these cells.  A threshold, Auc >= p[3] on these cells is used to 
  map the channel network. 
  5. Grid order threshold  O >= p[0]. 
p[4] (input):  4 element array containing parameters
xr, yr (input):  x and y coordinates of outlet.   
contcheck:  flag to indicate edge contamination checking  */
int netexC(short **dir, int **area, char *treefile, char *coordfile, char *ordfile, int nx, int ny, int itresh, 
		   int icr, int &icend, double dx, double dy, double *bndbox,double csize, int filetype, int err, short *inodes, 
		   short *jnodes,long nnodes, long *idnodes);
void netpropC(short **dir, int **area, float **elev, char *coordfile, int icr, int icmax,
			   double dx, double dy,int nx, int ny, 
			   double *bndbox, double csize,int err);
bool isnode2C(int i , int j , short *inodes, short *jnodes, int nnodes,int &nodeno);
bool isnodeC(int mnext, int mag, int i , int j , short *inodes, short *jnodes, int nnodes);
bool strtC(int i,int j,int **area,short **dir, long nx, long ny, int igx, int igy,int itresh);

int dropan(char *areafile,char *slopefile,char *plenfile,char *dirfile, 
		   char *srcfile, char *elevfile, char *gordfile, char *scafile,
		   char *fdrfile, int ipar,float *p, int nxy, double *x, double *y, 
		   int contcheck,int nthresh,int *thresholds, int *n1, int *n2,
		   float *s1, float *s2, float *s1sq, float *s2sq, float *length,long masksca);

//void arclinks(char *coordfile,char *treefile,char *outfile, int *ilink, float *amin);
/*  Reads tree data files and outputs Arc Export format file.  An element for each link
coordfile (input):  list of coordinates in channel network '*coord.dat'
treefile (input):  list of links in channel network tree '*tree.dat'
outfile (output):  Arc Export format file '*.e00'
ilink (input):  Link number to define outlet if a subset is required (use 0 for whole tree)
amin (input):  Area threshold to prune grid (use 0 for whole tree)   */

//void arcstreams(char *coordfile,char *treefile,char *outfile, int *ilink, float *amin);
/*  Reads tree data files and outputs Arc Export format file, an elemnt for each stream.
coordfile (input):  list of coordinates in channel network '*coord.dat'
treefile (input):  list of links in channel network tree '*tree.dat'
outfile (output):  Arc Export format file '*.e00'
ilink (input):  Link number to define outlet if a subset is required (use 0 for whole tree)
amin (input):  Area threshold to prune grid (use 0 for whole tree)   */

int subbasinsetup(char *pfile, char *wfile,char *treefile,char *coordfile,char *shpfile, int ordert,int subbno);
/*  Sets up subwatersheds each with their unique number
pfile (input): grid of D8 flow directions 'p'
wfile (output):  grid of watershed identifiers 'w'
treefile (input):  list of links in channel network tree '*tree.dat'
coordfile (input):  list of coordinates in channel network '*coord.dat'
shpfile(output):  shape file of resultant channel network '*.shp'
ordert (input):  Strahler order threshold for watershed delineation.   */

int depgrd(char *angfile, char *dgfile, char *depfile);
/* Computes dependence.  Dependence is defined as the contribution to specific catchment 
area at one or a set of grid cells, from each grid cell.
angfile (input):  The grid of Dinf angles.
dgfile (input):  The set of cells whose dependence is to be evaluated.  This is a grid of 1's and 0's.
depfile (output):  The grid containing the dependence function.  */
  
//int atanbgrid(char *slopefile,char *areafile,char *atanbfile, int diskbasedflag);
int atanbgrid(char *slopefile,char *areafile,char *atanbfile);
/*  Computes slope to area ratio (inverse of TopModel wetness index S/A at each point
   Inverse to avoid divide by 0 for 0 slope    
	slopefile - the slope grid
	areafile - the specific catchment area grid
	atanbfile - the result S/A grid.  */
//int distgrid(char *pfile, char *srcfile, char *distfile, int thresh, int diskbasedflag);
int distgrid(char *pfile, char *srcfile, char *distfile, int thresh);

/*  Function to compute hillslope flow distances

    Takes as input 2 grid files, 
	pfile - the d8 directions
	srcfile - the channel definition file
	thresh is the integer value used to define channels in srcfile
	(a greater than or equal to test is used)

    distfile is the output grid of distances to channels
 */
int topsetup(char *atanbfile, char *distfile, char *raincoordfile, char *flowcoordfile, 
			 char *treefile, char *coordfile, char *pfile, char *wfile, char *elevfile, 
			 char *trigridfile, char *anngridfile, char *modelspcfile, char *basinparfile, char *reachareafile,
			 char tfilenamelist[16][1024], char gfilenamelist[16][1024], 
			 int ntgpairs, int methods[16], float fixedpval[16],int calibp[16], 
			 int tgpair[16], int tcol[16], float ncpar[7],int dotrigrid, char *moelfile, short ver, 
			 char *nodelinkfile, int markthresh, char *dcatchfile, int usemoel, char *cefile) ;


//int dmarea(char *angfile, char *adecfile, char *dmfile, double *x, double *y, long nxy, 
//		 char *wfile, int usew, int contcheck);
int dmarea(char* angfile,char* adecfile,char* dmfile,char* datasrc,char* lyrname,int uselyrname,int lyrno,char* wfile,
		   int useOutlets,int usew,int contcheck);

//int dsaccum(char *angfile,char *wgfile, char *depfile, char *maxfile, int touch, float wgval);
int dsaccum(char *angfile,char *wgfile, char *raccfile, char *dmaxfile);

//int tlaccum(char *pfile, char *wfile, char *tcfile, char *tlafile, char *depfile, 
//			char *cfile, char *coutfile,
//			double  *x, double *y, int nxy,   int usec, int contcheck);
int tlaccum(char *angfile, char *tsupfile, char *tcfile, char *tlafile, char *depfile, 
			char *cinfile, char *coutfile, char* datasrc,char* lyrname,int uselyrname,int lyrno, int useOutlets, int usec, 
			int contcheck);
/*int tlaccum(char *pfile, char *wfile, char *tcfile, char *tlafile, char *depfile, 
			char *cfile, char *coutfile,
			double  *x, double *y, int nxy,   int usec, int contcheck, ItkCallback * callback = NULL);*/


int dsllArea(char* angfile,char* ctptfile,char* dmfile,char* datasrc,char* lyrname,int uselyrname,int lyrno,char* qfile, char* dgfile, 
		   int useOutlets, int contcheck, float cSol);

/*int dsllArea(char *pfile, char *afile, char *dmfile, double *x, double *y,  long nxy, 
		 char *wfile, char *indicatorFile, int contcheck, float cSol,ItkCallback * callback = NULL);  */

//int outletstosrc(char *pfile, char *srcfile,char *ad8file, long *xnode, long *ynode, int nxy,int max_dist,long *ismoved,  long *dist_moved);
int outletstosrc(char *pfile, char *srcfile,char *outletshpfile, char *outletmovedfile, int maxdist);
//This function duplicates a part of the netsetup() to trace the oulets to the src, if their coordinates
///are not on the river.
//gridconversion function:Ajay (2003-2004)
int gridconvert(char *ipFile,char* opFile,int ipdataformat,int opdataformat,int opfiletype);

int sloped(char *pfile,char* felfile,char* slpdfile, double dn);
//  Function to compute slope in a D8 downslope direction over a distance dn.

//This function return the statistics of a grid, demarked by a sample region represented by an index frid
int gridstat(char *grdname /*Path to grid to compute statistics*/, char *idgrid/*Path to the idgrid (sampling region)*/, long ind, double *statlist/*this array hold all the statistics information*/);
// *grdname ->Path to grid to compute statistics 
// *idgrid  ->Path to the idgrid (sampling region) 
// ind -> index number by which the sampling region is marked
// *statlist ->this array hold all the statistics information

//This function returns the distance to stream raster along Dinf flow directions
int disttostreamgrd(char *angfile, char *wfile, char *srcfile, char *dtsfile, int method, int useweight, int concheck);

//This function returns the rise to ridge along Dinf flow directions
int risetoridgegrd(char *angfile, char *felfile, char *rtrfile, int method, int concheck, float thresh);

//This function returns the vertical distance to stream along Dinf flow directions
int droptostreamgrd(char *angfile, char *felfile, char *srcfile, char *vsfile, int method);

//This functions returns a grid indicating avalanche runout along Dinf flow directions
int avalancherunoutgrd(char *angfile, char *felfile, char *assfile, char *rzfile, char *dmfile, float thresh, float alpha, int path);
//This function returns a grid of slope^m*sca^n
int slopearea(char *slopefile, char*scafile, char *safile, float *p);
//This function returns a grid of 1 and 0 indicating if areaD8 >= M*plen^y
int lengtharea(char *plenfile, char*ad8file, char *ssfile, float *p);
//This function returns an indicator (1,0) grid of grid cells that have values >= the input grid
int threshold(char *ssafile,char *srcfile,char *maskfile, float thresh, int usemask);
//This function creates a grid that has combined stability index (SINMAP)
int sindexcombined(char *slopefile,  char *scaterrainfile, char *scarminroadfile,char* scarmaxroadfile,
           char *tergridfile, char *terparfile, char *satfile,char* sincombinedfile,double Rminter,double Rmaxter, 
		   double *par);
