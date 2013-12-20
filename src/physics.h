#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdbool.h>
//Add 4 to the vector size for every dimension.
typedef float v4sf __attribute__ ((vector_size (16)));

typedef struct {
	v4sf pos, vel, acc, Ftot, Fgrv, Fele, Flink;
	long double mass, charge;
	bool linkwith[20];
} data;


int initphys(data** object);
int integrate(data* object);

#endif
