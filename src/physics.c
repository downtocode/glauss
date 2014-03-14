#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <unistd.h>
#include "physics.h"

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2

/*	Global vars	*/
int obj, width, height, objcount;
float dt;
long double gconst, epsno, elcharge;
bool enforced, quiet;

/*	Static vars	*/
static int i;
static v4sf accprev, forceconst = {0, 0, 0};
static float spring = 500;
static long double pi;

/*	Indexing of cores = 1, 2, 3...	*/
unsigned short int avail_cores;
pthread_t *threads, physthread;
pthread_attr_t thread_attribs;
struct sched_param parameters;
struct thread_settings *thread_opts;
static bool thread_stop;

data *objalt;


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
	*object = calloc(obj+1,sizeof(data));
	for( i = 0; i < obj + 1; i++ ) {
		(*object)[i].linkwith = calloc(obj+1,sizeof(float));
	}
	
	if(quiet == 0)
		printf("Allocated %lu bytes to object array.\n", \
		((obj+1)*sizeof(**object)+(obj+1)*sizeof(float)));
	
	pi = acos(-1);
	
	objalt = *object;
	
	int online_cores = 0;
	
	#ifdef __linux__
		online_cores = sysconf(_SC_NPROCESSORS_ONLN);
	#endif
	
	if(avail_cores == 0 && online_cores != 0 && enforced == 0) {
		avail_cores = online_cores;
		if(quiet == 0)
		printf("System has %i processors, running program with %i threads\n", \
		online_cores, avail_cores);
	} else if( avail_cores != 0 && online_cores != 0 && online_cores > avail_cores ) {
		if(quiet == 0)
			printf("System has %i processors, running program with %i threads. \
			You've got %i processors/threads free.\n", \
			online_cores, avail_cores, online_cores - avail_cores);
	} else if(avail_cores == 1 && quiet == 0) {
		printf("Running program in a single thread.\n");
	} else if(avail_cores > 1 && quiet == 0) {
		printf("Running program with %i threads\n", avail_cores);
	} else if(avail_cores == 0 && enforced == 0) {
		/*	Poor Mac OS...	*/
		avail_cores = failsafe_cores;
		printf("Unable to automatically determine processors, running with %i threads.\n", avail_cores);
	} else {
		fprintf(stderr, "Running program without force calculations.\n");
	}
	
	threads = calloc(avail_cores+1, sizeof(pthread_t));
	thread_opts = calloc(avail_cores+1, sizeof(struct thread_settings));
	
	/*	Poorly documented.  IBM's documents use 3, so I figure it's fine.	*/
	parameters.sched_priority = 3;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	SCHED_RR - Round Robin, SCHED_FIFO - FIFO	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	
	
	/*	Split objects equally between cores	*/
	int totcore = (int)((float)obj/avail_cores);
	for(int k = 1; k < avail_cores + 1; k++) {
		thread_opts[k].threadid = k;
		thread_opts[k].looplimit1 = thread_opts[k-1].looplimit2 + 1;
		thread_opts[k].looplimit2 = thread_opts[k].looplimit1 + totcore - 1;
		if(k == avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_opts[k].looplimit2 += obj - thread_opts[k].looplimit2;
		}
		/*	A loop with an end value of 0 won't execute, so it's fine	*/
		if(thread_opts[k].looplimit2 != 0 && quiet == 0)
			printf("Processor %i's objects = [%u,%u]\n", \
			k, thread_opts[k].looplimit1, thread_opts[k].looplimit2);
	}
	return 0;
}

static bool running;

int threadcontrol(int status) {
	//Codes: 0-terminate, 1-start, 2-return status, 3-query clock cycles
	if(status == 1 && running == 0) {
		pthread_create(&physthread, NULL, integrate, NULL);
		running = 1;
	} else if(status == 0 && running == 1) {
		thread_stop = 1;
		running = 0;
	} else if(status == 2) return running;
	return 0;
}

void *resolveforces(void *arg) {
	struct thread_settings *thread = &thread_opts[(long)arg];
	
	v4sf vecnorm, grv = {0,0,0} , ele = {0,0,0}, link = {0,0,0};
	long double mag, grvmag, elemag, springmag;
	
	/*	TODO - LJ potential, collisions.	*/
	
	for(int j = thread->looplimit1; j < thread->looplimit2 + 1; j++) {
		if(thread->objid != j) {
			vecnorm = objalt[j].pos - objalt[thread->objid].pos;
			mag = lenght(vecnorm);
			vecnorm /= (v4sf){mag, mag, mag};
			
			grvmag = ((gconst*objalt[thread->objid].mass*objalt[j].mass)/(mag*mag));
			elemag = ((objalt[thread->objid].charge*objalt[j].charge)/(4*pi*epsno*mag*mag));
			
			grv += vecnorm*(v4sf){grvmag,grvmag,grvmag};
			ele += -vecnorm*(v4sf){elemag,elemag,elemag};
			
			if( objalt[i].linkwith[j] != 0 ) {
				springmag = (spring)*(mag - objalt[thread->objid].linkwith[j])*0.1;
				link += vecnorm*(v4sf){springmag, springmag, springmag};
			}
			if( mag < objalt[thread->objid].radius + objalt[j].radius ) {
				link = -vecnorm*(v4sf){spring*3, spring*3, spring*3};
			}
		}
	}
	objalt[thread->objid].Ftot = objalt[thread->objid].Ftot + grv + ele + link;
	return 0;
}

void *integrate(void *arg)
{
	while(1) {
		for(i = 1; i < obj + 1; i++) {
			if(objalt[i].ignore != '0') continue;
			if(objalt[i].pos[0] - objalt[i].radius < -2 || objalt[i].pos[0] + objalt[i].radius > 2) {
				objalt[i].vel[0] = -objalt[i].vel[0];
			}
			
			if(objalt[i].pos[1] - objalt[i].radius < -2 || objalt[i].pos[1] + objalt[i].radius > 2) {
				objalt[i].vel[1] = -objalt[i].vel[1];
			}
			
			if(objalt[i].pos[2] - objalt[i].radius < -2 || objalt[i].pos[2] + objalt[i].radius > 2) {
				objalt[i].vel[2] = -objalt[i].vel[2];
			}
			
			objalt[i].pos += (objalt[i].vel*(v4sf){dt,dt,dt}) + (objalt[i].acc)*(v4sf){((dt*dt)/2),((dt*dt)/2),((dt*dt)/2)};

			for(int k = 1; k < avail_cores + 1; k++) {
				thread_opts[k].objid = i;
				pthread_create(&threads[k], &thread_attribs, resolveforces, (void*)(long)k);
			}
			for(int k = 1; k < avail_cores + 1; k++) {
				pthread_join(threads[k], NULL);
			}
			
			accprev = objalt[i].acc;
			objalt[i].acc = (objalt[i].Ftot)/(v4sf){objalt[i].mass,objalt[i].mass,objalt[i].mass};
			objalt[i].vel += (objalt[i].acc + accprev)*(v4sf){((dt)/2),((dt)/2),((dt)/2)};
		}
		if(thread_stop) {
			thread_stop = 0;
			pthread_exit(NULL);
		}
	}
	return 0;
}
