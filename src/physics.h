#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED


typedef float v4sf __attribute__ ((vector_size (16)));

typedef struct {
	v4sf pos, vel, acc, Ftot, Fgrv, Fele;
	float mass, charge;
} data;


int initphys(data** object);
int integrate(data* object);

#endif
