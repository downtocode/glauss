#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdbool.h>
//Vector size must be a power of 2 and big enough to contain the dimensions
typedef float v4sf __attribute__ ((vector_size (16)));

typedef struct {
	v4sf pos, vel, acc, Ftot, Fgrv, Fele, Flink;
	long double mass, charge;
	bool linkwith[20];
} data;


int initphys(data** object);
int integrate(data* object);

#endif
