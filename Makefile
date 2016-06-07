
EXES= scmtiff scmogle scmjpeg

#-------------------------------------------------------------------------------

# This build goes out of its way to use GCC instead of LLVM (the current default
# under OS X) to ensure the availability of OpenMP.

CC = /usr/local/bin/gcc -std=c99 -Wall -m64 -fopenmp -O3
# CC = /usr/local/bin/gcc -std=c99 -Wall -m64 -g
# CC = /usr/local/bin/gcc -std=c99 -Wall
# CC = gcc -std=c99 -Wall -m64 -fopenmp -O3
# CC = gcc -std=c99 -Wall -m64 -O3
# CC = gcc -std=c99 -Wall -m64 -g

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
	$(CP) prune.h    $(SRCDIR)
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

scmtiff     : err.o util.o scmdef.o scmdat.o scmio.o scm.o img.o jpg.o png.o tif.o pds.o extrema.o convert.o rectify.o combine.o mipmap.o border.o prune.o finish.o polish.o normal.o sample.o scmtiff.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBJPG) $(LIBTIF) $(LIBPNG) $(LIBZ) $(LIBEXT)

scmogle : err.o util.o scmdef.o scmdat.o scmio.o scm.o img.o scmogle.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBZ) $(LIBGLEW) $(LIBOGL) $(LIBEXT)

scmjpeg : err.o scmjpeg.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBTIF) $(LIBJPG) $(LIBZ)

#-------------------------------------------------------------------------------

border.o :  border.c scm.h scmdat.h scmdef.h util.h process.h
combine.o : combine.c scm.h scmdat.h err.h util.h process.h
convert.o : convert.c scm.h scmdat.h scmdef.h img.h config.h err.h util.h process.h
err.o :     err.c err.h
extrema.o : extrema.c img.h config.h util.h
finish.o :  finish.c scm.h scmdat.h err.h util.h process.h
getopt.o :  getopt.c
img.o :     img.c config.h img.h err.h util.h
jpg.o :     jpg.c img.h config.h err.h
mipmap.o :  mipmap.c scm.h scmdat.h scmdef.h err.h util.h process.h
normal.o :  normal.c scm.h scmdat.h scmdef.h err.h util.h process.h
pds.o :     pds.c config.h img.h err.h util.h
png.o :     png.c img.h config.h err.h
polish.o :  polish.c scm.h scmdat.h err.h util.h process.h
prune.o :   prune.c scm.h scmdat.h scmdef.h process.h
rectify.o : rectify.c scm.h scmdat.h scmdef.h img.h config.h err.h util.h process.h
sample.o :  sample.c scm.h scmdat.h scmdef.h util.h
scm.o :     scm.c scmdef.h scmdat.h scmio.h scm.h err.h util.h
scmdat.o :  scmdat.c scmdat.h util.h
scmdef.o :  scmdef.c scmdef.h
scmio.o :   scmio.c scmdat.h scmio.h util.h err.h
scmjpeg.o : scmjpeg.c err.h
scmogle.o : scmogle.c scm.h scmdat.h scmdef.h err.h util.h
scmtiff.o : scmtiff.c config.h scm.h scmdat.h err.h process.h
tif.o :     tif.c img.h config.h err.h
util.o :    util.c config.h util.h
