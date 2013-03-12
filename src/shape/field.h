# ifndef FIELD_H
# define FIELD_H

# include "dbf.h"
# include "types.h"
# include "exception.h"
# include <deque>

class field
{
	public:
		//CONSTRUCTORS
		field();
		field( const field & f );
		field( const char * p_fieldName,
			   DBFFieldType p_type,
			   int p_fieldWidth,
			   int p_number_of_decimals );

		//DESTRUCTOR
		virtual ~field();

		//OPERATORS
		field operator=( const field & f );

		//DATA MEMBER ACCESS
		char * getFieldName();
		DBFFieldType getFieldType();
		int getFieldWidth();
		int getNumberOfDecimals();

	private:
		DBFFieldType type;
		char * fieldName;
		int fieldWidth;
		int number_of_decimals;

	public:
		#ifdef SHAPE_OCX
			std::deque<CString> getMembers();
			bool setMembers( std::deque<CString> members );
		#endif

};
/*
    typedef enum
	{	FTString,			// fixed length string field 		
		FTInteger,		// numeric field with no decimals 	
		FTDouble,			// numeric field with decimals 		
		FTInvalid         // not a recognised field type 		
    } DBFFieldType;
*/
# endif

