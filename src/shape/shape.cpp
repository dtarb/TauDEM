//#include "stdafx.h"
# include "shape.h"
# include "types.h"

shape::shape()
{	shapetype = API_SHP_NULLSHAPE;
	topLeft = api_point( 0.0, 0.0 );
	bottomRight = api_point( 0.0, 0.0 );
}

shape::shape( const shape & s )
{	shapetype = s.shapetype;
	topLeft = s.topLeft;
	bottomRight = s.bottomRight;

	allPoints.clear();

	for( int i = 0; i < s.allPoints.size(); i++ )
		allPoints.push_back( s.allPoints[i] );
	for( int j = 0; j < s.shapeRecord.size(); j++ )
		shapeRecord.push_back( s.shapeRecord[j] );
}

int shape::recordbyteLength()
{	return REC_HEADER_BYTES;
}

int shape::getShapetype()
{	return shapetype;
}

bool shape::read( FILE * f, int & bytesRead )
{	
	char intbuf[sizeof(int)];	
	//get record number -- BIG ENDIAN	
	fread(intbuf,sizeof(char),sizeof(int),f);
	SwapEndian(intbuf,sizeof(int));
	//recNum = *(int*)intbuf;

	//get content length -- BIG ENDIAN
	fread(intbuf,sizeof(char),sizeof(int),f);
	SwapEndian(intbuf,sizeof(int));
	int recordLen = *(int*)intbuf;

	bytesRead = sizeof(int);
	bytesRead += sizeof(int);

	return true;
}

shape shape::operator=( shape & s )
{	shapetype = s.shapetype;
	topLeft = s.topLeft;
	bottomRight = s.bottomRight;

	allPoints.clear();

	for( int i = 0; i < s.size(); i++ )
		allPoints.push_back( s[i] );
	for( int j = 0; j < s.shapeRecord.size(); j++ )
		shapeRecord.push_back( s.shapeRecord[j] );
	
	return *this;
}

api_point shape::operator[]( int api_point_index )
{	if( api_point_index < size() )
		return allPoints[api_point_index];
	else
	{	api_point p;
		return p;
	}
}

api_point shape::getPoint( int api_point_index )
{	if( api_point_index < size() )
		return allPoints[api_point_index];
	else
	{	api_point p;
		return p;
	}
}

int shape::size()
{	return allPoints.size();
}

int shape::insertPoint( api_point p, int position )
{	if( position <= 0 )
	{	allPoints.push_front( p );
		position = 0;
	}
	else if( position >= allPoints.size() )
	{	allPoints.push_back ( p );
		position = allPoints.size();
	}
	else
	{	allPoints.insert( allPoints.begin() + position, p );	
		position = position;
	}
	bounds();
	return position;
}

bool shape::deletePoint( int position )
{	try
	{	if( position >= 0 && position < allPoints.size() )
		{	allPoints.erase( allPoints.begin() + position );	
			bounds();
		}
		else
			throw( Exception("Could not delete point.  Index out of bounds.") );
	}
	catch( Exception e )
	{	return false;
	}
	return true;
}

bool shape::setPoint( api_point p, int position )
{	if( position >= 0 && position < allPoints.size() )
	{	allPoints[position] = p;
		return true;
	}
	else
		return false;
}

void shape::SwapEndian(char* a,int size) 
{
	char hold;
	for(int i=0; i<size/2; i++) {
		hold = a[i];
		a[i]=a[size-i-1];
		a[size-i-1] = hold;
	}
}

api_point shape::topLeftBound()
{	return topLeft;
}

api_point shape::bottomRightBound()
{	return bottomRight;
}

api_point shape::shapeMiddle( api_point topLeftBound, api_point bottomRightBound )
{	api_point p;
	return p;
}

bool shape::pointInBounds( api_point p, api_point topLeftBound, api_point bottomRightBound )
{	if( p.getX() > topLeftBound.getX() && p.getX() < bottomRightBound.getX() && p.getY() > bottomRightBound.getY() && p.getY() < topLeftBound.getY() )
		return true;
	return false;
}

void shape::writeShape( FILE * out, int recordNumber )
{	writeRecordHeader( out, recordNumber );
	//Write a NULL SHAPE
	fwrite(&shapetype,sizeof(int),1, out);	
}

void shape::writeRecordHeader( FILE * out, int recordNumber )
{	void* intbuf;
	intbuf = (char*)&recordNumber;
	SwapEndian((char*)intbuf,sizeof(int));
	fwrite(intbuf,sizeof(char),sizeof(int), out);

	//Record Lengths are measured in 16 bit words
    int contentLength = recordbyteLength();
    contentLength -= 2*sizeof(int);	
    contentLength = contentLength/2;	
	intbuf = (char*)&contentLength;
	SwapEndian((char*)intbuf,sizeof(int));
	fwrite(intbuf,sizeof(char),sizeof(int), out);
}

void shape::bounds()
{	
	for( int i = 0; i < allPoints.size(); i++ )
	{	api_point shapebottomRight = allPoints[i];
		api_point shapetopLeft = allPoints[i];

		//Initialization
		if( i == 0 )
		{	bottomRight = shapebottomRight;
			topLeft = shapetopLeft;
		}
		else
		{	//X bounds
			if( shapebottomRight.getX() > bottomRight.getX() )
				bottomRight.setX( shapebottomRight.getX() );
			else if( shapetopLeft.getX() < topLeft.getX() )
				topLeft.setX( shapetopLeft.getX() );
			
			//Y bounds
			if( shapebottomRight.getY() < bottomRight.getY() )
				bottomRight.setY( shapebottomRight.getY() );
			else if( shapetopLeft.getY() > topLeft.getY() )
				topLeft.setY( shapetopLeft.getY() );
		}
	}
}

int shape::numberRecords()
{	return shapeRecord.size();
}

bool shape::setCell( cell c, int position )
{	if( position < 0 || position >= shapeRecord.size() )
	{	return false;
	}
	else
	{	shapeRecord[position] = c;	
		return true;
	}
}

cell shape::getCell( int position )
{	if( position >= 0 && position <= shapeRecord.size() )
		return shapeRecord[ position ];
	else
	{	cell c;
		return c;
	}	
}

void shape::createCell( cell c, int position )
{	
	if( position <= 0 )
		shapeRecord.push_front( c );
	else if( position >= shapeRecord.size() )
		shapeRecord.push_back( c );
	else
		shapeRecord.insert( shapeRecord.begin() + position, c );
}

void shape::deleteCell( int position )
{	if( position >= 0 || position < shapeRecord.size() )
		shapeRecord.erase( shapeRecord.begin() + position );	
}

#ifdef SHAPE_OCX
		std::deque<CString> shape::getMembers()
		{	std::deque<CString> members;
			return members;
		}

		bool shape::setMembers( std::deque<CString> members )
		{	return true;
		}
#endif

