//#include "stdafx.h"


# include "field.h"

//CONSTRUCTORS
field::field()
{	type = FTInvalid;
	fieldWidth = 0;
	number_of_decimals = 0;
	fieldName = NULL;
}

field::field( const field & f )
{	
	if( f.fieldName != NULL && strlen( f.fieldName ) > 0 )
	{	fieldName = new char[ strlen( f.fieldName ) + 2 ];
		strcpy( fieldName, f.fieldName );
	}
	else
		fieldName = NULL;
	fieldWidth = f.fieldWidth;
	number_of_decimals = f.number_of_decimals;
	type = f.type;
}

field::field( const char * p_fieldName,
	   DBFFieldType p_type,
	   int p_fieldWidth,
	   int p_number_of_decimals )
{	
	if( p_fieldName != NULL )
	{	fieldWidth = strlen( p_fieldName ) + 2;
		fieldName = new char[ fieldWidth ];
		strcpy( fieldName, p_fieldName );		
	}
	if( p_fieldWidth > 0 )
		fieldWidth = p_fieldWidth;
	number_of_decimals = p_number_of_decimals;
	type = p_type;
}

//DESTRUCTOR
field::~field()
{
	if (fieldName)
		delete fieldName;

	fieldName = NULL;
}

//OPERATORS
field field::operator=( const field & f )
{
	if( f.fieldName != NULL && strlen( f.fieldName ) > 0 )
	{	fieldName = new char[ strlen( f.fieldName ) ];
		strcpy( fieldName, f.fieldName );
	}
	else
		fieldName = NULL;
	fieldWidth = f.fieldWidth;
	number_of_decimals = f.number_of_decimals;
	type = f.type;
	return *this;
}

//DATA MEMBER ACCESS
char * field::getFieldName()
{	return fieldName;
}

DBFFieldType field::getFieldType()
{	return type;
}

int field::getFieldWidth()
{	return fieldWidth;
}

int field::getNumberOfDecimals()
{	return number_of_decimals;
}

#ifdef SHAPE_OCX
		std::deque<CString> field::getMembers()
		{	std::deque<CString> members;
			members.push_back( fieldName );
			members.push_back( intToCString( fieldWidth ) );
			members.push_back( intToCString( number_of_decimals ) );
			members.push_back( intToCString( (int)type ) );
			return members;
		}

		bool field::setMembers( std::deque<CString> members )
		{	
			try
			{
				if( members.size() != 4 )
				{	try	
					{	if( strlen( members[0] ) <= 0 )
							throw( Exception("A field name must be specified.") );
						fieldName = new char[ strlen( members[0] ) ];
						strcpy( fieldName, members[0] );
						fieldWidth = CStringToInt( members[1] );
						number_of_decimals = CStringToInt( members[2] );
						type = (DBFFieldType)CStringToInt( members[3] );				
						return true;
					}
					catch( Exception e )
					{	return false;
					}
					
				}
				else
					throw( Exception("Cannot create field ... wrong number of parameters.") );
			}
			catch( Exception e )
			{	return false;
			}
		}
#endif

