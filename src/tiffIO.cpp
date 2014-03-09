/*  Taudem common library functions 

  David Tarboton, Kim Schreuders, Dan Watson
  Utah State University  
  May 23, 2010
  
*/

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

#include <mpi.h>
#include <stdio.h>
//#include "stdint.h"  // See http://en.wikipedia.org/wiki/Stdint.h for details.
#include <memory>
#include "tiffIO.h"
#include "tifFile.h"
#include <iostream>

//  The block below so that mkdir from the correct header is used
#ifdef  _MSC_VER  //  Microsoft compiler
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <ctype.h>

using namespace std;

//initialize to read in.
tiffIO::tiffIO(char *dirname, DATA_TYPE newtype){
	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);
//	if(rank == 0)
//		cout << "Constructor Start" << endl;
	dir = NULL;
	drnt = NULL;
	numFiles = 0;
	long filecount = 0;
	double temp = 0;
	double MaxY;
	double MaxX;
	double MinX;
	double MinY;
	gTotalX = 0;
	gTotalY = 0;
	fileX = 0;
	fileY = 0;
	datatype = newtype;
	strncpy(dirn, dirname, NAME_MAX);
	//Check that we can open the directory.
	if((dir = opendir(dirname)) == NULL){
		cout << "Error: dir " << opendir << " " << dirname << " failed to open." << endl;
		MPI_Abort(MCW,3);//file open fail.
	}
	//Count the number of tiff files in the folder.
	long numChar;
	while(drnt = readdir(dir)){
		//if(strstr(drnt->d_name,".tif") != NULL){
			numChar = strlen(drnt->d_name);
			if(numChar > 4)  //  Can not be a tif file if name is too short
			 if(drnt->d_name[numChar-4] == '.' && tolower(drnt->d_name[numChar-3]) == 't'
                         && tolower(drnt->d_name[numChar-2]) == 'i' && tolower(drnt->d_name[numChar-1]) == 'f')
				numFiles++;
		//}
	}
	if( numFiles == 0 ){
		cout << "error no files to read" << endl;
		MPI_Abort(MCW, 100);
	}
	closedir(dir);

	//create an array of file names, one for each tif file.
	files = new char*[numFiles];
	for(long i = 0; i < numFiles;i++)
		files[i] = new char[NAME_MAX];

	//make sure the directory opened again. And open it.
	if((dir = opendir(dirname)) != NULL)
	{	//read the first entery in the directory
		while(drnt = readdir(dir))
		{	//if the name has '.tif' in it, and at the end, save the file name.
			//if(strstr(drnt->d_name,".tif") != NULL){
			numChar = strlen(drnt->d_name);
			if(numChar > 4){
			if(drnt->d_name[numChar-4] == '.' && tolower(drnt->d_name[numChar-3]) == 't'
				&& tolower(drnt->d_name[numChar-2]) == 'i' && tolower(drnt->d_name[numChar-1]) == 'f')
				if(filecount < numFiles){
					strncpy(files[filecount],drnt->d_name,NAME_MAX);
					filecount++;
				}
			}
		}
	}
	else
	{
		printf("Can not open directory '%s'\n", dirn);
		MPI_Abort(MCW, 101);
	}
	
	closedir(dir);
	IOfiles = new tifFile*[numFiles];//create a array of tifFile pointers. One for each file name.
	for(int i = 0; i < numFiles; i++)
	{	//Open each tif file as a tifFile.
		strncpy(file,dirname,NAME_MAX); // Put the directory path into the file name to be oppened
		strncat(file,"/",2);		//add trailing slash.
		IOfiles[i] = new tifFile(strncat(file,files[i],NAME_MAX),datatype);//Create a new tifFile and Read it in.
	}
	nodata = IOfiles[0]->getNodata();
	DATA_TYPE olddatatype = IOfiles[0]->getDatatype();
	dx = IOfiles[0]->getdx();
	dy = IOfiles[0]->getdy();

	//Calculate the biggest and smallest x and y values.
	for (int i = 0; i < numFiles; i++){
		double tol=0.0000001;
		if(rank==0)  // Only do checks on rank=0 as values are the same in all processors and to limit repeating output
		{
			// Check consistency of dx
			if(abs(dx-IOfiles[i]->getdx()) > tol*dx) 
			{
				printf("dx does not match for all input files: %lf %lf\n",dx,IOfiles[i]->getdx());  
				MPI_Abort(MCW,7); //dx outside of tolerance.
			}
			// Check consistency of dy
			if(abs(dy-IOfiles[i]->getdy()) > tol*dy) 
			{
				printf("dy does not match for all input files: %lf %lf\n",dy,IOfiles[i]->getdy());  
				MPI_Abort(MCW,7); //dy outside of tolerance.
			}	
			//  Check consistency of nodata
			if( olddatatype == SHORT_TYPE ) {
				if((*(short*)nodata - *(short*)IOfiles[i]->getNodata()) != 0)
					printf("Warning! No-Data values across the files do not match!\n%d:%d\n",(*(short*)nodata),(*(short*)IOfiles[i]->getNodata()));
			}
			else if( olddatatype == LONG_TYPE) {
				if((*(long*)nodata - *(long*)IOfiles[i]->getNodata()) != 0)
					printf("Warning! No-Data values across the files do not match!\n%ld:%ld\n",(*(long*)nodata),(*(long*)IOfiles[i]->getNodata()));
			}
			else if( olddatatype == FLOAT_TYPE) {
				if(abs(*(float*)nodata - *(float*)IOfiles[i]->getNodata()) > (float)tol)
					printf("Warning! No-Data values across the files do not match!\n%f:%f\n",(*(float*)nodata),(*(float*)IOfiles[i]->getNodata()));
			}
		}

		//Set geo bounds for file
		IOfiles[i]->setgeoystart( IOfiles[i]->getYTopEdge());
		IOfiles[i]->setgeoyend(IOfiles[i]->getYTopEdge() - IOfiles[i]->getTotalY() * dy);
		IOfiles[i]->setgeoxstart( IOfiles[i]->getXLeftEdge());
		IOfiles[i]->setgeoxend( IOfiles[i]->getXLeftEdge() + IOfiles[i]->getTotalX() * dx);

		//Find the max and min geo x and geo y.
		if(i==0)  // first file
		{
			MaxY = IOfiles[i]->getgeoystart();
			MaxX = IOfiles[i]->getgeoxend();
			MinX = IOfiles[i]->getgeoxstart();
			MinY = IOfiles[i]->getgeoyend();
		}
		else
		{
			if(MaxY < IOfiles[i]->getgeoystart())
				MaxY = IOfiles[i]->getgeoystart();
			if(MaxX < IOfiles[i]->getgeoxend())
				MaxX = IOfiles[i]->getgeoxend();
			if(MinX > IOfiles[i]->getgeoxstart())
				MinX = IOfiles[i]->getgeoxstart();
			if(MinY > IOfiles[i]->getgeoyend())
				MinY = IOfiles[i]->getgeoyend();
		}
	}
	//Calculate the global total x and global total y rounding double geo coordinates to long.
	gTotalY = (long)((MaxY - MinY)/dy+0.5);
	gTotalX = (long)((MaxX - MinX)/dx+0.5);
// Calculate global/domain values from geographic coordinates.
	for(int i = 0; i < numFiles; i++){
		IOfiles[i]->setgystart( (uint32_t)((MaxY - IOfiles[i]->getgeoystart()) / dy+0.5));
		IOfiles[i]->setgyend((uint32_t)((MaxY-IOfiles[i]->getgeoyend()) / dy-0.5));
		//  The end values are -0.5 to refer the last row/col of the file using start from 0 C indexing
		// +0.5 -1 = -0.5
		IOfiles[i]->setgxstart((uint32_t)((IOfiles[i]->getgeoxstart() - MinX) / dx+0.5));
		IOfiles[i]->setgxend((uint32_t)((IOfiles[i]->getgeoxend() - MinX) / dx-0.5));

		//  Debugging
		//if(rank==7)
		//{
		//	printf("%d: %s\n",i,files[i]);					
		//	if( olddatatype == SHORT_TYPE ) {
		//		printf("%d\n",*(short*)IOfiles[i]->getNodata());
		//	}
		//	else if( olddatatype == LONG_TYPE) {
		//		printf("%ld\n",*(long*)IOfiles[i]->getNodata());
		//	}
		//	else if( olddatatype == FLOAT_TYPE) {
		//		printf("%g\n",*(float*)IOfiles[i]->getNodata());
		//	}
		//	printf("%d %d %d %d\n",IOfiles[i]->getgystart(),IOfiles[i]->getgyend(),IOfiles[i]->getgxstart(),IOfiles[i]->getgxend());
		//}

	}
	
	// Assign synonym variables that are used by the object.
	ytop = MaxY;
	xleft = MinX; 
	xleftedge=MinX;
	ytopedge=MaxY;

	//calculate the xllcenter and yllcenter. 
        xllcenter=xleftedge+dx/2.;
        yllcenter=ytopedge-(gTotalY*dy)-dy/2.;

	long partStart = (gTotalY / size) * rank;
	long partEnd = (gTotalY / size) * (rank + 1);
	readwrite = 0;
	//if(rank == 0)
	//	cout << "Constructor End" << endl;
}

//Copy constructor.  Requires datatype in addition to the object to copy from.
//initialize to write out.
tiffIO::tiffIO(char *dirname, DATA_TYPE newtype, void* nd, const tiffIO &copy) {
	MPI_Comm_size(MCW, &size);
	MPI_Comm_rank(MCW, &rank);
//	if(rank == 0)
//                cout << "Copy Constructor Start" << endl;
	xleftedge = copy.xleftedge;
	ytopedge = copy.ytopedge;
	xllcenter = copy.xllcenter;
	yllcenter = copy.yllcenter;
	xleft = copy.xleft;
	ytop = copy.ytop;
	dx = copy.dx;
	dy = copy.dy;
	gTotalX = copy.gTotalX;
	gTotalY = copy.gTotalY;
	strncpy(dirn, dirname, NAME_MAX);
	numFiles = 1;
	dir = NULL;
	drnt = NULL;
	nodata = nd;
	datatype = newtype;
	int makeDir = 0;

	// Make sure each process has a directory to write to.  Do this in a loop to accommodate the case of local disks
	// that may serve a subset of the processes that will also need the directory created.
	for(int i = 0; i < size; i++){
		if(rank == i){
			if((dir = opendir(dirname))==NULL)
			{
				//  The block below so that correct platform dependent mkdir is used
				#ifdef  _MSC_VER  //  Microsoft compiler
					int a = mkdir(dirname);
				#else
					int a = mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO);
				#endif
				if(a != 0){
					printf("Failed to create directory '%s'\n", dirn); fflush(stdout);
					MPI_Abort(MCW,103);
				}
				//printf("Created directory '%s'\n", dirn);
				if((dir = opendir(dirname))==NULL)  // Check that we can open directory after creation
				{
					printf("Failed to open directory after creation '%s'\n", dirn); fflush(stdout);
					MPI_Abort(MCW,103);
				}
				closedir(dir);
			}
		}
		MPI_Barrier(MCW); // Dont proceed untill all processes get here (and directory has been created in proccess i, opened and closed)

	}
	readwrite = 1;
	IOfiles = new tifFile*[1];
	files = new char*[1];
	files[0] = new char[NAME_MAX];
	strncpy(files[0],copy.dirn,NAME_MAX);
	strncpy(files[0],"/",2);
	numFiles = 1;
	IOfiles[0] = new tifFile(strncpy(files[0],copy.files[0],NAME_MAX),datatype,nd,*copy.IOfiles[0], true);
	fileX = copy.fileX;
	fileY = copy.fileY;
	//closedir(dir);
	/*if(rank == 0)
                cout << "Copy Constructor End" << endl;*/
}

tiffIO::~tiffIO(){
	//delete all files...
	if(readwrite == 1){//if it was copy constructed delete what was allocated.
		delete IOfiles[0];
		delete IOfiles;
		delete files[0];
		delete files;
		return;
	}
	for(int i =0; i < numFiles; i++){
		delete files[i];
		delete IOfiles[i];
	}
	delete files;
	delete IOfiles;
}

void tiffIO::read(long xstart, long ystart, long numRows, long numCols, void* dest){//, long partOffset, long DomainX) {
	//read in multiple files
	long partStart = (gTotalY / size) * rank;
	long partEnd = (gTotalY / size) * (rank + 1);
	if(rank == size -1)
		partEnd += (gTotalY - size*((long)(gTotalY / size)));
	//  partEnd as used here is actually the index of the first row in the next partition
	for(int i =0 ; i < numFiles; i++){//TODO adjust for not reading in a full part. assumes read in all x values for a part and all y values for a part.
		uint32_t filegystart=IOfiles[i]->getgystart();
		uint32_t filegyend=IOfiles[i]->getgyend();
		uint32_t filegxstart=IOfiles[i]->getgxstart();
		uint32_t numy,numx;
		numy=IOfiles[i]->getTotalY();
		numx=IOfiles[i]->getTotalX();

		if(filegystart >= partStart && filegyend < partEnd){//file is only in this partition.
			IOfiles[i]->read(	
				0,//xstart,//xstart
				0,//ystart,//ystart
				numy,  // numRows to read
				numx,  // numCols to read
				dest,  // Destination of partition in memory, i.e. top left cell of partition
				gTotalX * (filegystart - partStart) + filegxstart,// Offset from beginning of partition to beginning of this file
				gTotalX);//x jump factor
		}else if(filegystart >= partStart && filegystart < partEnd && filegyend >= partEnd){//top of file in this partition
			IOfiles[i]->read(       
				0,
				0,
				partEnd - filegystart,
				numx,
				dest,
				gTotalX * (filegystart - partStart) + filegxstart,
				gTotalX);
		}else if(filegystart < partStart && filegyend < partEnd && filegyend >= partStart){//bottom of file in this partition
			IOfiles[i]->read(       
				0,
				partStart-filegystart, 
				numy-(partStart-filegystart),
				numx,
				dest,
				filegxstart,
				gTotalX);
		}else if(filegystart < partStart && filegyend >= partEnd){//This file spans the whole partition.
			IOfiles[i]->read(       
				0,
				partStart - filegystart,
				partEnd - partStart,
				numx,
				dest,
				filegxstart,
				gTotalX);
		}else{//file doesn't span any of the partition
			//do nothing
		}
		if(numy * numx > fileX * fileY){// calculate bigest file and use it as output file size.
			fileY = numy;
			fileX = numx;
		}
		//get global xleft and ytop
		if(xleft > IOfiles[i]->getXLeftEdge()){//min x
			xleft = IOfiles[i]->getXLeftEdge();
			xleftedge = xleft;
		}
		if(ytop < IOfiles[i]->getYTopEdge()){  //max y
			ytop = IOfiles[i]->getYTopEdge();
			ytopedge = ytop;
		}
	}
	//calculate xllcenter and yllcenter.
	xllcenter=xleftedge+dx/2.;
	yllcenter=ytopedge-(gTotalY*dy)-dy/2.;
	//if(rank == 0)
 //               cout << "IO Read  End" << endl;

}

//Create/re-write tiff output file
void tiffIO::write(long xstart, long ystart, long numRows, long numCols, void* source,char prefix[],int prow,int pcol) {
//write output files.
	xsize = 0;
	ysize = 0;
	tifFile *outFile;
	bool byPartition = false;
	if(prow>0 && pcol>0)
		byPartition = true;
	long tempXleft = 0, tempYtop = 0;
	if(byPartition == true){

		
	//filex and filey are size of partiton.
	fileX = gTotalX;
	fileY = gTotalY / size;
        if(rank == size -1)
        	fileY += (gTotalY - size*((long)(gTotalY / size)));
	//make filex and file y size of file wanted
	int numXFiles = pcol;
	int numYFiles = prow;
	fileX = fileX/numXFiles;
	fileY = fileY/numYFiles;

	PYS = rank * (gTotalY / size) + 0;
	PYE = PYS + gTotalY / size;
	if(rank == size -1)
		PYE += (gTotalY - size*((long)(gTotalY / size)));
	long numY = PYE-PYS;
	for(int i = 0; i < gTotalX / fileX ; i++){
		for(int j = 0; j < numY / fileY ; j++){
			//calculate the number of x and y values to write to the file.
			if(fileX*(i+1) - gTotalX < 0)// while xfile
				xsize = fileX;
			else if(fileX*(i+1) == gTotalX)//exact size, no more x files
				xsize = fileX ;
			else if(fileX*(i+1) - gTotalX  > 0)//remainder file size
				xsize = fileX - (fileX*(i+1) - gTotalX); 
			if(i == numXFiles-1)
				xsize = fileX + gTotalX%fileX;//%numXFiles;

			if(fileY*(j+1) - numY < 0)// while yfile
				ysize = fileY;
			else if(fileY*(j+1) == numY)//exact size, no more y files
				ysize = fileY;
			else if(fileY*(j+1) - numY > 0)//remainder file size
				ysize = fileY - (fileY*(j+1) - numY); 
			if(j == numYFiles-1)
				ysize = fileY + numY%fileY;//;numYFiles;

			FYS = PYS+fileY*j;
			FYE = FYS + ysize;

			//if file has information to be written write it.
			char outName[NAME_MAX];
			strncpy(outName,dirn,NAME_MAX);
			char buff[40];//biggest at zz
			strncat(outName,"/",2);//add trailing slash.
			strcat(outName,prefix);
			sprintf(buff,"p%d",rank);//TODO - out put in format (char)(char)(num).tif
			strcat(outName,buff);
			sprintf(buff,"r%d",j);//TODO - out put in format (char)(char)(num).tif
			strcat(outName,buff);
			sprintf(buff,"c%d",i);
			strcat(outName,buff);
			strcat(outName, ".tif");

			outFile = new tifFile(outName, datatype, nodata, *IOfiles[0]);
			outFile->setTotalXYtopYleftX(xsize, ysize, xleft + i * fileX * dx, ytop - PYS * dy - j * fileY * dy);
			if(PYS >= FYS && PYE < FYE)
			{//part is contained in the y coords being writtin to file.
				outFile->write(0, PYS - FYS, PYE - PYS, xsize, source,i * fileX, gTotalX, byPartition);//ysize - (PYS - FYS) - (FYE - PYE)
			}else if(PYS >= FYS && PYS < FYE && PYE >= FYE)
			{//top part of part in file
				outFile->write(0,PYS - FYS, FYE - PYS, xsize, source, i * fileX, gTotalX, byPartition); //ysize - (PYS - FYS)
			}else if(PYS < FYS && FYS < PYE && PYE < FYE)
			{//bottum of part in file
				outFile->write(0, 0, PYE - FYS, xsize, source, (FYS - PYS) * gTotalX + i * fileX, gTotalX, byPartition);//ysize - (FYS - PYS)
			}else if(PYS < FYS && FYE <= PYE)
			{//the files y coords are contained in this part.
				outFile->write(0, 0, FYE - FYS, xsize, source, (FYS - PYS) * gTotalX + i * fileX, gTotalX, byPartition);//ysize
			}else{//this part has no data for this file
				;//do nothing
			}
			delete outFile;

		}
	}
	}else{
	for(int i = 0; i < gTotalX / fileX + 1; i++){
		for(int j = 0; j < gTotalY / fileY + 1; j++){
			//calculate the number of x and y values to write to the file.
			if(fileX*(i+1) - gTotalX < 0)// while xfile
				xsize = fileX;
			else if(fileX*(i+1) == gTotalX)//exact size, no more x files
				xsize = fileX ;
			else if(fileX*(i+1) - gTotalX  > 0)//remainder file size
				xsize = fileX - (fileX*(i+1) - gTotalX); 

			if(fileY*(j+1) - gTotalY < 0)// while yfile
				ysize = fileY;
			else if(fileY*(j+1) == gTotalY)//exact size, no more y files
				ysize = fileY;
			else if(fileY*(j+1) - gTotalY > 0)//remainder file size
				ysize = fileY - (fileY*(j+1) - gTotalY); 
			
			PYS = rank * (gTotalY / size) + 0;
			PYE = PYS + gTotalY / size;
			if(rank == size -1)
				PYE += (gTotalY - size*((long)(gTotalY / size)));
			FYS = fileY*j;
			FYE = FYS + ysize;
			if(ysize ==0 || xsize == 0)
				continue;// exact break condition.  TODO- use Dr. Tarbotons arithmatic in for loops instead.

			//if file has information to be written write it.
			char outName[NAME_MAX];
			strncpy(outName,dirn,NAME_MAX);
			char buff[40];//biggest at zz
			strncat(outName,"/",2);//add trailing slash.
			strcat(outName,prefix);
			sprintf(buff,"r%d",j);//TODO - out put in format (char)(char)(num).tif
			strcat(outName,buff);
			sprintf(buff,"c%d",i);
			strcat(outName,buff);
			strcat(outName, ".tif");

			outFile = new tifFile(outName, datatype, nodata, *IOfiles[0]);
			outFile->setTotalXYtopYleftX(xsize, ysize, xleft + i * fileX * dx, ytop - j * fileY * dy);
			if(PYS >= FYS && PYE < FYE)
			{//part is contained in the y coords being writtin to file.
				outFile->write(0, PYS - FYS, PYE - PYS, xsize, source,i * fileX, gTotalX, byPartition);//ysize - (PYS - FYS) - (FYE - PYE)
			}else if(PYS >= FYS && PYS < FYE && PYE >= FYE)
			{//top part of part in file
				outFile->write(0,PYS - FYS, FYE - PYS, xsize, source, i * fileX, gTotalX, byPartition); //ysize - (PYS - FYS)
			}else if(PYS < FYS && FYS < PYE && PYE < FYE)
			{//bottum of part in file
				outFile->write(0, 0, PYE - FYS, xsize, source, (FYS - PYS) * gTotalX + i * fileX, gTotalX, byPartition);//ysize - (FYS - PYS)
			}else if(PYS < FYS && FYE <= PYE)
			{//the files y coords are contained in this part.
				outFile->write(0, 0, FYE - FYS, xsize, source, (FYS - PYS) * gTotalX + i * fileX, gTotalX, byPartition);//ysize
			}else{//this part has no data for this file
				outFile->write(0, 0, 0, 0, source, 0, 0,byPartition);//so process 0 will write out the header.
			}
			delete outFile;

		}
	}
	}
}

bool tiffIO::compareTiff(const tiffIO &comp){
	double tol=0.0001;
	if(gTotalX != comp.gTotalX)
	{
		printf("Columns do not match: %d %d\n",gTotalX,comp.gTotalX);  
		return false;
	}
	if(gTotalY != comp.gTotalY)
	{
		printf("Rows do not match: %d %d\n",gTotalY,comp.gTotalY);  
		return false;
	}
	if(abs(dx-comp.dx) > tol) 
	{
		printf("dx does not match: %lf %lf\n",dx,comp.dx);  
		return false;
	}
	if(abs(dy-comp.dy) > tol) 
	{
		printf("dy does not match: %lf %lf\n",dy,comp.dy);  
		return false;
	}	
	if(abs(xleftedge - comp.xleftedge) > 0.0) 	
	{
		if(rank==0)
		{
			printf("Warning! Left edge does not match exactly:\n");
			printf(" %lf in file %s\n",xleftedge,filename);  
			printf(" %lf in file %s\n",comp.xleftedge,comp.filename);
		} 
		//return false;  //DGT decided to relax this to a warning as some TIFF files seem to use center or corner to reference edges in a way not understood 
	}
	if(abs(ytopedge - comp.ytopedge) > 0.0) 	
	{
		if(rank==0)
		{
			printf("Warning! Top edge does not match exactly:\n");
			printf(" %lf in file %s\n",ytopedge,filename);  
			printf(" %lf in file %s\n",comp.ytopedge,comp.filename);
		} 
		//return false;  //DGT decided to relax this to a warning as some TIFF files seem to use center or corner to reference edges in a way not understood 
	}
	// 6/25/10.  DGT decided to not check spatial reference information as tags may be
	//  in different orders and may include comments that result in warnings when differences
	//  are immaterial.  Rather we rely on leftedge and topedge comparisons to catch projection
	//  differences as if the data is in a different projection and really different it would be
	//  highly coincidental for the leftedge and topedge to coincide to within tol
	return true;
}
void tiffIO::geoToGlobalXY(double geoX, double geoY, int &globalX, int &globalY){
	globalX = (int)((geoX - xleftedge) / dx);
	globalY = (int)((ytopedge - geoY) / dy);
}
void tiffIO::globalXYToGeo(long globalX, long globalY, double &geoX, double &geoY){
	geoX = xleftedge + dx/2. + globalX*dx;
	geoY = ytopedge - dy/2. - globalY*dy;  
}
