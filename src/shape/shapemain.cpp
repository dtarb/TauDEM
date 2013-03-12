//#include "stdafx.h"
# include <iostream>
# include <fstream>
# include <string>
# include "shapefile.h"


std::string error;		//global string for error messages to be put into



//----------------------------------------
//----------### CONSOLE TESTING ###-------
//----------------------------------------
# ifndef DBF_FIELDTYPE_STRUCT
# define DBF_FIELDTYPE_STRUCT

typedef enum {
  FTString,
  FTInteger,
  FTDouble,
  FTInvalid
} DBFFieldType;
# endif

# ifdef CONSOLE_TESTING
void main()
{
	//### Test 1 ### Getting and Changing a Point on a Polyline
	shapefile s;
	s.open( "c:\\windows\\desktop\\stream_network_test\\seg.shp" );	
	shape * sh = s[0];
	api_point p = sh->getPoint( 1 );
	sh->insertPoint( p, 1 );
	p.setX( p.getX() + 200 );
	p.setY( p.getY() - 200 );	
	sh->setPoint( p, 1 );
	s.close( "c:\\windows\\desktop\\stream_network_test\\apiTest.shp");
	//### End Test 1 ###

	/*
	//### Test 2 ### Creating a polyline shapefile
	shapefile s;
	s.open( "c:\\windows\\desktop\\stream_network_test\\seg.shp" );	
	shapefile s2;
	s2.create("c:\\windows\\desktop\\stream_network_test\\test2.shp", 3 );
	field f("Test", FTString, 10, 0 );
	s2.insertField( f, 0 );
	cell c;	
	c.setValue(0);

	shape * sh = s[0];
	s2.insertShape( sh, 0 );
	s2[0]->setCell( c, 0 );
	sh = s[1];
	s2.insertShape( sh, 0 );
	s2[0]->setCell( c, 0 );
	s2.close("c:\\windows\\desktop\\stream_network_test\\test2.shp");
	//### End Test 2 ###
	*/
	/*
	//### Test 2 ### Deleting a valid shape index
	shapefile s;
	s.open( "c:\\windows\\desktop\\seg.shp" );	
	s.deleteShape( 0 );
	s.close( "c:\\windows\\desktop\\apiTest.shp");
	//### End Test 2 ###
	*/

	/*
	//### Test 3 ### Adding a field to the Database
	shapefile s;
	s.open( "c:\\windows\\desktop\\seg.shp" );	
	field f( "TEST", FTString, 10, 0 );
	s.insertField( f, 1 );
	shape * sh = s[0];	
	cell c;
	c.setValue( "Testing" );
	sh->setCell( c, 1 );
	s.close( "c:\\windows\\desktop\\apiTest.shp");
	//### End Test 3
	*/

	//### Test 4 ### Getting a record Value from a Shape
	//### End Test 4 ###

	/*
	//### Test 5 ### Deleting a field from a shapefile
	shapefile s;
	s.open( "c:\\windows\\desktop\\seg.shp" );	
	s.deleteField( 0 );
	s.close( "c:\\windows\\desktop\\apiTest.shp");
	//### End Test 5 ###
	*/

	/*
	//### Test 6 ### Error Opening a shape file
	shapefile s;
	if( !s.open( "c:\\windows\\desktop\\nonexistentfile.shp" ) )
		cout<<error.c_str()<<endl;
	s.close();
	//### End Test 6 ###
	*/

	/*
	//### Test 7 ### Opening and Closing a shape file
	shapefile s;
	s.open( "c:\\windows\\desktop\\seg.shp" );	
	s.close( "c:\\windows\\desktop\\apiTest.shp" );
	if( !s.open( "c:\\windows\\desktop\\apiTest.shp" ) )
		cout<<error.c_str()<<endl;
	else
		s.close();
	//### End Test 7 ###
	*/

	/*
	//### Test 8 ### Opening two shape files on same class instance
	shapefile s;
	s.open( "c:\\windows\\desktop\\seg.shp" );	
	if( !s.open( "c:\\windows\\desktop\\apiTest.shp" ) )
		cout<<error.c_str()<<endl;
	s.close();
	//### End Test 8 ###
	*/

	/*
	//### Test 9 ### Save As
	shapefile s;
	s.open( "c:\\windows\\desktop\\seg.shp" );
	s.save( "c:\\windows\\desktop\\apiTest.shp" );
		
		//Change a point
		shape * sh = s[0];
		api_point p = sh->getPoint( 1 );
		p.setX( p.getX() + 200 );
		p.setY( p.getY() - 200 );
		sh->insertPoint( p, 1 );

	s.save();
	s.close();
	//### End Test 9 ###
	*/

	/*
	//### Test 10 ### Getting a Point
	shapefile s;
	if( s.open( "c:\\windows\\desktop\\apiTest.shp" ) )
	{	shape * sh = s[0];
		api_point p = sh->getPoint( 1 );
		cout<<p.getX()<<" "<<p.getY()<<endl;
		s.close();
	}
	else
		cout<<error.c_str()<<endl;
	//### End Test 10 ###
	*/	
	
}
# endif

//----------------------------------------
//----### END OF CONSOLE TESTING ###------
//----------------------------------------






//----------------------------------------
//---------### LINUX LIBRARY ###----------
//----------------------------------------

//LINKING FROM EXECUTABLE
//g++ -g shapeapitest.cpp -o shapeapitest -L. -lshapeapi

# ifdef  LINUX_LIBRARY

//GLOBAL SHAPEFILE USED FOR WRAPPING API FOR LINUX
std::deque<	shapefile * > s;

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

int shp_open( const char * shapefilename )
{	shapefile * sh = new shapefile();
	if( sh->open( shapefilename ) )
	{	s.push_back( sh );
		return s.size() - 1;
	}
	else
		return -1;
}

int shp_create( const char * shapefilename, int shapetype )
{	shapefile * sh = new shapefile();
	if( sh->create( shapefilename, shapetype ) )
	{	s.push_back( sh );
		return s.size() - 1;
	}
	else
		return -1;
}

bool shp_save( int shp_handle, const char * shapefilename )
{	return s[shp_handle]->save( shapefilename );
}

bool shp_close( int shp_handle, const char * shapefilename )
{	return s[shp_handle]->close( shapefilename );
}

bool shp_clear( int shp_handle )
{	return s[shp_handle]->clear();
}

int shp_getShapetype( int shp_handle )
{	return s[shp_handle]->shapeType();
}

int shp_numberShapes( int shp_handle )
{	return s[shp_handle]->size();
}

int shp_insertField( int shp_handle, int fieldPosition, char * name, int fieldWidth, int number_of_decimals, DBFFieldType type )
{	field f( name, type, fieldWidth, number_of_decimals );
	return s[shp_handle]->insertField( f, fieldPosition );
}

bool shp_deleteField( int shp_handle, int fieldPosition )
{	return s[shp_handle]->deleteField( fieldPosition );
}

char * shp_getField( int shp_handle, int fieldPosition, int & fieldWidth, int & number_of_decimals, DBFFieldType & type  )
{	field f = s[shp_handle]->getField( fieldPosition );
	
	fieldWidth = f.getFieldWidth();
	number_of_decimals = f.getNumberOfDecimals();
	type = f.getFieldType();
	return f.getFieldName();
}

int shp_numberFields( int shp_handle )
{	return s[shp_handle]->numberFields();
}

char * shp_getRecord_String( int shp_handle, int shapePosition, int fieldPosition )
{	field f = s[shp_handle]->getField(fieldPosition);
	return s[shp_handle]->getShape(shapePosition)->getCell( fieldPosition ).StringValue();	
}

int shp_getRecord_Integer( int shp_handle, int shapePosition, int fieldPosition )
{	field f = s[shp_handle]->getField(fieldPosition);
	return s[shp_handle]->getShape(shapePosition)->getCell( fieldPosition ).IntegerValue();	
}

double shp_getRecord_Double( int shp_handle, int shapePosition, int fieldPosition )
{	field f = s[shp_handle]->getField(fieldPosition);
	return s[shp_handle]->getShape(shapePosition)->getCell( fieldPosition ).DoubleValue();	
}

bool shp_editRecord( int shp_handle, const char * stringValue, int intValue, double doubleValue, int shapePosition, int fieldPosition )
{	cell c;
	c.setValue( stringValue );
	c.setValue( intValue );
	c.setValue( doubleValue );

	if( s[shp_handle]->getShape(shapePosition)->setCell( c, fieldPosition ) )
		return true;	
	else
		return false;
}

int shp_insertPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition )
{	api_point p( x, y );
	return s[shp_handle]->getShape(shapePosition)->insertPoint( p, pointPosition );
}

bool shp_deletePoint( int shp_handle, int shapePosition, int pointPosition )
{	return s[shp_handle]->getShape(shapePosition)->deletePoint( pointPosition );
}

bool shp_setPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition )
{	api_point p( x, y );
	return s[shp_handle]->getShape(shapePosition)->setPoint( p, pointPosition );
}

bool shp_deleteShape( int shp_handle, int shapePosition )
{	return s[shp_handle]->deleteShape( shapePosition );
}

int shp_createShape( int shp_handle, int shapePosition )
{	int shapeType = s[shp_handle]->shapeType();
	if( shapeType == API_SHP_POINT )
	{	shape * sh = new shp_point();
		return	s[shp_handle]->insertShape( sh, shapePosition );
	}
	else if( shapeType == API_SHP_POLYLINE )
	{	shape * sh = new shp_polyline();
		return s[shp_handle]->insertShape( sh, shapePosition );
	}
	else if( shapeType == API_SHP_POLYGON )
	{	shape * sh = new shp_polygon();
		return s[shp_handle]->insertShape( sh, shapePosition );
	}
	return -1;
}

void shp_getPoint( int shp_handle, int shapePosition, int pointPosition, double & x, double & y )
{	api_point result = s[shp_handle]->getShape(shapePosition)->getPoint( pointPosition );
	x = result.getX();
	y = result.getY();
}

int shp_numberPoints( int shp_handle, int shapePosition )
{	return s[shp_handle]->getShape(shapePosition)->size();
}

std::string shp_getError()
{	return error;
}

# endif

//----------------------------------------
//-------### END OF LINUX LIBRARY ###-----
//----------------------------------------








//----------------------------------------
//---------### SHAPEAPI DLL  ###----------
//----------------------------------------
# ifdef  SHAPE_DLL
# include <windows.h>

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  ul_reason_for_call, 
                      LPVOID lpReserved)
{
    switch( ul_reason_for_call )
	{
		case DLL_PROCESS_ATTACH:
		{
			break;
		}
		case DLL_THREAD_ATTACH:
		{
			break;
		}
		case DLL_THREAD_DETACH:
		{
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
    }

    return TRUE;
}

//GLOBAL SHAPEFILE USED FOR WRAPPING API FOR DLL
std::deque<	shapefile * > s;

//### FUNCTION HEADERS ###

//IO
__declspec( dllexport ) int shp_open( const char * shapefilename );
__declspec( dllexport ) int shp_create( const char * shapefilename, int shapetype );
__declspec( dllexport ) bool shp_save( int shp_handle, const char * shapefilename = NULL );
__declspec( dllexport ) bool shp_close( int shp_handle, const char * shapefilename = NULL );

//REFRESH
__declspec( dllexport ) bool shp_clear( int shp_handle );

//TYPE INFO
__declspec( dllexport ) int shp_getShapetype( int shp_handle );
__declspec( dllexport ) int shp_numberShapes( int shp_handle );

//DATABASE
__declspec( dllexport ) int shp_insertField( int shp_handle, int fieldPosition, char * name, int fieldWidth, int number_of_decimals, DBFFieldType type );
__declspec( dllexport ) bool shp_deleteField( int shp_handle, int fieldPosition );
__declspec( dllexport ) char * shp_getField( int shp_handle, int fieldPosition, int & fieldWidth, int & number_of_decimals, DBFFieldType & type  );
__declspec( dllexport ) int shp_numberFields( int shp_handle );
__declspec( dllexport ) char * shp_getRecord_String( int shp_handle, int shapePosition, int fieldPosition );
__declspec( dllexport ) int shp_getRecord_Integer( int shp_handle, int shapePosition, int fieldPosition );
__declspec( dllexport ) double shp_getRecord_Double( int shp_handle, int shapePosition, int fieldPosition );
__declspec( dllexport ) bool shp_editRecord( int shp_handle, const char * stringValue, int intValue, double doubleValue, int shapePosition, int fieldPosition );

//SHAPES
__declspec( dllexport ) int shp_insertPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition );
__declspec( dllexport ) bool shp_deletePoint( int shp_handle, int shapePosition, int pointPosition );
__declspec( dllexport ) bool shp_setPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition );
__declspec( dllexport ) bool shp_deleteShape( int shp_handle, int shapePosition );
__declspec( dllexport ) int shp_createShape( int shp_handle, int shapePosition );
__declspec( dllexport ) void shp_getPoint( int shp_handle, int shapePosition, int pointPosition, double & x, double & y );
__declspec( dllexport ) int shp_numberPoints( int shp_handle, int shapePosition );

//ERRORS
__declspec( dllexport ) std::string shp_getError();

//### END FUNCTION HEADERS ###

__declspec( dllexport ) int shp_open( const char * shapefilename )
{	shapefile * sh = new shapefile();
	if( sh->open( shapefilename ) )
	{	s.push_back( sh );
		return s.size() - 1;
	}
	else
		return -1;
}

__declspec( dllexport ) int shp_create( const char * shapefilename, int shapetype )
{	shapefile * sh = new shapefile();
	if( sh->create( shapefilename, shapetype ) )
	{	s.push_back( sh );
		return s.size() - 1;
	}
	else
		return -1;
}

__declspec( dllexport ) bool shp_save( int shp_handle, const char * shapefilename )
{	return s[shp_handle]->save( shapefilename );
}

__declspec( dllexport ) bool shp_close( int shp_handle, const char * shapefilename )
{	return s[shp_handle]->close( shapefilename );
}

__declspec( dllexport ) bool shp_clear( int shp_handle )
{	return s[shp_handle]->clear();
}

__declspec( dllexport ) int shp_getShapetype( int shp_handle )
{	return s[shp_handle]->shapeType();
}

__declspec( dllexport ) int shp_numberShapes( int shp_handle )
{	return s[shp_handle]->size();
}

__declspec( dllexport ) int shp_insertField( int shp_handle, int fieldPosition, char * name, int fieldWidth, int number_of_decimals, DBFFieldType type )
{	field f( name, type, fieldWidth, number_of_decimals );
	return s[shp_handle]->insertField( f, fieldPosition );
}

__declspec( dllexport ) bool shp_deleteField( int shp_handle, int fieldPosition )
{	return s[shp_handle]->deleteField( fieldPosition );
}

__declspec( dllexport ) char * shp_getField( int shp_handle, int fieldPosition, int & fieldWidth, int & number_of_decimals, DBFFieldType & type  )
{	field f = s[shp_handle]->getField( fieldPosition );
	
	fieldWidth = f.getFieldWidth();
	number_of_decimals = f.getNumberOfDecimals();
	type = f.getFieldType();
	return f.getFieldName();
}

__declspec( dllexport ) int shp_numberFields( int shp_handle )
{	return s[shp_handle]->numberFields();
}

__declspec( dllexport ) char * shp_getCell_String( int shp_handle, int shapePosition, int fieldPosition )
{	field f = s[shp_handle]->getField(fieldPosition);
	return s[shp_handle]->getShape(shapePosition)->getCell( fieldPosition ).StringValue();	
}

__declspec( dllexport ) int shp_getCell_Integer( int shp_handle, int shapePosition, int fieldPosition )
{	field f = s[shp_handle]->getField(fieldPosition);
	return s[shp_handle]->getShape(shapePosition)->getCell( fieldPosition ).IntegerValue();	
}

__declspec( dllexport ) double shp_getCell_Double( int shp_handle, int shapePosition, int fieldPosition )
{	field f = s[shp_handle]->getField(fieldPosition);
	return s[shp_handle]->getShape(shapePosition)->getCell( fieldPosition ).DoubleValue();	
}

__declspec( dllexport ) bool shp_setCell( int shp_handle, const char * stringValue, int intValue, double doubleValue, int shapePosition, int fieldPosition )
{	cell c;
	c.setValue( stringValue );
	c.setValue( intValue );
	c.setValue( doubleValue );

	if( s[shp_handle]->getShape(shapePosition)->setCell( c, fieldPosition ) )
		return true;	
	else
		return false;
}

__declspec( dllexport ) int shp_insertPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition )
{	api_point p( x, y );
	return s[shp_handle]->getShape(shapePosition)->insertPoint( p, pointPosition );
}

__declspec( dllexport ) bool shp_deletePoint( int shp_handle, int shapePosition, int pointPosition )
{	return s[shp_handle]->getShape(shapePosition)->deletePoint( pointPosition );
}

__declspec( dllexport ) bool shp_setPoint( int shp_handle, double x, double y, int shapePosition, int pointPosition )
{	api_point p( x, y );
	return s[shp_handle]->getShape(shapePosition)->setPoint( p, pointPosition );
}

__declspec( dllexport ) bool shp_deleteShape( int shp_handle, int shapePosition )
{	return s[shp_handle]->deleteShape( shapePosition );
}

__declspec( dllexport ) int shp_createShape( int shp_handle, int shapePosition )
{	int shapeType = s[shp_handle]->shapeType();
	if( shapeType == API_SHP_POINT )
	{	shape * sh = new shp_point();
		return	s[shp_handle]->insertShape( sh, shapePosition );
	}
	else if( shapeType == API_SHP_POLYLINE )
	{	shape * sh = new shp_polyline();
		return s[shp_handle]->insertShape( sh, shapePosition );
	}
	else if( shapeType == API_SHP_POLYGON )
	{	shape * sh = new shp_polygon();
		return s[shp_handle]->insertShape( sh, shapePosition );
	}
	return -1;
}

__declspec( dllexport ) void shp_getPoint( int shp_handle, int shapePosition, int pointPosition, double & x, double & y )
{	api_point result = s[shp_handle]->getShape(shapePosition)->getPoint( pointPosition );
	x = result.getX();
	y = result.getY();
}

__declspec( dllexport ) int shp_numberPoints( int shp_handle, int shapePosition )
{	return s[shp_handle]->getShape(shapePosition)->size();
}

__declspec( dllexport ) std::string shp_getError()
{	return error;
}

# endif

//----------------------------------------
//------### END OF SHAPEAPI DLL  ###------
//----------------------------------------
