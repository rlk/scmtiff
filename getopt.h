#ifndef GETOPT_H
#define GETOPT_H

int getopt(int nargc, char * const nargv[], const char *ostr) ;

extern int   opterr;
extern int   optind;
extern int   optopt;
extern char *optarg;

#endif
