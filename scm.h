#ifndef SCM_H
#define SCM_H

//------------------------------------------------------------------------------

typedef struct scm scm;

scm *scm_write_file(const char *, int, int, int);
int  scm_write_page(int);

scm *scm_read_file(const char *);
int  scm_read_page(int);

void scm_close(scm *);

//------------------------------------------------------------------------------

int  scm_get_n(scm *);                  // Page sample count excluding border
int  scm_get_d(scm *);                  // Page subdivision count
int  scm_get_c(scm *);                  // Sample channel count
int  scm_get_b(scm *);                  // Channel bit count
int  scm_get_s(scm *);                  // Channel signed flag

void scm_set_pixel(scm *, int, int, int, const double *);
void scm_get_pixel(scm *, int, int, int,       double *);
void scm_get_bound(scm *, int, int, int,       double *);

//------------------------------------------------------------------------------

#endif
