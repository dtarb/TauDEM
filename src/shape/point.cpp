//#include "stdafx.h"

# include "point.h"

api_point::api_point()
{	x = 0.0;
	y = 0.0;
}

api_point::api_point( const api_point & p )
{	x = p.x;
	y = p.y;
}

api_point::api_point( double p_X, double p_Y )
{	x = p_X;
	y = p_Y;
}

double api_point::getX()
{	return x;
}

double api_point::getY()
{	return y;
}

void api_point::setX( double p_X )
{	x = p_X;
}

void api_point::setY( double p_Y )
{	y = p_Y;
}

#ifdef SHAPE_OCX
	std::deque<CString> api_point::getMembers()
		{	std::deque<CString> members;
			members.push_back( doubleToCString( x ) );
			members.push_back( doubleToCString( y ) );
			return members;
		}

		bool api_point::setMembers( std::deque<CString> members )
		{
			try
			{
				if( members.size() != 2 )
					throw( Exception("Cannot create point ... wrong number of parameters.") );

				x = CStringToDouble( members[0] );
				y = CStringToDouble( members[1] );
				return true;
			}
			catch( Exception e )
			{	return false;
			}
		}
#endif

