# ifndef SHAPE_H
# define SHAPE_H

# include <iostream>
# include <fstream>
# include <deque>
# include <math.h>
# include "point.h"
# include "types.h"
# include "cell.h"
# include "exception.h"
# include <stdio.h>

class shape
{	
	public:
		//CONSTRUCTORS
		shape();
		shape( const shape & s );

		//OPERATORS
		virtual shape operator=( shape & s );
		virtual api_point operator[]( int api_point_index );

		//DATA MEMBER ACCESS
		virtual int getShapetype();				
		virtual int size();
		virtual api_point getPoint( int api_point_index );
		virtual api_point topLeftBound();
		virtual api_point bottomRightBound();

		//FUNCTIONS
		virtual int insertPoint( api_point p, int position );
		virtual bool deletePoint( int position );
		virtual bool setPoint( api_point p, int position );
		virtual api_point shapeMiddle( api_point topLeftBound, api_point bottomRightBound );
		virtual bool pointInBounds( api_point p, api_point topLeftBound, api_point bottomRightBound );
		void bounds();	
		
		//DATABASE FUNCTIONS
		int numberRecords();
		bool setCell( cell c, int position );
		cell getCell( int position );

		void createCell( cell c, int position );
		void deleteCell( int position );
		
		virtual int recordbyteLength();
		virtual bool read( FILE * f, int & bytesRead );
		virtual void writeShape( FILE * fp, int recordNumber );
		virtual void writeRecordHeader( FILE * out, int recordNumber );
		void SwapEndian(char* a,int size);	

	protected:
		int shapetype;
		api_point topLeft;
		api_point bottomRight;
		std::deque<api_point> allPoints;
		std::deque<cell> shapeRecord;

	public:
		#ifdef SHAPE_OCX
			virtual std::deque<CString> getMembers();
			virtual bool setMembers( std::deque<CString> members );
		#endif
};

# endif

