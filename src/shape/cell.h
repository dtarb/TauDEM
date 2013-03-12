# ifndef CELL_H
# define CELL_H

# include "types.h"
# include <stdlib.h>
# include <cstring>
# include <deque>
# include "exception.h"
# include <iostream>

class cell
{	
	public:
		
		//CONSTRUCTORS
		cell();
		cell( const cell & r );

		//DESTRUCTOR
		virtual ~cell();

		//OPERATORS
		cell operator=( const cell & r );

		//DATA MEMBER ACCESS
		void setValue( const char * value );
		void setValue( int value );
		void setValue( double value );
		char * StringValue();
		int IntegerValue();
		double DoubleValue();
		
	private:
		char * cellStringValue;
		int cellIntegerValue;
		double cellDoubleValue;

	public:
		#ifdef SHAPE_OCX
			std::deque<CString> getMembers();
			bool setMembers( std::deque<CString> members );
		#endif
};

# endif
