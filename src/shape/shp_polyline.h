# ifndef SHP_POLYLINE
# define SHP_POLYLINE

# include <iostream>
# include <fstream>
# include <deque>
# include "shape.h"
# include "types.h"

class shp_polyline : public shape
{
	public:
		shp_polyline();
		shp_polyline( const shp_polyline & p );
		virtual int recordbyteLength();
		virtual bool read( FILE * f, int & bytesRead );
		virtual shp_polyline operator=( const shp_polyline & sp );
		virtual api_point shapeMiddle( api_point topLeftBound, api_point bottomRightBound );
		virtual void writeShape( FILE * out, int recordNumber );
		virtual bool deletePoint( int position );
		virtual int insertPoint( api_point p, int position );
		virtual bool setPoint( api_point p, int position );

	protected:
		std::deque<int> parts;

	public:
		#ifdef SHAPE_OCX
			virtual std::deque<CString> getMembers();
			virtual bool setMembers( std::deque<CString> members );
		#endif
};

# endif
