#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <unistd.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include "physics.h"
#include "options.h"
#include "msg_phys.h"

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2
#define spring 500

/*	Global vars	*/
int obj, objcount;

/*	Static vars	*/
static long double pi;
static float dt;
static unsigned int avail_cores;
long double gconst, epsno, elcharge;

/*	Indexing of cores = 1, 2, 3...	*/
pthread_t *threads;
pthread_attr_t thread_attribs;
pthread_mutex_t movestop;
struct sched_param parameters;
static bool running, quit;

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
	for(int i = 0; i < obj + 1; i++ ) {
		(*object)[i].linkwith = calloc(obj+1,sizeof(float));
	}
	
	pprint(8, "Allocated %lu bytes to object array.\n", \
	((obj+1)*sizeof(**object)+(obj+1)*sizeof(float)));
	
	pi = acos(-1);
	
	objalt = *object;
	dt = option->dt;
	avail_cores = option->avail_cores;
	
	int online_cores = 0;
	
	#ifdef __linux__
		online_cores = sysconf(_SC_NPROCESSORS_ONLN);
	#endif
	
	if(avail_cores == 0 && online_cores != 0 ) {
		avail_cores = online_cores;
		printf("Detected %i threads, will use all.\n", online_cores);
	} else if( avail_cores != 0 && online_cores != 0 && online_cores > avail_cores ) {
		printf("Using %i out of %i threads.\n", avail_cores, online_cores);
	} else if(avail_cores == 1) {
		printf("Running program in a single thread.\n");
	} else if(avail_cores > 1) {
		printf("Running program with %i threads.\n", avail_cores);
	} else if(avail_cores == 0 ) {
		/*	Poor Mac OS...	*/
		avail_cores = failsafe_cores;
		printf("Thread detection unavailable, running with %i.\n", avail_cores);
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
	
	
	/*
	 * Split objects equally between cores
	 * You can deliberatly load one core more by deducting a few objects in totcore.
	 */
	int totcore = (int)((float)obj/avail_cores);
	for(int k = 1; k < avail_cores + 1; k++) {
		thread_opts[k].threadid = k;
		thread_opts[k].looplimit1 = thread_opts[k-1].looplimit2 + 1;
		thread_opts[k].looplimit2 = thread_opts[k].looplimit1 + totcore - 1;
		if(k == avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_opts[k].looplimit2 += obj - thread_opts[k].looplimit2;
		}
		if(thread_opts[k].looplimit2 != 0)
			pprint(5, "Thread %i's objects = [%u,%u]\n", \
			k, thread_opts[k].looplimit1, thread_opts[k].looplimit2);
	}
	option->avail_cores = avail_cores;
	
	return 0;
}

int threadcontrol(int status)
{
	/*	Codes: 0 - unpause, 1-pause, 2-return status, 8 -start 9 - destroy	*/
	if(status == 1 && running == 0) {
		dt = option->dt;
		running = 1;
		pthread_mutex_unlock(&movestop);
	} else if(status == 0 && running == 1) {
		dt = option->dt;
		running = 0;
	} else if(status == 2) {
		return running;
	} else if(status == 8) {
		pthread_mutex_init(&movestop, NULL);
		for(int k = 1; k < avail_cores + 1; k++) {
			pthread_create(&threads[k], &thread_attribs, resolveforces, (void*)(long)k);
		}
		running = 1;
	} else if(status == 9) {
		running = 1;
		pthread_mutex_unlock(&movestop);
		quit = 1;
		pthread_mutex_unlock(&movestop);
		for(int k = 1; k < avail_cores + 1; k++) {
			pthread_join(threads[k], NULL);
		}
		pthread_mutex_destroy(&movestop);
	}
	return 0;
}

void *resolveforces(void *arg) {
	struct thread_settings *thread = &thread_opts[(long)arg];
	v4sf vecnorm, accprev, Ftot = {0,0,0}, grv = {0,0,0}, ele = {0,0,0}, flj = {0,0,0}, link = {0,0,0};
	long double mag, grvmag, elemag = 0.0, fljmag, springmag;
	
	/*	TODO - LJ potential, collisions.	*/
	while(!quit) {
		for(int i = thread->looplimit1; i < thread->looplimit2 + 1; i++) {
			if(objalt[i].ignore != '0') continue;
				if(objalt[i].pos[0] - objalt[i].radius < -20 || objalt[i].pos[0] + objalt[i].radius > 20) {
					objalt[i].vel[0] = -objalt[i].vel[0];
				}
				
				if(objalt[i].pos[1] - objalt[i].radius < -20 || objalt[i].pos[1] + objalt[i].radius > 20) {
					objalt[i].vel[1] = -objalt[i].vel[1];
				}
				
				if(objalt[i].pos[2] - objalt[i].radius < -20 || objalt[i].pos[2] + objalt[i].radius > 20) {
					objalt[i].vel[2] = -objalt[i].vel[2];
				}
			objalt[i].pos += (objalt[i].vel*(v4sf){dt,dt,dt}) + (objalt[i].acc)*(v4sf){((dt*dt)/2),((dt*dt)/2),((dt*dt)/2)};
		}
		
		pthread_mutex_lock(&movestop);
		if(running) pthread_mutex_unlock(&movestop);
		
		for(int i = thread->looplimit1; i < thread->looplimit2 + 1; i++) {
			if(objalt[i].ignore != '0') continue;
			for(int j = 1; j < obj + 1; j++) {
				if(i==j) continue;
				vecnorm = objalt[j].pos - objalt[i].pos;
				mag = lenght(vecnorm);
				vecnorm /= (v4sf){mag, mag, mag};
				
				//grvmag = ((gconst*objalt[i].mass*objalt[j].mass)/(mag*mag));
				elemag = ((objalt[i].charge*objalt[j].charge)/(4*pi*epsno*mag*mag));
				//fljmag = (24/mag*mag)*(2*pow((1/mag),12.0) - pow((1/mag),6.0));
				
				//grv += vecnorm*(v4sf){grvmag,grvmag,grvmag};
				ele += -vecnorm*(v4sf){elemag,elemag,elemag};
				//flj += vecnorm*(v4sf){fljmag,fljmag,fljmag};
				
				if( objalt[i].linkwith[j] != 0 ) {
					if(mag > 3*objalt[i].linkwith[j]) objalt[i].linkwith[j] = 0;
					springmag = (spring)*(mag - objalt[i].linkwith[j]);
					link += vecnorm*(v4sf){springmag, springmag, springmag};
				}
			}
			Ftot += ele + link + flj;
			accprev = objalt[i].acc;
			objalt[i].acc = (Ftot)/(v4sf){objalt[i].mass,objalt[i].mass,objalt[i].mass};
			objalt[i].vel += (objalt[i].acc + accprev)*(v4sf){((dt)/2),((dt)/2),((dt)/2)};
			Ftot = ele = grv = flj = link = (v4sf){0,0,0,0};
		}
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread->time);
		//SDL_Delay(option->sleepfor);
	}
	return 0;
}
