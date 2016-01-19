//
//#include "shapefile.h"
//#include "gridio.h"

int sedaccumweight(char* dem,char* dpfile,char* swtfile,char* idfieldname,double* sedprod,int* streamcon,int isstrmcon);
int sedaccumstream(char* sacfile,char* ad8file,char* streamfile,char* fldsac,char* fldspecsed);
//int seddirstream(char* demfile,char* sacfile,char* ad8file,char* streamfile,char* fldstrmord,char* fldseddir,char* fldspecsed);
int seddirstream(char* streamfile, char* fldseddir, char* fldspecsed, char* fldlinkno, char* flduslink1no, char* flduslink2no, char* flddscontarea, char* fldsedaccum);
//int sindexdrainpoint(char* sifile,char* dpfile,char* sinfieldname);
int sindexdrainpoint(char* sifile,char* dpfile,char* idFieldName, SAFEARRAY **siValues);
int specificsed(char* sacfile,char* ad8file,char* specfile);
int sloped(char *pfile,char* felfile,char* slpdfile, double dn);
int accumLRbdxDrainPoints(char* dpfile, char* rdfile,char* DrainIDDp,char* RoadID,int* DrainID1RdArr,
	int* DrainID2RdArr,double* rdLength,char* demfile,char* lrbdxminfile,char* lrbdxmaxfile,double* R, double b);

int coord2gridrowcol(api_point * pnt,ghead head, int *x,int *y);
int preprocess(char* shplist, char* fnameslist);
int lsplotvalues(char* calfile, char* dpfile,char* didfieldname, double* slpvalues,double* lenvalues,
				 long* dtovalues, char* lstxtfile);
int lsplotvalues2(char* dpfile,char* didfieldname, double* slpvalues,double* lenvalues,
				 long* dtovalues, char* lstxtfile);
int createweightgridfrompoints(char* demfile,char* dpfile,char* wtfile,char* idfieldname,
				   double* weightvalue);
int pointsnapping(char *pfile, char *srcfile,char *ad8file, double *xnode,double *ynode, int nxy, int max_dist,long *ismoved,
				 long *dist_moved);
int locatehabitatsegmentation(char *pfile, char *srcfile,char *ad8file, char* netfile, char* dpfile,
							  char* habpatchtable,char* dpidfieldname, int* barrier,char* habsegfieldname);
int lineshapeelevrange(char *zfile, char *shpfile, char *newfield, char *length);

