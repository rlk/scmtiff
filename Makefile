CC = cc
CP = cp

TARG = scmtiff
OPTS = -g -Wall
LIBS = -ljpeg -ltiff -lpng -lz
OBJS = error.o scm.o main.o

%.o : %.c
	$(CC) $(OPTS) -c $<

$(TARG) : $(OBJS)
	$(CC) $(OPTS) -o $(TARG) $(OBJS) $(LIBS)

install : $(TARG)
	$(CP) $(TARG) $(HOME)/bin

clean :
	$(RM) $(TARG) $(OBJS)
