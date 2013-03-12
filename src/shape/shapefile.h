# ifndef SHAPE_FILE
# define SHAPE_FILE

# include <deque>
# include <string>
# include "shape.h"
# include "shp_point.h"
# include "shp_polyline.h"
# include "shp_polygon.h"
# include "types.h"
# include "field.h"
# include "cell.h"

class shapefile
{
	public:
		
		//CONSTRUCTORS
		shapefile();
		~shapefile();
		shapefile( const char * p_shapefilename );
		shapefile( const shapefile & s );
				
		//OPERATORS		
		shape * operator[]( int shapenumber );
		shapefile operator=( const shapefile & s );
		
		//FUNCTIONS
		int insertShape( shape * s, int position );
		bool deleteShape( int position );
		void bounds();
		bool open( const char * p_shapefilename );
		bool create( const char * p_shapefilename, int p_shapetype );
		bool save( const char * p_shapefilename = NULL );
		bool close( const char * p_shapefilename = NULL );
		bool clear();

		//DATABASE FUNCTIONS
		int numberFields();
		field getField( int position );
		int insertField( field f, int position );
		bool deleteField( int position );
				
		//DATA MEMBER ACCESS
		int size();
		int shapeType();		
		shape * getShape( int shapenumber );

	protected:
		
		bool readDBFFile();
		bool readShpFile();	
		bool writeDBFFile( const char * p_shapefilename );		
		bool writeShxFile( const char * p_shapefilename );
		void writeFileHeader( FILE * out, int fileLength );
		int filebyteLength();			
		void SwapEndian(char* a,int size);
		void refreshVariables();

	private:
		const char * shapefilename;
		std::deque<shape *> allshapes;
		std::deque<field> tableFields;
		int shapetype;
		api_point topLeft;
		api_point bottomRight;

};
# endif

