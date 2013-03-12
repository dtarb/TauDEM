# ifndef EXCEPTION_H
# define EXCEPTION_H

# include <string>

class Exception
{
	public:
		Exception();
		Exception( std::string errormsg );
		std::string getError();
	private:
		std::string msg;
};

# endif

