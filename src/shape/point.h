# ifndef API_POINT_H
# define API_POINT_H
# include "types.h"
# include <deque>
# include "exception.h"

class api_point
{
	public:
		api_point();
		api_point( const api_point & p );
		api_point( double p_X, double p_Y );
		double getX();
		double getY();
		void setX( double p_X );
		void setY( double p_Y );
	private:
		double x;
		double y;
	public:

		#ifdef SHAPE_OCX
			std::deque<CString> getMembers();
			bool setMembers( std::deque<CString> members );
		#endif
};

# endif

