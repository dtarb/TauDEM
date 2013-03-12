//#include "stdafx.h"
# include "record.h"

//CONSTRUCTORS
record::record()
{	recordStringValue = NULL;
	recordIntegerValue = 0;
	recordDoubleValue = 0.0;
}

record::record( const record & r )
{	if( r.recordStringValue == NULL )
		recordStringValue = NULL;
	else if( strlen( r.recordStringValue ) < 0 )
		recordStringValue = NULL;
	else
	{	recordStringValue = new char[ strlen( r.recordStringValue ) + 1];
		strcpy( recordStringValue, r.recordStringValue );
	}		
	recordIntegerValue = r.recordIntegerValue;
	recordDoubleValue = r.recordDoubleValue;
}

//DESTRUCTOR
record::~record()
{
	if (recordStringValue)
		delete recordStringValue;

	recordStringValue = NULL;
}

//OPERATORS
record record::operator=( const record & r )
{	if( r.recordStringValue == NULL )
		recordStringValue = NULL;
	else if( strlen( r.recordStringValue ) < 0 )
		recordStringValue = NULL;
	else
	{	recordStringValue = new char[ strlen( r.recordStringValue ) + 1];
		strcpy( recordStringValue, r.recordStringValue );
	}		
	recordIntegerValue = r.recordIntegerValue;
	recordDoubleValue = r.recordDoubleValue;
	return *this;
}

//DATA MEMBER ACCESS
void record::setValue( const char * value )
{	if( value != NULL && strlen( value ) > 0 )
	{	recordStringValue = new char[ strlen( value ) + 10];
		strcpy( recordStringValue, value );		
	}
	else
	{	recordStringValue = NULL;
	}
}

void record::setValue( int value )
{	recordIntegerValue = value;
}

void record::setValue( double value )
{	recordDoubleValue = value;
}

char * record::StringValue()
{	if( recordStringValue != NULL )
		return recordStringValue;
	else
		return "";
}

int record::IntegerValue()
{	return recordIntegerValue;
}

double record::DoubleValue()
{	return recordDoubleValue;
}

#ifdef SHAPE_OCX
		std::deque<CString> record::getMembers()
		{	std::deque<CString> members;
			members.push_back( recordStringValue );
			members.push_back( intToCString( recordIntegerValue ) );
			members.push_back( doubleToCString( recordDoubleValue ) );
			return members;
		}

		bool record::setMembers( std::deque<CString> members )
		{	try
			{
				if( members.size() != 1 )
					throw( Exception("Cannot create record ... wrong number of parameters.") );	
				
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

