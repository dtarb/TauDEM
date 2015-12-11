// DinfDistUp.h header file

int nameadd(char *full,char *arg,char *suff);
int dinfdistup(char *angfile,char *felfile,char *slpfile,char *wfile,
				char *dtsfile,int statmethod,int typemethod,
				int usew, int concheck, float thresh);
int hdisttoridgegrd(char *angfile, char *wfile, char *rtrfile, int statmethod, 
				int concheck, float thresh,int useweight);
int vrisetoridgegrd(char *angfile, char *felfile, char *rtrfile, int statmethod, 
					int concheck, float thresh);
int pdisttoridgegrd(char *angfile, char *felfile, char *wfile, char *rtrfile, 
					int statmethod, int useweight, int concheck, float thresh);
int sdisttoridgegrd(char *angfile, char *slpfile, char *wfile, char *rtrfile, 
					int statmethod, int useweight, int concheck, float thresh);
#define MAXLN 4096