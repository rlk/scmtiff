
EXES= scmtiff scmview

#-------------------------------------------------------------------------------

CC = cc
CP = cp
RM = rm -f

CFLAGS = -g -Wall

ifneq ($(wildcard /opt/local),)
	OPTS += -I/opt/local/include
	LIBS += -L/opt/local/lib
endif

%.o : %.c
	$(CC) $(CFLAGS) -c $<

#-------------------------------------------------------------------------------

all : $(EXES);

install : $(EXES)
	$(CP) $(EXES) $(HOME)/bin

clean :
	$(RM) $(EXES) *.o

#-------------------------------------------------------------------------------

scmtiff : err.o scm.o img.o jpg.o png.o tif.o pds.o convert.o combine.o mipmap.o border.o normal.o scmtiff.o
	$(CC) $(CFLAGS) -o $@ $^ -ljpeg -ltiff -lpng -lz

scmview : err.o scm.o img.o scmview.o
	$(CC) $(CFLAGS) -o $@ $^ -framework OpenGL -framework GLUT -lz
