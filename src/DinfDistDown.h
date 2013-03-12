// DinfDistDown.h header file
int nameadd(char *full,char *arg,char *suff);
int dinfdistdown(char *angfile,char *felfile,char *slpfile,char *wfile,char *srcfile,
				char *dtsfile,int statmethod,int typemethod,int usew, int concheck, int prow, int pcol);
int hdisttostreamgrd(char *angfile, char *wfile, char *srcfile, char *dtsfile, int statmethod, int usew, 
					 int concheck, int prow, int pcol);
int vdroptostreamgrd(char *angfile, char *felfile, char *srcfile, char *dtsfile, int statmethod, int concheck, int prow, int pcol);
int pdisttostreamgrd(char *angfile, char *felfile, char *wfile, char *srcfile, char *dtsfile, int statmethod, 
					 int usew, int concheck, int prow, int pcol);
int sdisttostreamgrd(char *angfile, char *slpfile, char *wfile,  char *srcfile, char *dtsfile, 
					 int statmethod, int usew, int concheck, int prow, int pcol);

#define MAXLN 4096

