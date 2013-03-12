//#include "stdafx.h"

# include "exception.h"

Exception::Exception()
{	msg="";
}

//GLOBAL STRING error THAT ERROR MESSAGE IS SET INTO
extern std::string error;

Exception::Exception( std::string errormsg)
{	msg = errormsg;
	error = msg;	
}

std::string Exception::getError()
{	return msg;	
}

