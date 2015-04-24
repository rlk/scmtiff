
EXES= scmtiff scmogle scmjpeg

#-------------------------------------------------------------------------------

# This build goes out of its way to use GCC instead of LLVM (the current default
# under OS X) to ensure the availability of OpenMP.
CC = gcc -std=c99 -Wall -m64 -fopenmp -O3
#CC = /usr/local/bin/gcc -std=c99 -Wall -m64 -fopenmp -O3
#CC = /usr/local/bin/gcc -std=c99 -Wall -m64 -g
#CC = /usr/local/bin/gcc -std=c99 -Wall
#CC = gcc -std=c99 -Wall -m64 -fopenmp -O3
#CC = gcc -std=c99 -Wall -m64 -O3
#CC = gcc -std=c99 -Wall -m64 -g

CP = cp
RM = rm -f

CFLAGS =

#-------------------------------------------------------------------------------

ifeq ($(shell uname), Darwin)
	LIBOGL  = -framework GLUT -framework OpenGL
	LIBEXT  = -framework Cocoa -framework ApplicationServices -framework Carbon
	LIBGLEW = -lglew32
endif

ifeq ($(shell uname), MINGW32_NT-6.1)
	LIBOGL  = -lopengl32
	LIBEXT  = -mwindows -lws2_32 -luser32 -lgdi32 -lwinmm
	LIBSDL  = -lmingw32
	CC     += -static
	CFLAGS += -DGLEW_STATIC
	LIBGLEW = -lglew32
endif

ifeq ($(shell uname), Linux)
	LIBOGL  = -lglut -lGL
	LIBEXT  = -lpthread
	LIBGLEW = -lGLEW -lGLU -lGL
endif

#-------------------------------------------------------------------------------

LIBTIF  = $(firstword $(wildcard /usr/local/lib/libtiff*.a \
				 /opt/local/lib/libtiff*.a \
				   C:/MinGW/lib/libtiff*.a \
				    $(HOME)/lib/libtiff*.a \
				       /usr/lib/libtiff*.a \
				   C:/MinGW/lib/libtiff*.a) -ltiff)

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

ifneq ($(wildcard /opt/local/include),)
        CFLAGS += -I/opt/local/include
endif

ifneq ($(wildcard /usr/local/include),)
        CFLAGS += -I/usr/local/include
endif

ifeq ($(shell uname), Darwin)
	CFLAGS += -Wno-deprecated-declarations
else
	CFLAGS += -D_FILE_OFFSET_BITS=64 -DM_PI=3.14159265358979323846
endif

#-------------------------------------------------------------------------------

%.o : %.c Makefile
	$(CC) $(CFLAGS) -c $<

#-------------------------------------------------------------------------------

all : $(EXES);

install : $(EXES)
	$(CP) $(EXES) $(HOME)/bin

clean :
	$(RM) $(EXES) *.o

#-------------------------------------------------------------------------------

BINDIR= scmtiff-bin-$(shell svnversion)
SRCDIR= scmtiff-src-$(shell svnversion)

dist : all
	mkdir -p         $(BINDIR)

	$(CP) scmtiff    $(BINDIR)
	$(CP) scmogle    $(BINDIR)

	zip -r $(BINDIR) $(BINDIR)

dist-src:
	mkdir -p         $(SRCDIR)
	mkdir -p         $(SRCDIR)/etc

	$(CP) Makefile   $(SRCDIR)
	$(CP) border.c   $(SRCDIR)
	$(CP) combine.c  $(SRCDIR)
	$(CP) convert.c  $(SRCDIR)
	$(CP) err.c      $(SRCDIR)
	$(CP) err.h      $(SRCDIR)
	$(CP) extrema.c  $(SRCDIR)
	$(CP) finish.c   $(SRCDIR)
	$(CP) img.c      $(SRCDIR)
	$(CP) img.h      $(SRCDIR)
	$(CP) jpg.c      $(SRCDIR)
	$(CP) mipmap.c   $(SRCDIR)
	$(CP) normal.c   $(SRCDIR)
	$(CP) pds.c      $(SRCDIR)
	$(CP) png.c      $(SRCDIR)
	$(CP) polish.h   $(SRCDIR)
	$(CP) process.h  $(SRCDIR)
	$(CP) rectify.c  $(SRCDIR)
	$(CP) sample.c   $(SRCDIR)
	$(CP) scm.c      $(SRCDIR)
	$(CP) scm.h      $(SRCDIR)
	$(CP) scmdat.c   $(SRCDIR)
	$(CP) scmdat.h   $(SRCDIR)
	$(CP) scmdef.c   $(SRCDIR)
	$(CP) scmdef.h   $(SRCDIR)
	$(CP) scmio.c    $(SRCDIR)
	$(CP) scmio.h    $(SRCDIR)
	$(CP) scmtiff.c  $(SRCDIR)
	$(CP) scmogle.c  $(SRCDIR)
	$(CP) tif.c      $(SRCDIR)
	$(CP) util.c     $(SRCDIR)
	$(CP) util.h     $(SRCDIR)
	$(CP) COPYING    $(SRCDIR)

	$(CP) etc/Makefile-DTM $(SRCDIR)/etc
	$(CP) etc/Makefile-WAC $(SRCDIR)/etc
	$(CP) etc/NAC_ROI.sh   $(SRCDIR)/etc

	zip -r $(SRCDIR) $(SRCDIR)

#-------------------------------------------------------------------------------

scmtiff     : err.o util.o scmdef.o scmdat.o scmio.o scm.o img.o jpg.o png.o tif.o pds.o extrema.o convert.o rectify.o combine.o mipmap.o border.o finish.o polish.o normal.o sample.o scmtiff.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBJPG) $(LIBTIF) $(LIBPNG) $(LIBZ) $(LIBEXT) -lm

scmogle : err.o util.o scmdef.o scmdat.o scmio.o scm.o img.o scmogle.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBZ) $(LIBGLEW) $(LIBOGL) $(LIBEXT) -lm

scmjpeg : err.o scmjpeg.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBTIF) $(LIBJPG) $(LIBZ)

#-------------------------------------------------------------------------------

border.o : scm.h
border.o : util.h
combine.o : scm.h
combine.o : err.h
combine.o : util.h
convert.o : scm.h
convert.o : img.h
convert.o : util.h
extrema.o : img.h
extrema.o : util.h
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
scmogle.o : scm.h
scmogle.o : err.h
tif.o : img.h
tif.o : err.h
