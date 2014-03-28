#ifndef PHYSENGINE_PHYS
#define PHYSENGINE_PHYS

#include <stdbool.h>
#include <time.h>

#ifdef __clang__
/*
 * Clang supports both OpenCL and GCC vectors.
 * The former have feature parity with GCC's GCC vectors. Only on newer Clang
 * versions however. We could siplify the equations quite a bit if we dropped support for Clang below 3.3
 * GCC's vectors on the other hand, do not support scalar operations on vectors. However, they are still
 * better than GCC's own GCC vector support because we're not limited to just power of two vector size.
 * Better to choose the lesser of two evils and have double support for Clang by using OpenCL's vectors.
 * Segmentation is always a bad choice but I don't see any other way.
 */
typedef double v4sd __attribute__((ext_vector_type(3)));
#else
/*
 * I'm starting to get tired of GCC's antics. No non-power-of-two vectors, INCREDIBLY bad performance
 * with double precision when using 32 byte vectors, I just don't get it. It can optimize basic programs
 * but man does it fail here. Leaving it float for now just to get somewhat nice performance out of GCC.
 */
typedef float v4sd __attribute__ ((vector_size (16)));
#endif

typedef struct {
	v4sd pos, vel, acc;
	long double mass, charge;
	float radius, *linkwith;
	char ignore, atom;
	unsigned int index;
} data;

struct thread_settings {
	data* obj;
	unsigned int looplimit1, looplimit2, threadid;
	unsigned long long processed;
	clockid_t clockid;
};

struct thread_settings *thread_opts;

int initphys(data** object);
int threadseperate();
int threadcontrol(int status, data** object);
void *resolveforces(void *arg);

#endif
