#include "ogr_api.h"
#include "commonLib.h"
using namespace std;

int main(int argc,char **argv)
{
   char ds[MAXLN],srcfile[MAXLN],outletmovedfile[MAXLN],outletshpfile[MAXLN];
   int err,i,maxdist=50;
   
   if(argc < 9)
    {  
       printf("No simple use case for this function.\n");
	   goto errexit;
    }
		i = 1;
//		printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	while(argc > i)
	{
		if(strcmp(argv[i],"-p")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(pfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-src")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(srcfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-o")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletshpfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-om")==0)
		{
			i++;
			if(argc > i)
			{
				strcpy(outletmovedfile,argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if(strcmp(argv[i],"-md")==0)
		{
			i++;
			if(argc > i)
			{
				sscanf(argv[i],"%d",&maxdist);
				i++;
			}
			else goto errexit;
		}
		else 
		{
			goto errexit;
		}
	}
    if(err=outletstosrc(pfile,srcfile,outletshpfile,outletmovedfile,maxdist) != 0)
       printf("Move outlets to stream error %d\n",err);

	return 0;
errexit:
	   printf("Incorrect input.\n");
	   printf("Use with specific file names:\n %s -p <pfile>\n",argv[0]);
       printf("-src <srcfile> -o <outletsshapefile> -om <outletsmoved>\n");
       printf("[-md <max dist>]\n");
	   printf("<pfile> is the name of the D8 flow direction file (input).\n");
	   printf("<srcfile> is the name of the stream raster file (input).\n");
	   printf("<outletsshapefile> is the name of the outlet shape file (input).\n");
	   printf("<outletsmoved> is the name of the moved outlet shape file (output).\n");
	   printf("<max dist> is the maximum number of grid cells to move an outlet (input).\n");
	   printf("Default <max dist> is 50 if not input.\n");
       exit(0);
} 