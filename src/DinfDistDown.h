// DinfDistDown.h header file
int nameadd(char *full,char *arg,char *suff);
int dinfdistdown(char *angfile,char *felfile,char *slpfile,char *wfile,char *srcfile,
				char *dtsfile,int statmethod,int typemethod,int usew, int concheck);
int hdisttostreamgrd(char *angfile, char *wfile, char *srcfile, char *dtsfile, int statmethod, int usew, 
					 int concheck);
int vdroptostreamgrd(char *angfile, char *felfile, char *srcfile, char *dtsfile, int statmethod, int concheck);
int pdisttostreamgrd(char *angfile, char *felfile, char *wfile, char *srcfile, char *dtsfile, int statmethod, 
					 int usew, int concheck);
int sdisttostreamgrd(char *angfile, char *slpfile, char *wfile,  char *srcfile, char *dtsfile, 
					 int statmethod, int usew, int concheck);

#define MAXLN 4096