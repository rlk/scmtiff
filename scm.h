// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

//------------------------------------------------------------------------------

typedef struct scm scm;

scm *scm_ofile(const char *, int, int, int, int, const char *);
scm *scm_ifile(const char *);

void scm_close(scm *);

//------------------------------------------------------------------------------

off_t scm_append(scm *, off_t, off_t, int, const double *);
off_t scm_rewind(scm *);

off_t  scm_read_node(scm *, off_t, off_t *);
size_t scm_read_page(scm *, off_t, double *);

//------------------------------------------------------------------------------

const char *scm_get_copyright(scm *);

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_s(scm *);

void scm_get_bound(scm *, int, int, int, double *);

//------------------------------------------------------------------------------

#endif
