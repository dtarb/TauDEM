//#include "stdafx.h"
# include "item.h"

//CONSTRUCTORS
item::item()
{	itemStringValue = NULL;
	itemIntegerValue = 0;
	itemDoubleValue = 0.0;
}

item::item( const item & r )
{	if( r.itemStringValue == NULL )
		itemStringValue = NULL;
	else if( strlen( r.itemStringValue ) < 0 )
		itemStringValue = NULL;
	else
	{	itemStringValue = new char[ strlen( r.itemStringValue ) + 1];
		strcpy( itemStringValue, r.itemStringValue );
	}		
	itemIntegerValue = r.itemIntegerValue;
	itemDoubleValue = r.itemDoubleValue;
}

item::~item()
{
	if (itemStringValue)
		delete itemStringValue;

	itemStringValue = NULL;
}

//OPERATORS
item item::operator=( const item & r )
{	if( r.itemStringValue == NULL )
		itemStringValue = NULL;
	else if( strlen( r.itemStringValue ) < 0 )
		itemStringValue = NULL;
	else
	{	itemStringValue = new char[ strlen( r.itemStringValue ) + 1];
		strcpy( itemStringValue, r.itemStringValue );
	}		
	itemIntegerValue = r.itemIntegerValue;
	itemDoubleValue = r.itemDoubleValue;
	return *this;
}

//DATA MEMBER ACCESS
void item::setValue( const char * value )
{	if( value != NULL && strlen( value ) > 0 )
	{	itemStringValue = new char[ strlen( value ) + 10];
		strcpy( itemStringValue, value );		
	}
	else
	{	itemStringValue = NULL;
	}
}

void item::setValue( int value )
{	itemIntegerValue = value;
}

void item::setValue( double value )
{	itemDoubleValue = value;
}

char * item::StringValue()
{	if( itemStringValue != NULL )
		return itemStringValue;
	else
		return "";
}

int item::IntegerValue()
{	return itemIntegerValue;
}

double item::DoubleValue()
{	return itemDoubleValue;
}

#ifdef SHAPE_OCX
		std::deque<CString> item::getMembers()
		{	std::deque<CString> members;
			members.push_back( itemStringValue );
			members.push_back( intToCString( itemIntegerValue ) );
			members.push_back( doubleToCString( itemDoubleValue ) );
			return members;
		}

		bool item::setMembers( std::deque<CString> members )
		{	try
			{
				if( members.size() != 1 )
					throw( Exception("Cannot create item ... wrong number of parameters.") );	
				
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

