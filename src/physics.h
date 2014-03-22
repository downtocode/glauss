#ifndef PHYSENGINE_PHYS
#define PHYSENGINE_PHYS

#include <stdbool.h>
#include <time.h>

typedef double v4sd __attribute__ ((vector_size (24)));

typedef struct {
	v4sd pos, vel, acc;
	long double mass, charge;
	float radius, *linkwith;
	char ignore, atom;
	unsigned int index;
} data;

struct thread_settings {
	unsigned int looplimit1, looplimit2, threadid;
	unsigned long long processed;
	struct timespec time;
	char timepercent[50];
};

struct thread_settings *thread_opts;

float dotprod(v4sd a, v4sd b);
float lenght(v4sd a);
int initphys(data** object);
int threadcontrol(int status);
void *resolveforces(void *arg);

#endif
