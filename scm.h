// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

//------------------------------------------------------------------------------

typedef struct scm scm;

scm *scm_ofile(const char *, int, int, int, int);
scm *scm_ifile(const char *);

void scm_close(scm *);

//------------------------------------------------------------------------------

off_t scm_append(scm *, off_t, const double *);

off_t scm_read_head(scm *, off_t, off_t *);
int   scm_read_data(scm *, off_t, double *);

//------------------------------------------------------------------------------

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_s(scm *);

const char *scm_get_copyright(scm *);
void        scm_set_copyright(scm *, const char *);

void scm_get_bound(scm *, int, int, int, double *);

//------------------------------------------------------------------------------

#endif
