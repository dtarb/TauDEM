Linux Compilation Instructions
==============================

Type `make` on the command line to initiate compilation.

It may be necessary to modify `makefile`:

 * Uncomment the line `INCDIRS=-I/usr/lib/openmpi/include -I/usr/include/gdal`
 * On the line beginning with `all :` remove the `clean` target from the end