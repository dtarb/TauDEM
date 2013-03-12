# ifndef SHP_POINT_H
# define SHP_POINT_H

# include <iostream>
# include <fstream>
# include "shape.h"
# include "types.h"
# include <stdio.h>

class shp_point : public shape
{
	public:
		shp_point();
		shp_point( const shp_point & p );
		virtual int insertPoint( api_point p, int position );
		virtual bool deletePoint( int position );
		virtual bool setPoint( api_point p, int position );
		virtual int recordbyteLength();
		virtual bool read( FILE * f, int & bytesRead );
		virtual shp_point operator=( const shp_point & sp );
		virtual api_point shapeMiddle( api_point topLeftBound, api_point bottomRightBound );
		virtual void writeShape( FILE * out, int recordNumber );

	protected:
	
	public:
		#ifdef SHAPE_OCX
			virtual std::deque<CString> getMembers();
			virtual bool setMembers( std::deque<CString> members );
		#endif
};

# endif
