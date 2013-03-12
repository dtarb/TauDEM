//#include "stdafx.h"
# include "cell.h"

//CONSTRUCTORS
cell::cell()
{	cellStringValue = NULL;
	cellIntegerValue = 0;
	cellDoubleValue = 0.0;
}

cell::cell( const cell & r )
{	if( r.cellStringValue == NULL )
		cellStringValue = NULL;
	else if( strlen( r.cellStringValue ) < 0 )
		cellStringValue = NULL;
	else
	{	cellStringValue = new char[ strlen( r.cellStringValue ) + 1];
		strcpy( cellStringValue, r.cellStringValue );
	}		
	cellIntegerValue = r.cellIntegerValue;
	cellDoubleValue = r.cellDoubleValue;
}

//DESTRUCTOR
cell::~cell()
{
	if (cellStringValue)
		delete cellStringValue;

	cellStringValue = NULL;
}

//OPERATORS
cell cell::operator=( const cell & r )
{	if( r.cellStringValue == NULL )
		cellStringValue = NULL;
	else if( strlen( r.cellStringValue ) < 0 )
		cellStringValue = NULL;
	else
	{	cellStringValue = new char[ strlen( r.cellStringValue ) + 1];
		strcpy( cellStringValue, r.cellStringValue );
	}		
	cellIntegerValue = r.cellIntegerValue;
	cellDoubleValue = r.cellDoubleValue;
	return *this;
}

//DATA MEMBER ACCESS
void cell::setValue( const char * value )
{	if( value != NULL && strlen( value ) > 0 )
	{	cellStringValue = new char[ strlen( value ) + 10];
		strcpy( cellStringValue, value );		
	}
	else
	{	cellStringValue = NULL;
	}
}

void cell::setValue( int value )
{	cellIntegerValue = value;
}

void cell::setValue( double value )
{	cellDoubleValue = value;
}

char * cell::StringValue()
{	if( cellStringValue != NULL )
		return cellStringValue;
	else
		return "";
}

int cell::IntegerValue()
{	return cellIntegerValue;
}

double cell::DoubleValue()
{	return cellDoubleValue;
}

#ifdef SHAPE_OCX
		std::deque<CString> cell::getMembers()
		{	std::deque<CString> members;
			members.push_back( cellStringValue );
			members.push_back( intToCString( cellIntegerValue ) );
			members.push_back( doubleToCString( cellDoubleValue ) );
			return members;
		}

		bool cell::setMembers( std::deque<CString> members )
		{	try
			{
				if( members.size() != 1 )
					throw( Exception("Cannot create cell ... wrong number of parameters.") );	
				
				setValue( members[0] );
				setValue( CStringToInt( members[0] ) );
				setValue( CStringToDouble( members[0] ) );			

				return true;					
			}
			catch( Exception e )
			{	return false;
			}
		}
#endif

