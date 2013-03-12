# ifndef ITEM_H
# define ITEM_H

# include "types.h"
# include <stdlib.h>
# include <cstring>
# include <deque>
# include "exception.h"
# include <iostream>

class item
{	
	public:
		
		//CONSTRUCTORS
		item();
		item( const item & r );

		//DESTRUCTOR
		virtual ~item();

		//OPERATORS
		item operator=( const item & r );

		//DATA MEMBER ACCESS
		void setValue( const char * value );
		void setValue( int value );
		void setValue( double value );
		char * StringValue();
		int IntegerValue();
		double DoubleValue();
		
	private:
		char * itemStringValue;
		int itemIntegerValue;
		double itemDoubleValue;

	public:
		#ifdef SHAPE_OCX
			std::deque<CString> getMembers();
			bool setMembers( std::deque<CString> members );
		#endif
};

# endif
