/*  Code for testing the tiffIO function.
     
  David Tarboton
  Utah State University  
  May 23, 2010 

*/

//  This is not a functional part of TauDEM.  Rather is is a testing program used to test code 
//  fragments being developed, often for testing the tiffio libraries

#include "tiffIO.h"
#include "commonLib.h"
#include "stdint.h"
#include "mpi.h"

#include "linearpart.h"
#include "createpart.h"
#include "dirent.h"

#include <iostream>

# define NAME_MAX 100

int main(){
	MPI_Init(NULL, NULL);
	{
		DIR* dir;
		struct dirent *drnt;
		int numFiles=0,filecount=0;
		char **files;

		char dirname[50];
		sprintf(dirname,"C:\\users\\dtarb\\scratch\\logan");
		dir=opendir(dirname);
        //Check that we can open the directory.
        if(dir == NULL){
                cout << "Error: dir " << opendir << " failed to open." << endl;
                MPI_Abort(MCW,3);//file open fail.
        }
        //Count the number of tiff files in the folder.
        while(drnt = readdir(dir)){
                if(strstr(drnt->d_name,".tif") != NULL)
                        numFiles++;
        }
        //cout << "cout = " << numFiles << endl;
        closedir(dir);
        //create an array of file names, one for each tif file.
        files = new char*[numFiles];
        for(long i = 0; i < numFiles;i++)
                files[i] = new char[NAME_MAX];
        dir = opendir(dirname);
        //make sure the directory opened again.
        if(dir)
        {       //read the first entery in the directory
                while(drnt = readdir(dir))
                {       //if the name has '.tif' in it save the file name.
                        if(strstr(drnt->d_name,".tif") != NULL){
                                cout << drnt->d_name << endl;
                                if(filecount < numFiles){
                                        strncpy(files[filecount],drnt->d_name,NAME_MAX);
                                        filecount++;
                                }
                        }
                }
        }
        else
        {
                printf("Can not open directory '%s'\n", dirname);
        }


		printf("Char Size %d\n",sizeof(char));
		printf("Short Size %d\n",sizeof(short));
		printf("Int Size %d\n",sizeof(int));
		printf("Long Size %d\n",sizeof(long));
		printf("Long Long Size %d\n",sizeof(long long));
		printf("Float Size %d\n",sizeof(float));
		printf("Double Size %d\n",sizeof(double));
		printf("Long Double Size %d\n",sizeof(long double));
		printf("int32_t size %d\n",sizeof(int32_t));
		printf("int64_t size %d\n",sizeof(int64_t));

		//  Program used to test Tiffio.  Provide any format in.tif file.
		//  Compile with type in the line below either SHORT_TYPE, LONG_TYPE or FLOAT_TYPE
		//  Then the output in out.tif should be the input data changed to the designated type

//  Tiffio testing block of code


		tiffIO infile("DemTestfel.tif",FLOAT_TYPE);
		long cols = infile.getTotalX();
		long rows = infile.getTotalY();

		DATA_TYPE type = infile.getDatatype();
		//tiffIO fel("fs_small.tif",FLOAT_TYPE);
		//if(!fel.compareTiff(infile))
		//printf("Mismatch\n");

		void *inarray;
		void* nd = infile.getNodata();	
		if(type == SHORT_TYPE){
			inarray = new short[rows*cols];
			short ndv= *(short *)nd;
			printf("No data: %d\n",ndv);
		}
		else if(type == LONG_TYPE){
			inarray = new long[rows*cols];
			long ndv= *(long *)nd;
			printf("No data: %ld\n",ndv);
		}
		else if(type == FLOAT_TYPE){
			inarray = new float[rows*cols];
		}

		infile.read(0, 0, rows, cols, inarray);


	//	float nd = 1.0f;
		tiffIO outfile("out.tif", type, nd, infile);
		outfile.write(0, 0, rows, cols, inarray);
   	}
	MPI_Finalize();
	return 0;
}
