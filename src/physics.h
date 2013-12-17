#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

//Add 4 to the vector size for every dimension.
typedef float v4sf __attribute__ ((vector_size (8)));

typedef struct {
	v4sf pos, vel, acc, Ftot, Fgrv, Fele, Flink;
	float mass, charge;
	unsigned short int linkwith;
	unsigned char links;
} data;


int initphys(data** object);
int integrate(data* object);

#endif
