/*  PeukerDouglas function that operates on an elevation grid and outputs an 
  indicator (1,0) grid of upward curved grid cells according to the Peuker and 
  Douglas algorithm.
     
  David Tarboton
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
#include <math.h>
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include "ctime"

using namespace std;

int peukerdouglas(char *felfile, char *ssfile,float *p)
{
	MPI_Init(NULL,NULL);
	{
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("PeukerDouglas version %s\n",TDVERSION);

	double begint = MPI_Wtime();

	long x,y;
	int k,ik,jk,jomax,iomax,bound;
	float ndve,emax;
	float* ndveptr;
								/* Read elevation headers */
	tiffIO felev(felfile, FLOAT_TYPE);			//input	 elevation	
	long totalX = felev.getTotalX();			//Globabl x and y
	long totalY = felev.getTotalY();
	double dxA = felev.getdxA();				//cell x and y
	double dyA = felev.getdyA();
	if(rank==0)
		{
			float timeestimate=(1e-7*totalX*totalY/pow((double) size,1))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}


	ndveptr =  (float*)felev.getNodata();
	ndve = *ndveptr;
								//Create partition and read data
	tdpartition *elev;
	elev = CreateNewPartition(felev.getDatatype(), totalX, totalY, dxA, dyA, felev.getNodata());
	int elevnx = elev->getnx();
	int elevny = elev->getny();
	int globalxstart, globalystart; 			
	elev->localToGlobal(0, 0, globalxstart, globalystart);  

	felev.read((long)globalxstart, (long)globalystart, (long)elevny, (long)elevnx, elev->getGridPointer());

	//Record time reading files
	double readt = MPI_Wtime();
								
//Create empty partition to store new information
	tdpartition *selev;
	selev = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, felev.getNodata());//selev is to smooth the elevation

//Create empty partition to store new information
	tdpartition *ss;
	short ssnodata =-2;
	ss = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, ssnodata);//-2 nodata value
	
// SHARE elev to populate the borders
	elev->share();
							
	float floatTemp1;
								//--Smooth the Grid--
	for(y=0; y<elevny; y++){
		for(x=0; x <elevnx; x++)
		{
								//  NEED function in linearpart to evaluate whether is on edge. 
								//  Suggestion **->isOnEdge(x,y) to return true if on edge of global domain or adjacent to a no data value, otherwise to return false.
								//  FIX code below to if on a global edge or adjacent to a no-data value do first "if block"

		  if(rank == 0 && y == 0 || rank == (size-1) && y == (elevny-1) )
		  {
			  selev->setData(x,y,elev->getData(x, y,floatTemp1 ));
			  ss->setData(x,y,(short)0);
		  }
		  else if(x == 0 || x == (elevnx-1) ||  elev->isNodata(x,y))
		  {
			  selev->setData(x,y,elev->getData(x, y,floatTemp1 ));
			  ss->setData(x,y,(short)0);
		  }
		  else
		  {  						//  THIS is for all the remainder of cells

			ss->setData((long)x, (long)y, (short)1);  								// Initializing to 1 for all non edge grid cells
							
			float elevwsum = p[0] * elev->getData(x, y, floatTemp1 );
			float wsum=p[0];
			if(p[1] > 0.)
			  for(k=1; k<=7; k=k+2)
			  {
				if(!elev->isNodata(x+d1[k],y+d2[k]))
				{			  
		  		  elevwsum +=  elev->getData(x+d1[k],y+d2[k],floatTemp1) * (p[1]);
				  wsum += p[1];
				}				  
			  }
			if(p[2] > 0.)
			  for(k=2; k<=8; k=k+2)
			  {
				if(!elev->isNodata(x+d1[k],y+d2[k]))
				{
		  		  elevwsum +=  elev->getData(x+d1[k],y+d2[k],floatTemp1) *p[2];
				  wsum += p[2];
				}				  
			  }
			elevwsum=elevwsum/wsum;
			selev->setData(x,y,elevwsum);
		   }
	 	 }
	}			
	
	//-- Put smoothed elevations back in elevation grid--
	  for(y=0; y < elevny; y++)
	  {
		 for(x=0; x < elevnx; x++)
		 {
			elev->setData(x,y,selev->getData(x,y,floatTemp1));
		 }
	  }
	elev->share();							
	
	//--Calculate Streams--
  	  for(y=-1; y < elevny; y++){
		  for(x=0; x < elevnx-1; x++)
		  {
			  emax =elev->getData(x,y,floatTemp1); 
			  iomax=0;
			  jomax=0;   
			  bound=0;  					/*  .false.  */
			
									/*  --FIRST PASS FLAG MAX ELEVATION IN GROUP OF FOUR  */
			  for(ik=0; ik<2; ik++)
				  for(jk=1-ik; jk < 2; jk++)
				  {
					  if(elev->isNodata(x+jk,y+ik))
						 bound= 1; 
					  else if(elev->getData(x+jk,y+ik,floatTemp1) > emax)
					  {
						  emax=floatTemp1;
						  iomax=ik;
						  jomax=jk;
					  }
				  }
				 			/*  c---Unflag max pixel */
			 ss->setData(x+jomax,y+iomax,(short)0);
				/*  c---Unflag pixels where the group of 4 touches a boundary  */
			  if(bound == 1)
			  {
				for(ik=0; ik < 2; ik++)
				  for(jk=0; jk< 2; jk++)
				  {
				 	ss->setData(x+jk,y+ik,(short)0);
				  }
			  }else{
										/*  i.e. unflag flats.  */
				for(ik=0; ik < 2; ik++)
				  for(jk=0; jk< 2; jk++)
				  {
					 if(elev->getData(x+jk,y+ik,floatTemp1) == emax)
					 {
						ss->setData(x+jk,y+ik,(short)0);
					 }
				  }
			  }
			
		  }
	}															
	//Stop timer
	double computet = MPI_Wtime();
	tiffIO outelev(ssfile,SHORT_TYPE,&ssnodata, felev);
	
	outelev.write((long)globalxstart, (long)globalystart, (long)elevny, (long)elevnx, ss->getGridPointer());
	double writet = MPI_Wtime();
	double dataRead, compute, write, total,temp;
        dataRead = readt-begint;
        compute = computet-readt;
        write = writet-computet;
        total = writet - begint;

        MPI_Allreduce (&dataRead, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = temp/size;
        MPI_Allreduce (&compute, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = temp/size;
        MPI_Allreduce (&write, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        write = temp/size;
        MPI_Allreduce (&total, &temp, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = temp/size;

        if( rank == 0)
                printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                  size , dataRead, compute, write,total);

	}
	MPI_Finalize();
	return 0;
}


