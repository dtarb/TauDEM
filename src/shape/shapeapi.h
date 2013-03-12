# ifndef SHAPEAPI_H
# define SHAPEAPI_H

# include <string>

# ifndef DBF_FIELDTYPE_STRUCT
# define DBF_FIELDTYPE_STRUCT

typedef enum {
  FTString,
  FTInteger,
  FTDouble,
  FTInvalid
} DBFFieldType;

# endif

//### FUNCTION HEADERS ###

//IO
int shp_open( const char * shapefilename );
int shp_create( const char * shapefilename, int shapetype );
bool shp_save( int shp_handle, const char * shapefilename = NULL );
bool shp_close( int shp_handle, const char * shapefilename = NULL );

//REFRESH
bool shp_clear( int shp_handle );

//TYPE INFO
int shp_getShapetype( int shp_handle );
int shp_numberShapes( int shp_handle );

//DATABASE
int shp_insertField( int shp_handle, int fieldPosition, char * name, int fieldWidth, int number_of_decimals, DBFFieldType type );
bool shp_deleteField( int shp_handle, int fieldPosition );
char * shp_getField( int shp_handle, int fieldPosition, int & fieldWidth, int & number_of_decimals, DBFFieldType & type  );
int shp_numberFields( int shp_handle );
char * shp_getCell_String( int shp_handle, int shapePosition, int fieldPosition );
int shp_getCell_Integer( int shp_handle, int shapePosition, int fieldPosition );
double shp_getCell_Double( int shp_handle, int shapePosition, int fieldPosition );
bool shp_setCell( int shp_handle, const char * stringValue, int intValue, double doubleValue, int shapePosition, int fieldPosition );

//SHAPES
int shp_insertPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition );
bool shp_deletePoint( int shp_handle, int shapePosition, int pointPosition );
bool shp_setPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition );
bool shp_deleteShape( int shp_handle, int shapePosition );
int shp_createShape( int shp_handle, int shapePosition );
void shp_getPoint( int shp_handle, int shapePosition, int pointPosition, double & x, double & y );
int shp_numberPoints( int shp_handle, int shapePosition );

//ERRORS
std::string shp_getError();

//### END FUNCTION HEADERS ###
# endif