
EXES= scmtiff scmview

#-------------------------------------------------------------------------------

#CC = gcc -std=c99 -Wall -m64 -g
CC = gcc -std=c99 -Wall -m64 -fopenmp -O3
#CC = clang -m64 -Weverything -Wno-unused-parameter

CP = cp
RM = rm -f

CFLAGS =

#-------------------------------------------------------------------------------

ifeq ($(shell uname), Darwin)
	LIBOGL  = -framework GLUT -framework OpenGL
	LIBEXT  = -framework Cocoa -framework ApplicationServices -framework Carbon
endif

ifeq ($(shell uname), MINGW32_NT-6.1)
	LIBOGL  = -lopengl32
	LIBEXT  = -mwindows -lws2_32 -luser32 -lgdi32 -lwinmm
	LIBSDL  = -lmingw32
	CC     += -static
	CFLAGS += -DGLEW_STATIC
endif

ifeq ($(shell uname), Linux)
	LIBOGL  = -lglut -lGL
	LIBEXT  = -lpthread
endif

#-------------------------------------------------------------------------------

LIBTIF  = $(firstword $(wildcard /usr/local/lib/libtiff*.a \
				 /opt/local/lib/libtiff*.a \
				   C:/MinGW/lib/libtiff*.a \
				    $(HOME)/lib/libtiff*.a \
				       /usr/lib/libtiff*.a \
				   C:/MinGW/lib/libtiff*.a) -ltiff)

LIBGLEW = $(firstword $(wildcard /usr/local/lib/libGLEW*.a \
				 /opt/local/lib/libGLEW*.a \
				    $(HOME)/lib/libGLEW*.a \
				       /usr/lib/libGLEW*.a) -lglew32)

LIBJPG  = $(firstword $(wildcard /usr/local/lib/libjpeg*.a \
				 /opt/local/lib/libjpeg*.a \
				    $(HOME)/lib/libjpeg*.a \
				       /usr/lib/libjpeg*.a \
				   C:/MinGW/lib/libjpeg*.a) -ljpeg)

LIBPNG  = $(firstword $(wildcard /usr/local/lib/libpng*.a \
				 /opt/local/lib/libpng*.a \
				    $(HOME)/lib/libpng*.a \
				       /usr/lib/libpng*.a \
				   C:/MinGW/lib/libpng*.a) -lpng)

LIBZ    = $(firstword $(wildcard /usr/local/lib/libz*.a \
				 /opt/local/lib/libz*.a \
				    $(HOME)/lib/libz*.a \
				       /usr/lib/libz*.a \
				   C:/MinGW/lib/libz*.a) -lz)

#-------------------------------------------------------------------------------

ifneq ($(shell uname), Darwin)
	CFLAGS += -D_FILE_OFFSET_BITS=64 -DM_PI=3.14159265358979323846
endif

%.o : %.c Makefile
	$(CC) $(CFLAGS) -c $<

#-------------------------------------------------------------------------------

all : $(EXES);

install : $(EXES)
	$(CP) $(EXES) $(HOME)/bin

clean :
	$(RM) $(EXES) *.o

dist : all
	mkdir -p scm
	$(CP) scmtiff         scm
	$(CP) scmview         scm
	$(CP) doc/scmtiff.pdf scm

#-------------------------------------------------------------------------------

scmtiff     : err.o util.o scmdef.o scmdat.o scmio.o scm.o img.o jpg.o png.o tif.o pds.o convert.o combine.o mipmap.o border.o finish.o normal.o sample.o scmtiff.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBJPG) $(LIBTIF) $(LIBPNG) $(LIBZ) $(LIBEXT)

scmview : err.o util.o scmdef.o scmdat.o scmio.o scm.o img.o scmview.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBZ) $(LIBGLEW) $(LIBOGL) $(LIBEXT)

#-------------------------------------------------------------------------------

border.o : scm.h
border.o : util.h
combine.o : scm.h
combine.o : err.h
combine.o : util.h
convert.o : scm.h
convert.o : img.h
convert.o : util.h
img.o : img.h
img.o : err.h
img.o : util.h
jpg.o : img.h
jpg.o : err.h
mipmap.o : scm.h
mipmap.o : err.h
mipmap.o : util.h
normal.o : scm.h
normal.o : err.h
normal.o : util.h
pds.o : img.h
pds.o : err.h
png.o : img.h
png.o : err.h
scm.o : scmdat.h
scm.o : scmio.h
scm.o : scm.h
scm.o : err.h
scm.o : util.h
scm.h  : scmdat.h
scmdat.o : scmdat.h
scmdef.o : scmdef.h
scmio.o : scmdat.h
scmio.o : util.h
scmio.o : err.h
scmtiff.o : scm.h
scmtiff.o : err.h
scmview.o : scm.h
scmview.o : err.h
tif.o : img.h
tif.o : err.h
