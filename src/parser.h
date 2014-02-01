#ifndef FUNCTIONS_J_INCLUDED
#define FUNCTIONS_J_INCLUDED

int preparser(float *dt, long double *elcharge, long double *gconst, long double *epsno, int *width, int *height, int *boxsize, char fontname[200], char filename[200]);
int parser(data** object, char filename[200]);

#endif
