//#include "shapefile.h"
//#include "gridio.h"

double siplot(double s,double a,double c1,double c2, 
                  double t1,double t2,double x1,double x2,double r);
int landslide(char* slpfile,char* scafile,char* calfile,char* lsfile,char* lstxtfile);
/*int sindex(char *slopefile,  char *lrbdxminfile, char *lrbdxmaxfile,   char *sinroadfile,
           char *tergridfile,char *terparfile, char *satfile,double *par);*/
int sindex(char *slopefile,  char *lrbdxminfile, char *lrbdxmaxfile,
           char *tergridfile,char *terparfile, char *satfile,char* sinroadfile,double *par,char* sirfieldname);
//int sindex(char *slopefile,  char *areafile,   char *sindexfile,char *tergridfile,char *terparfile, char *satfile,double *defpar);
int creategrid(char* demfile,char* opgridfile,int gridtype,int cellvalue);
long sindexstat(char *SIFile,char *CalFile,char* CalpFile,char *LSFile, char *StatFile,long nr);
int sir(char* demfile,char* dpfile,char* calpfile,char* elnfldname,char* slpfldname,char* resultfile,double b);\
int sirDP(char* sirfile,char* dpfile,char* fname);
int getRegionIndex(char* calpfile,long nor);
int findRegIndex(int regno,long nor,long ndvter);
long sistat(char *dpfile,char *calfile,char* calpfile, char *sistatfile,char* didfieldname, 
			double* sivalue,long* dtovalue,long nr);
long esistat(char *dpfile,char *calfile,char* calpfile, char *esistatfile,char* didfieldname,
			 double *esivalue, long *dtovalue,double *esidef, long nr);
long esistat2(char *dpfile,char *esistatfile,char* didfieldname, 
			  double *esivalue, long *dtovalue,double *esidef);
int sindexcombined(char *slopefile,  char *scaterrainfile, char *scarminroadfile,char* scarmaxroadfile,
           char *tergridfile,char *terparfile, char *satfile,char* sincombinedfile,double Rminter,double Rmaxter, 
		   double Rminrd, double Rmaxrd, double *par);


