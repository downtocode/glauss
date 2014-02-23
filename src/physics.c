#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <unistd.h>
#include "physics.h"

/*	Default threads to use when system != linux. Everyone should have 2 physical CPUS now	*/
#define failsafe_cores 2

/*	Global vars	*/
int obj, width, height, objcount;
float dt;
long double gconst, epsno, elcharge;
bool enforced, quiet;

/*	Static vars	*/
static int i;
static v4sf accprev, forceconst = {0, 0};
static float spring = 500;
static long double pi;

/*	Indexing of cores = 1, 2, 3...	*/
unsigned short int avail_cores;
pthread_t *threads;
pthread_attr_t thread_attribs;
struct sched_param parameters;


data *objalt;
int looplimit1[8], looplimit2[8];

float dotprod(v4sf a, v4sf b)
{
	float result = a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; 
	return result;
}
float lenght(v4sf a)
{
	float result = a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
	return sqrt(result);
}

int initphys(data** object)
{
	*object = calloc(obj+1,sizeof(**object));
	for( i = 0; i < obj + 1; i++ ) {
		(*object)[i].linkwith = calloc(obj+1,sizeof(float));
	}
	
	printf("Allocated %lu bytes to object array.\n", (obj+1)*sizeof(**object)+(obj+1)*sizeof(float));
	
	pi = acos(-1);
	
	objalt = *object;
	
	int online_cores = 0;
	
	#ifdef __linux__
		online_cores = sysconf(_SC_NPROCESSORS_ONLN);
	#endif
	
	if(avail_cores == 0 && online_cores != 0 && enforced == 0) {
		avail_cores = online_cores;
		printf("System has %i processors, running program with %i threads\n", online_cores, avail_cores);
	} else if( avail_cores != 0 && online_cores != 0 && online_cores > avail_cores ) {
		printf("System has %i processors, running program with %i threads. You've got %i processors/threads free.\n", \
		online_cores, avail_cores, online_cores - avail_cores);
	} else if(avail_cores == 1) {
		printf("Running program in a single thread.\n");
	} else if(avail_cores > 1) {
		printf("Running program with %i threads\n", avail_cores);
	} else if(avail_cores == 0 && enforced == 0) {
		/*	Poor Mac OS...	*/
		avail_cores = failsafe_cores;
		printf("Unable to automatically determine processors, running with %i threads.\n", avail_cores);
	} else {
		fprintf(stderr, "Running program without force calculations.\n");
	}
	threads = calloc(avail_cores, sizeof(pthread_t));
	
	/*	Poorly documented. Just set an arbitrary value. IBM's documents use 3, so I figure 2's fine as well.	*/
	parameters.sched_priority = 2;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	Currently using Round-Robin scheduling, you can change it to SCHED_FIFO, but it doesn't seem make much difference.	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	
	
	/*	Split objects equally between cores	*/
	int totcore = (int)((float)obj/avail_cores);
	for(int k = 1; k < avail_cores + 1; k++) {
		looplimit1[k] = looplimit2[k-1] + 1;
		looplimit2[k] = looplimit1[k] + totcore - 1;
		if(k == avail_cores) {
			/*	Takes care of rounding problems with odd numbers. Unfair to the last thread	*/
			looplimit2[k] += obj - looplimit2[k];
		}
		/*	A loop with an end value of 0 won't execute, so it's fine	*/
		if(looplimit2[k] != 0 && quiet == 0) printf("Processor %i's objects = [%i,%i]\n", k, looplimit1[k], looplimit2[k]);
	}
	return 0;
}

void *resolveforces(void *arg) {
	v4sf vecnorm, grv = {0,0,0} , ele = {0,0,0}, link = {0,0,0};
	float mag;
	
	for(int j = looplimit1[(long)arg]; j < looplimit2[(long)arg] + 1; j++) {
		if(i != j) {
			vecnorm = objalt[j].pos - objalt[i].pos;
			mag = lenght(vecnorm);
			vecnorm /= mag;
			
			grv += vecnorm*((float)((gconst*objalt[i].mass*objalt[j].mass)/(mag*mag)));
			/*	future:use whole joints instead of individual stuff	*/
			ele += -vecnorm*((float)((objalt[i].charge*objalt[j].charge)/(4*pi*epsno*mag*mag)));
			
			if( objalt[i].linkwith[j] != 0 ) {
				link += vecnorm*((spring)*(mag - objalt[i].linkwith[j])*(float)0.2);
			}
			if( mag < objalt[i].radius + objalt[j].radius ) {
				/*	Removed real elastic collision code due to it being incomplete. Still need to do the math for 3D collisions.	*/
				link = -vecnorm*spring*3;
			}
		}
	}
	objalt[i].Ftot = forceconst + grv + ele + link;
	return 0;
}

int integrate(data* object)
{
	for(i = 1; i < obj + 1; i++) {
		if(object[i].ignore == 1) continue;
		if(object[i].pos[0] - object[i].radius < -2 || object[i].pos[0] + object[i].radius > 2) {
			object[i].vel[0] = -object[i].vel[0];
		}
		
		if(object[i].pos[1] - object[i].radius < -2 || object[i].pos[1] + object[i].radius > 2) {
			object[i].vel[1] = -object[i].vel[1];
		}
		
		if(object[i].pos[2] - object[i].radius < -2 || object[i].pos[2] + object[i].radius > 2) {
			object[i].vel[2] = -object[i].vel[2];
		}
		
		object[i].pos += (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		
		for(int k = 1; k < avail_cores + 1; k++) {
			pthread_create(&threads[k], &thread_attribs, resolveforces, (void*)(long)k);
		}
		for(int k = 1; k < avail_cores + 1; k++) {
			pthread_join(threads[k], NULL);
		}
		
		accprev = object[i].acc;
		
		object[i].acc = (object[i].Ftot)/(float)object[i].mass;
		object[i].vel += ((dt)/2)*(object[i].acc + accprev);
	}
	return 0;
}
