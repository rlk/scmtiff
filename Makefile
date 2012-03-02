
EXES= scmtiff scmview

#-------------------------------------------------------------------------------

#CC = gcc -std=c99 -Wall -m64 -g
CC = gcc -std=c99 -Wall -m64 -fopenmp -O3

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

scmtiff     : err.o util.o scmdat.o scmio.o scm.o img.o jpg.o png.o tif.o pds.o convert.o combine.o mipmap.o border.o normal.o scmtiff.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ -ljpeg -ltiff -lpng -lz

scmview : err.o util.o scmdat.o scmio.o scm.o img.o scmview.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ -lz
