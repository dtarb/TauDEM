//#include "stdafx.h"
# include "shapefile.h"


shapefile::shapefile()
{	shapefilename = NULL;
}

shapefile::~shapefile()
{	clear();
}

shapefile::shapefile( const shapefile & s )
{
}

shapefile shapefile::operator=( const shapefile & s )
{	return *this;
}

shape * shapefile::operator[]( int shapenumber )
{	if( shapenumber >= 0 && shapenumber < size() )
		return allshapes[shapenumber];
	else
		return NULL;		
}

int shapefile::size()
{	return allshapes.size();
}

int shapefile::shapeType()
{	return shapetype;
}

shape * shapefile::getShape( int shapenumber )
{	if( shapenumber >= 0 && shapenumber < size() )
		return allshapes[shapenumber];
	else
		return NULL;
}

bool shapefile::open( const char * p_shapefilename )
{	
	try
	{	if( shapefilename == NULL || strlen( shapefilename ) < 0 )
		{	shapefilename = p_shapefilename;
			return ( readShpFile() && readDBFFile() );	
		}
		else
			throw( Exception( (std::string)shapefilename +
							  " must be closed before opening " +
							  (std::string)p_shapefilename ) );
		
	}
	catch( Exception e )
	{	return false;
	}
}

bool shapefile::create( const char * p_shapefilename, int p_shapetype )
{	
	try
	{	if( shapefilename == NULL || strlen( shapefilename ) < 0 )
		{	shapefilename = p_shapefilename;
			shapetype = p_shapetype;
			return true;
		}
		else
			throw( Exception( (std::string)shapefilename +
							  " must be closed before creating " +
							  (std::string)p_shapefilename ) );		
	}
	catch( Exception e )
	{	return false;
	}
	
}

bool shapefile::save( const char * p_shapefilename )
{	//Save
	if( p_shapefilename == NULL || strlen( p_shapefilename ) <= 0 )
	{}
	//Save As
	else
		shapefilename = p_shapefilename;	
	
	try
	{	FILE * shpout = fopen( shapefilename, "wb");

		if( shpout )
		{	//Get the latest bound changes
			bounds();
			writeFileHeader( shpout, filebyteLength() );
			for( int i = 0; i < allshapes.size(); i++ )
				allshapes[i]->writeShape( shpout, i );

			fclose( shpout );
			
			return writeShxFile( shapefilename ) && writeDBFFile( shapefilename );
				
		}
		else
			throw( Exception( "Could not open " + (std::string)shapefilename + " for writing." ) );
	}
	catch( Exception e )
	{	return false;
	}				
}

bool shapefile::close( const char * p_shapefilename )
{	
	//Close without Saving
	if( p_shapefilename == NULL || strlen( p_shapefilename ) <= 0 )
	{	refreshVariables();
		return true;
	}
	else
	{	if( save( p_shapefilename ) )
		{	refreshVariables();
			return true;
		}
		else
			return false;
	}
}

bool shapefile::clear()
{	refreshVariables();
	return true;
}

bool shapefile::writeShxFile( const char * p_shapefilename )
{	
	try
	{
		char * shxFile = new char[ strlen( p_shapefilename )+1]; //  DGT +1 Fix to avoid overflow because space for new line is not allocated 
		strcpy( shxFile, p_shapefilename );
		int shxLen = strlen( shxFile );
		if( shxLen >3 )
		{	
			shxFile[ shxLen - 3 ] = 's';
			shxFile[ shxLen - 2 ] = 'h';
			shxFile[ shxLen - 1 ] = 'x';		

			FILE * shxout = fopen( shxFile, "wb" );		
			
			if( shxout )
			{
				//WRITE .SHX FILE
				int fileLength = 0;
				fileLength += HEADER_BYTES;							//INDEX FILE HEADER
				fileLength += sizeof(int) * allshapes.size();		//RECORD OFFSETS
				fileLength += sizeof(int) * allshapes.size();		//RECORD CONTENT LENGTHS

				writeFileHeader( shxout, fileLength );			
				
				int offset = HEADER_BYTES;
				for( int i = 0; i < allshapes.size(); i++ )
				{	
					void * intbuf;
					int sixteenBitOffset = offset/2;
					intbuf = (char*)&sixteenBitOffset;
					SwapEndian((char*)intbuf,sizeof(int));
					fwrite(intbuf, sizeof(char),sizeof(int), shxout );

					int contentLength = allshapes[i]->recordbyteLength();
					offset += contentLength;
					
					//recordLength -= (2 * #HEADER LENGTH)
					contentLength -= 2*sizeof(int);	
					contentLength = contentLength/2;

					intbuf = (char*)&contentLength;
					SwapEndian((char*)intbuf,sizeof(int));
					fwrite(intbuf, sizeof(char),sizeof(int), shxout);																	
				}

				fclose( shxout );
				//END OF WRITE .SHX FILE	
			}
			else
				throw( Exception( "Could not open " + (std::string)shxFile + " for writing." ) );
		}
		else
			throw( Exception( "Could not open " + (std::string)shxFile + " for writing." ) );

		delete shxFile;
	}
	catch( Exception e )
	{	return false;
	}

	return true;
}

void shapefile::writeFileHeader( FILE * out, int fileLength )
{
	void* intbuf;
	int fileCode = FILE_CODE;
	intbuf = (char*)&fileCode;
	SwapEndian((char*)intbuf,sizeof(int));
	fwrite(intbuf,sizeof(char),sizeof(int),out);


	for(int i=0; i<UnusedSize; i++)
	{	int unused = UNUSEDVAL;
		intbuf = (char*)&unused;
		SwapEndian((char*)intbuf,sizeof(int));
		fwrite(intbuf,sizeof(char),sizeof(int),out);
	}
	
	//fileLength is measured in 16 bit words
	int tempfileLength = fileLength/2;
	intbuf = (char*)&tempfileLength;
	SwapEndian((char*)intbuf,sizeof(int));
	fwrite(intbuf, sizeof(char),sizeof(int),out);

	int version = VERSION;
	fwrite(&version, sizeof(int),1,out);
	
	int tempshapetype = shapetype;
	fwrite(&tempshapetype, sizeof(int),1,out);
	
	double shapefileBounds[8];
	shapefileBounds[0] = topLeft.getX();
	shapefileBounds[1] = bottomRight.getY();
	shapefileBounds[2] = bottomRight.getX();
	shapefileBounds[3] = topLeft.getY();
	shapefileBounds[4] = 0;
	shapefileBounds[5] = 0;
	shapefileBounds[6] = 0;
	shapefileBounds[7] = 0;
	
	fwrite(shapefileBounds,sizeof(double),8,out);
}

int shapefile::filebyteLength()
{	int totalbytes = HEADER_BYTES;
	for( int i = 0; i < size(); i++ )
		totalbytes += allshapes[i]->recordbyteLength();

	return totalbytes;
}

bool shapefile::readShpFile()	
{	
	//open shape file
	//Throw an Exception if unsuccessful
	try
	{	FILE* fHnd = fopen( shapefilename, "rb");
		if(!fHnd)
			throw( Exception( "Could not open " + (std::string)shapefilename )	);
			
		//################################################
		//## Get Header Info
		char intbuf[sizeof(int)];

		fread(intbuf,sizeof(char),sizeof(int),fHnd);
		SwapEndian(intbuf,sizeof(int));
		int fileCode = *(int*)intbuf;

		if(fileCode != FILE_CODE)
			throw( Exception( "Bad File Code in " + (std::string)shapefilename ) );			
				
		int * Unused = new int[ UnusedSize ];

		for(int i=0; i<UnusedSize; i++)
		{
			fread(intbuf, sizeof(char),sizeof(int),fHnd);
			SwapEndian(intbuf,sizeof(int));
			Unused[i] = *(int*)intbuf;

			if(Unused[i] != UNUSEDVAL)
				throw( Exception( "Bad Unused Value in " + (std::string)shapefilename ) );		
		}
		
		fread(intbuf, sizeof(char),sizeof(int),fHnd);
		SwapEndian(intbuf,sizeof(int));
		int fileLength = *(int*)intbuf;

		if(fileLength <= 50)
			throw( Exception( "Bad File Length in " + (std::string)shapefilename ) );
				
		int version;
		fread(&version, sizeof(int),1,fHnd);
		if(version != VERSION)
			throw( Exception( "Bad Version in " + (std::string)shapefilename ) );
		
		//Assign Class Variable shapetype
		fread(&shapetype, sizeof(int),1,fHnd);
			
		//Assign Class Variables topLeft && bottomRight
		double mybounds[8];
		fread(mybounds,sizeof(double),8,fHnd);
		topLeft = api_point( mybounds[0], mybounds[3] );
		bottomRight = api_point( mybounds[2], mybounds[1] );	
		//## End Get Header Info
		//################################################

		//get header values needed
		fileLength = fileLength*2;
		
		//local var's for reading in shape records
		int		bytesLeft	= fileLength - HEADER_BYTES;  //total length - header length
		
		while(bytesLeft > 0) 
		{			
			int	recordbytes	= 0;
			switch(shapetype)
			{	shape * sp;
				case API_SHP_POINT:	
					sp = new shp_point();
					if( sp->read( fHnd, recordbytes ) )
						allshapes.push_back( sp );
					break;	
				case API_SHP_POLYLINE:	
					sp = new shp_polyline();
					if( sp->read( fHnd, recordbytes ) )
						allshapes.push_back( sp );
					break;
				case API_SHP_POLYGON: 
					sp = new shp_polygon();
					if( sp->read( fHnd, recordbytes ) )
						allshapes.push_back( sp );
					break;					
				case API_SHP_NULLSHAPE:
					break;
				default:
					// DGT additions
					printf("Unreadable shape type in file: %s\n", shapefilename);
					printf("Shape has to be one of Point Polyline or Polygon with no Z or M coordinates.\n");
					printf("To convert a more complex shapefile to one of these see: http://resources.arcgis.com/content/kbase?fa=articleShow&d=30455\n");

					throw( Exception( "Cannot read this shapefile " + (std::string)shapefilename ) ); // DGT addition
					break;
			}
			bytesLeft -= recordbytes;	
		}

		delete Unused;
	}
	catch( Exception e )
	{	return false;
	}
	
	return true;
}

void shapefile::bounds()
{	for( int i = 0; i < allshapes.size(); i++ )
	{	api_point shapebottomRight = allshapes[i]->bottomRightBound();
		api_point shapetopLeft = allshapes[i]->topLeftBound();

		//Initialization
		if( i == 0 )
		{	bottomRight = shapebottomRight;
			topLeft = shapetopLeft;
		}
		else
		{	//X bounds
			if( shapebottomRight.getX() > bottomRight.getX() )
				bottomRight.setX( shapebottomRight.getX() );
			else if( shapetopLeft.getX() < topLeft.getX() )
				topLeft.setX( shapetopLeft.getX() );
			
			//Y bounds
			if( shapebottomRight.getY() < bottomRight.getY() )
				bottomRight.setY( shapebottomRight.getY() );
			else if( shapetopLeft.getY() > topLeft.getY() )
				topLeft.setY( shapetopLeft.getY() );
		}
	}
}

void shapefile::SwapEndian(char* a,int size) 
{
	char hold;
	for(int i=0; i<size/2; i++) {
		hold = a[i];
		a[i]=a[size-i-1];
		a[size-i-1] = hold;
	}
}

int shapefile::insertShape( shape * s, int position )
{	
	try
	{
		if( s == NULL )
			throw( Exception( "Could not insert shape." ) );
		
		for( int i = 0; i < tableFields.size(); i++ )
		{	s->createCell( cell(), 0 );
		}

		if( position <= 0 )
		{	allshapes.push_front( s );
			position = 0;
		}
		else if( position >= allshapes.size() )
		{	allshapes.push_back ( s );
			position = allshapes.size();
		}
		else
		{	allshapes.insert( allshapes.begin() + position, s );
			position = position;
		}		
		bounds();		
	}
	catch( Exception e )
	{	return -1;
	}
	return position;
}

bool shapefile::deleteShape( int position )
{	
	try
	{	if( position >= 0 && position <= allshapes.size() )
		{	allshapes.erase( allshapes.begin() + position );	
			bounds();
		}
		else
			throw( Exception("Bad index ... shape does not exist." ) );
	}
	catch( Exception e )
	{	return false;
	}

	return true;
}

//DATABASE FUNCTIONS
int shapefile::numberFields()
{	return tableFields.size();
}

field shapefile::getField( int fieldNumber )
{	
	if( fieldNumber >= 0 && fieldNumber < tableFields.size() )
		return tableFields[ fieldNumber ];
	else
	{	field f;
		return f;
	}
}

int shapefile::insertField( field f, int position )
{
	try
	{
		if( f.getFieldName() == NULL || strlen( f.getFieldName() ) <= 0 )
			throw( Exception("Could not insert field.  Bad field name." ) );

		if( position <= 0 ) 
		{	tableFields.push_front( f );
			position = 0;
		}
		else if( position >= tableFields.size() )
		{	tableFields.push_back( f );
			position = tableFields.size();
		}
		else
		{	tableFields.insert( tableFields.begin() + position, f );
			position = position;
		}
		
		//Insert a blank record into the shapes
		for( int i = 0; i < allshapes.size(); i++ )
		{	allshapes[i]->createCell( cell(), position );
		}		
	}
	catch( Exception e )
	{	return -1;
	}
	return position;
}

bool shapefile::deleteField( int position )
{	
	try
	{	if( position >= 0 && position < tableFields.size() )
		{	tableFields.erase( tableFields.begin() + position );
		
			//Delete the record in the shapes
			for( int i = 0; i < allshapes.size(); i++ )
				allshapes[i]->deleteCell( position );		
		}
		else
			throw( Exception("Could not delete field.  Index is out of bounds.") );		
	}
	catch( Exception e )
	{	return false;
	}

	return true;
}

bool shapefile::readDBFFile()
{	
	try
	{
		char * dbfFile = new char[ strlen( shapefilename )+1];  //  DGT +1 Fix to avoid overflow because space for new line is not allocated 
		strcpy( dbfFile, shapefilename );
		int dbfLen = strlen( dbfFile );
		if( dbfLen >3 )
		{	
			dbfFile[ dbfLen - 3 ] = 'd';
			dbfFile[ dbfLen - 2 ] = 'b';
			dbfFile[ dbfLen - 1 ] = 'f';

			DBFHandle handle = DBFOpen( dbfFile, "rb" );
			if( handle )
			{	int number_of_fields = DBFGetFieldCount( handle );
				int number_of_rows = DBFGetRecordCount( handle );
				
				if( number_of_rows == size() )
				{	
					//Populate Field Information
					for( int i = 0; i < number_of_fields; i++ )
					{	int width = 0;
						int decimals = 0;
						char * name = new char[ MAX_FIELD_NAME_SIZE + 2 ];
						DBFFieldType type = DBFGetFieldInfo( handle,
															 i,
															 name,
															 &width,
															 &decimals );
						field f( name, type, width, decimals );
						tableFields.push_back( f );

						//Populate shape Record Information
						for( int j = 0; j < number_of_rows; j++ )
						{	cell c;
							
							if( type == FTString )
							{	const char * value = DBFReadStringAttribute( handle, j, i );
								c.setValue( value );								
							}
							else if( type == FTInteger )
							{	int value = DBFReadIntegerAttribute( handle, j, i );
								c.setValue( value );								
							}
							else if( type == FTDouble )
							{	double value = DBFReadDoubleAttribute( handle, j, i );
								c.setValue( value );
							}
																			
							allshapes[j]->createCell( c, i );							
							//cell test = allshapes[j]->getCell( i );
						}
					}				
				}
				else
					throw( Exception( "Number of rows in " + (std::string)dbfFile + 
									  " does not match number of shapes in " +
									  (std::string)shapefilename ) );

				DBFClose( handle );
			}
			else
				throw( Exception( "Could not open " + (std::string)dbfFile ) );
		}
		else
			throw( Exception( "Could not open " + (std::string)dbfFile ) );

		delete dbfFile;
	}
	catch( Exception e )
	{	return false;
	}

	return true;
}

/*
typedef enum
{
	  FTString,		// fixed length string field 		
      FTInteger,	// numeric field with no decimals 
      FTDouble,		// numeric field with decimals 	
      FTInvalid     // not a recognised field type 	
} DBFFieldType;
*/
bool shapefile::writeDBFFile( const char * p_shapefilename )
{	
	try
	{
		char * dbfFile = new char[ strlen( p_shapefilename )+1];  //  DGT +1 Fix to avoid overflow because space for new line is not allocated 
		strcpy( dbfFile, p_shapefilename );
		int dbfLen = strlen( dbfFile );
		if( dbfLen >3 )
		{	
			dbfFile[ dbfLen - 3 ] = 'd';
			dbfFile[ dbfLen - 2 ] = 'b';
			dbfFile[ dbfLen - 1 ] = 'f';		

			//_unlink( dbfFile );

			DBFHandle handle = DBFCreate( dbfFile );
			if( handle )
			{	
				bool * fieldAdded = new bool[ tableFields.size() ];

				for( int i = 0; i < tableFields.size(); i++ )
				{	
					try
					{	if( DBFAddField( handle,
					        tableFields[i].getFieldName(),
							tableFields[i].getFieldType(),
							tableFields[i].getFieldWidth(),
							tableFields[i].getNumberOfDecimals() ) != - 1 )
						{
							fieldAdded[i] = true;
						}
						else
							throw("A field could not be added to " + (std::string)dbfFile );
					}
					catch( Exception e )
					{	fieldAdded[i] = false;						
					}
				}

				for( int j = 0; j < allshapes.size(); j++ )
				{
					for( int i = 0; i < tableFields.size(); i++ )
					{
						if( fieldAdded[i] )
						{
							DBFFieldType type = tableFields[i].getFieldType();
							cell c = allshapes[j]->getCell( i );
							
							if( type == FTString )
								DBFWriteStringAttribute( handle, j, i, c.StringValue() );
							else if( type == FTInteger )
								DBFWriteIntegerAttribute( handle, j, i, c.IntegerValue() );
							else if( type == FTDouble )
								DBFWriteDoubleAttribute( handle, j, i, c.DoubleValue() );
						}
					}		
				}
				DBFClose( handle );
			}
			else
				throw( Exception( "Could not open " + (std::string)p_shapefilename + " for writing." ) );
		}
		else
			throw( Exception( "Could not open " + (std::string)dbfFile + " for writing." ) );

		delete dbfFile;
	}
	catch( Exception e )
	{	return false;
	}
	return true;
}

void shapefile::refreshVariables()
{	

	for( int i = 0; i < allshapes.size(); i++ )
		allshapes[i] = NULL;
	allshapes.clear();
	tableFields.clear();
	
	shapetype = API_SHP_NULLSHAPE;
	shapefilename = NULL;
	bottomRight = api_point();
	topLeft = api_point();
}

