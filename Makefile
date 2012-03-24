
EXES= scmtiff scmview

#-------------------------------------------------------------------------------

#CC = gcc -std=c99 -Wall -m64 -g
#CC = gcc -std=c99 -Wall -m64 -fopenmp -O3
#CC = gcc -std=c99 -Wall -Werror -m64 -fopenmp -O3 -Wshorten-64-to-32
CC = clang -m64 -Weverything

CP = cp
RM = rm -f

CFLAGS = -Wall

ifneq ($(wildcard /usr/local),)
	CFLAGS += -I/usr/local/include
	LFLAGS += -L/usr/local/lib
endif

ifneq ($(wildcard /opt/local),)
	CFLAGS += -I/opt/local/include
	LFLAGS += -L/opt/local/lib
endif

ifeq ($(shell uname), Darwin)
	LFLAGS += -lGLEW -framework OpenGL -framework GLUT
else
	LFLAGS += -lGLEW -lglut -lGL
endif

%.o : %.c Makefile
	$(CC) $(CFLAGS) -c $<

#-------------------------------------------------------------------------------

all : $(EXES);

install : $(EXES)
	$(CP) $(EXES) $(HOME)/bin

clean :
	$(RM) $(EXES) *.o

#-------------------------------------------------------------------------------

scmtiff     : err.o util.o scmdat.o scmio.o scm.o img.o jpg.o png.o tif.o pds.o convert.o combine.o mipmap.o border.o relink.o normal.o scmtiff.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ -ljpeg -ltiff -lpng -lz

scmview : err.o util.o scmdat.o scmio.o scm.o img.o scmview.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ -lz

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
scmio.o : scmdat.h
scmio.o : util.h
scmio.o : err.h
scmtiff.o : scm.h
scmtiff.o : err.h
scmview.o : scm.h
scmview.o : err.h
tif.o : img.h
tif.o : err.h
