# ifndef RECORD_H
# define RECORD_H

# include "types.h"
# include <stdlib.h>
# include <cstring>
# include <deque>
# include "exception.h"
# include <iostream>

class record
{	
	public:
		
		//CONSTRUCTORS
		record();
		record( const record & r );
		
		//DESTRUCTOR
		virtual ~record();


		//OPERATORS
		record operator=( const record & r );

		//DATA MEMBER ACCESS
		void setValue( const char * value );
		void setValue( int value );
		void setValue( double value );
		char * StringValue();
		int IntegerValue();
		double DoubleValue();
		
	private:
		char * recordStringValue;
		int recordIntegerValue;
		double recordDoubleValue;

	public:
		#ifdef SHAPE_OCX
			std::deque<CString> getMembers();
			bool setMembers( std::deque<CString> members );
		#endif
};

# endif
