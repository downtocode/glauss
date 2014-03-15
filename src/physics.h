#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdbool.h>

/*	Vector size must be a power of 2 and big enough to contain the dimensions	*/
typedef float v4sf __attribute__ ((vector_size (16)));

typedef struct {
	unsigned int index;
	v4sf pos, vel, acc, Ftot;
	long double mass, charge;
	float radius, *linkwith;
	char ignore;
	bool center;
} data;

struct thread_settings {
	unsigned int looplimit1, looplimit2, threadid;
};

float dotprod(v4sf a, v4sf b);
float lenght(v4sf a);
int initphys(data** object);
int threadcontrol(int status);
void *resolveforces(void *arg);
void *integrate(void *arg);

#endif
