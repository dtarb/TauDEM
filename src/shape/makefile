CC = mpic++
CFLAGS=-O2

all : cell.o dbf.o exception.o field.o item.o point.o record.o shape.o shapefile.o shapemain.o shp_point.o shp_polygon.o shp_polyline.o ReadOutlets.o

%.o : %.cpp
	$(CC) $(CFLAGS) $(INCDIRS) -c $<

clean :
	rm *.o 
