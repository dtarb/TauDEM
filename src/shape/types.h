/******************************************************************************
 * File:     types.h                                                          *
 * Author:   Jason Maestri                                                    *
 * Date:     22-Jun-1999                                                      *
 *                                                                            *
 *****************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** Description:                                                             **
 **        This file defines the various data types and constants used in    **
 **    the shape file classes.                                               **
 **                                                                          **
 ******************************************************************************
 *****************************************************************************/
#ifndef TYPES_H
#define TYPES_H

// Shape Type Codes:
#define API_SHP_NULLSHAPE   0
#define API_SHP_POINT       1
#define API_SHP_POLYLINE    3
#define API_SHP_POLYGON     5
#define API_SHP_MULTIPOINT  8
#define API_SHP_POINTZ      11
#define API_SHP_POLYLINEZ   13
#define API_SHP_POLYGONZ    15
#define API_SHP_MULTIPOINTZ 18
#define API_SHP_POINTM      21
#define API_SHP_POLYLINEM   23
#define API_SHP_POLYGONM    25
#define API_SHP_MULTIPOINTM 28
#define API_SHP_MULTIPATCH  31

//File Header Length (in 16 bit words)
#define FILE_HEADER_LEN 50
#define HEADER_BYTES 100

//File Code  and Version # (Used to properly identify a shape file)
#define FILE_CODE      9994
#define VERSION        1000
const int UnusedSize = 5;
#define UNUSEDVAL		0     	    

#define RECORD_HEADER_LEN 4
#define	REC_HEADER_BYTES 8

//Array Size
#define MAXBUFFER 255
#define MAX_FIELD_NAME_SIZE 11

#define MAXSTREAMS 3
#define PIE 3.14159

//COMPILE OPTIONS
//# define CONSOLE_TESTING
//# define LINUX_LIBRARY
//# define SHAPE_DLL
//# define SHAPE_OCX
	
	# ifdef SHAPE_OCX
		//# include <afx.h>		
		extern CString intToCString(int number);
		extern CString doubleToCString(double number);
		extern int CStringToInt( CString str );
		extern double CStringToDouble( CString str );
	# endif


	# ifndef DBF_FIELDTYPE_STRUCT
	# define DBF_FIELDTYPE_STRUCT

	typedef enum {
	  FTString,
	  FTInteger,
	  FTDouble,
	  FTInvalid
	} DBFFieldType;

	# endif

#endif



