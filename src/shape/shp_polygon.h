# ifndef SHP_POLYGON
# define SHP_POLYGON

# include <iostream>
# include <fstream>
# include <deque>
# include "shape.h"
# include "types.h"

class shp_polygon : public shape
{
	public:
		shp_polygon();
		shp_polygon( const shp_polygon & p );
		virtual int recordbyteLength();
		virtual bool read( FILE * f, int & bytesRead );
		virtual shp_polygon operator=( const shp_polygon & sp );
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
