//#include "stdafx.h"

# include "shp_polyline.h"

shp_polyline::shp_polyline()
{	shapetype = API_SHP_POLYLINE;
}

shp_polyline::shp_polyline( const shp_polyline & p )
	:shape(p)
{	
}

int shp_polyline::recordbyteLength()
{	
	int numberbytes = shape::recordbyteLength();
	numberbytes += sizeof(int);
	numberbytes += sizeof(double)*4;
	numberbytes += sizeof(int);
	numberbytes += sizeof(int);
	if( parts.size() > 0 )
		numberbytes += sizeof(int)*parts.size();
	else
		numberbytes += sizeof(int);
	numberbytes += sizeof(double)*2*allPoints.size();

	return numberbytes;
}

bool shp_polyline::read( FILE * f, int & bytesRead )	
{	
	shape::read( f, bytesRead );

	int type;
	fread(&type,sizeof(int),1,f);

	bytesRead += sizeof(int);
	if(type != API_SHP_POLYLINE)
	{	std::cout << "Shape type does not match in record."<<std::endl;
		return false;		
	}
	
	//input Bounding Box
	//Assign Class Variables topLeft && bottomRight
	double mybounds[4];
	fread(mybounds,sizeof(double),4,f);
	bytesRead += sizeof(double)*4;
	topLeft = api_point( mybounds[0], mybounds[3] );
	bottomRight = api_point( mybounds[2], mybounds[1] );

	//input Number of Parts and Number of Points
	int numParts = 0;
	int numPoints = 0;
	fread(&numParts,sizeof(int),1,f);
	bytesRead += sizeof(int);
	fread(&numPoints,sizeof(int),1,f);
	bytesRead += sizeof(int);
	
	//Allocate space for numparts
	int * p_parts = new int[numParts];
	fread(p_parts,sizeof(int),numParts,f);
	bytesRead += sizeof(int)*numParts;
	//Input parts
	for( int j = 0; j < numParts; j++ )
		parts.push_back( p_parts[j] );	
	
	//Input points
	double x,y;
	for(int i=0;i<numPoints;i++)
	{
		fread(&x,sizeof(double),1,f);
		bytesRead += sizeof(double);
		fread(&y,sizeof(double),1,f);
		bytesRead += sizeof(double);
		api_point p(x,y);
		insertPoint( p, i );
	}

	delete p_parts;

	return true;
}

shp_polyline shp_polyline::operator=( const shp_polyline & sp )	
{	
	parts.clear();
	for( int i = 0; i < sp.parts.size(); i++ )
		parts.push_back( sp.parts[i] );

	return *this;
}

api_point shp_polyline::shapeMiddle( api_point topLeftBound, api_point bottomRightBound )
{	api_point p;

	if( size() > 0 )
	{	int index = (int)floor((double)size()/2);
		p = getPoint( index );
		int cnt = index;
		while( !pointInBounds( p, topLeftBound, bottomRightBound ) && cnt < size() )
			p = getPoint( ++cnt );
		
		cnt = index;
		while( !pointInBounds( p, topLeftBound, bottomRightBound) && cnt > 0 )
			p = getPoint( --cnt );		
		
		return p;
	}
	else
		return p;
}

void shp_polyline::writeShape( FILE * out, int recordNumber )
{	writeRecordHeader( out, recordNumber );
	
	fwrite(&shapetype,sizeof(int),1, out);
	
	double shapeBounds[4];
	shapeBounds[0] = topLeft.getX();
	shapeBounds[1] = bottomRight.getY();
	shapeBounds[2] = bottomRight.getX();
	shapeBounds[3] = topLeft.getY();
	fwrite(shapeBounds,sizeof(double),4, out);
	
	int numParts = parts.size();
	if( numParts <= 0 )
		numParts = 1;
	fwrite(&numParts,sizeof(int),1, out);
	int numPoints = allPoints.size();
	fwrite(&numPoints,sizeof(int),1, out);
	
	if( parts.size() > 0 )
	{	for( int i = 0; i < parts.size(); i++ )
		{	int recordPart = parts[i];
			fwrite(&recordPart,sizeof(int), 1, out);		
		}
	}
	else
	{	int recordPart = 0;
		fwrite( &recordPart, sizeof(int), 1, out );
	}

	//Input points
	double x,y;
	for( int j = 0; j < allPoints.size(); j++ )
	{	api_point p = allPoints[j];
		x = p.getX();
		y = p.getY();		
		fwrite(&x,sizeof(double),1, out);
		fwrite(&y,sizeof(double),1, out);
	}
}


bool shp_polyline::deletePoint( int position )
{	return shape::deletePoint( position );
}

int shp_polyline::insertPoint( api_point p, int position )
{	return shape::insertPoint( p, position );
}

bool shp_polyline::setPoint( api_point p, int position )
{	return shape::setPoint( p, position );
}

#ifdef SHAPE_OCX
		std::deque<CString> shp_polyline::getMembers()
		{	std::deque<CString> members;
			members.push_back( intToCString( shapetype ) );
			for( int i = 0; i < allPoints.size(); i++ )
			{	members.push_back( doubleToCString( allPoints[i].getX() ) );
				members.push_back( doubleToCString( allPoints[i].getY() ) );
			}
			return members;
		}

		bool shp_polyline::setMembers( std::deque<CString> members )
		{	try
			{	if( members.size() < 3 || ( members.size() - 1 )%2 != 0 )
					throw( Exception("Cannot create shp_polyline ... wrong number of parameters.") );
				if( members[0] != API_SHP_POLYLINE )
					throw( Exception("  Cannot create shp_polyline.  Shape type invalid." ) );
				
				for( int i = 1; i < members.size(); i += 2 )
				{	api_point p;
					p.setX( CStringToDouble( members[i] ) );
					p.setY( CStringToDouble( members[i+1] ) );
					insertPoint( p, 0 );
				}
				return true;
			}
			catch( Exception e )
			{	return false;
			}
		}
#endif

