//#include "stdafx.h"
# include "shp_point.h"

shp_point::shp_point()
{	shapetype = API_SHP_POINT;
}

shp_point::shp_point( const shp_point & p )
	:shape(p)
{	
}

int shp_point::insertPoint( api_point p, int position )
{	allPoints.clear();
	allPoints.push_back( p );
	bounds();
	return 0;
}

bool shp_point::deletePoint( int position )
{	allPoints.clear();	
	return true;
}

bool shp_point::setPoint( api_point p, int position )
{	insertPoint( p, position );
	return true;
}

int shp_point::recordbyteLength()
{	
	int numberbytes = shape::recordbyteLength();
	numberbytes += sizeof(int) + sizeof(double)*2;
	return numberbytes;
}

bool shp_point::read( FILE * f, int & bytesRead )	
{	
	shape::read( f, bytesRead );

	int type;
	fread(&type,sizeof(int),1,f);
	
	bytesRead += sizeof(int);
	if(type != API_SHP_POINT)
	{	std::cout << "Shape type does not match type in record."<<std::endl;
		return false;
	}
	double X, Y;
	fread(&X,sizeof(double),1,f);
	fread(&Y,sizeof(double),1,f);

	bytesRead += sizeof(double)*2;
	
	api_point p( X, Y );
	insertPoint( p, 0 );

	topLeft = p;
	bottomRight = p;
	
	return true;
}

shp_point shp_point::operator=( const shp_point & sp )	
{	return *this;
}


api_point shp_point::shapeMiddle( api_point topLeftBound, api_point bottomRightBound )
{	if( size() > 0 )
		return getPoint( 0 );
	else
	{	api_point p;
		return p;
	}
}	

void shp_point::writeShape( FILE * out, int recordNumber )
{	writeRecordHeader( out, recordNumber );
	//Get the only point
	api_point p = getPoint( 0 );
	double X = p.getX();
	double Y = p.getY();
	fwrite(&shapetype,sizeof(int),1, out);
	fwrite(&X,sizeof(double),1, out);
	fwrite(&Y,sizeof(double),1, out);
}

#ifdef SHAPE_OCX
		std::deque<CString> shp_point::getMembers()
		{	std::deque<CString> members;
			members.push_back( intToCString( shapetype ) );
			members.push_back( doubleToCString( allPoints[0].getX() ) );
			members.push_back( doubleToCString( allPoints[0].getY() ) );
			return members;
		}

		bool shp_point::setMembers( std::deque<CString> members )
		{	try
			{	if( members.size() != 3 )
					throw( Exception("Cannot create shp_point ... wrong number of parameters.") );
				if( members[0] != API_SHP_POINT )
					throw( Exception("  Cannot create shp_point.  Shape type invalid." ) );

				api_point p;
				p.setX( CStringToDouble( members[1] ) );
				p.setY( CStringToDouble( members[2] ) );
				insertPoint( p, 0 );
				return true;
			}
			catch( Exception e )
			{	return false;
			}
		}
#endif
