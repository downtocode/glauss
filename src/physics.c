/*
 * This file is part of physengine.
 * Copyright (c) 2012 Rostislav Pehlivanov <atomnuker@gmail.com>
 * 
 * physengine is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * physengine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with physengine.  If not, see <http://www.gnu.org/licenses/>.
 */
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

/*	Indexing of cores = 1, 2, 3...	*/
pthread_t *threads;
pthread_attr_t thread_attribs;
pthread_mutex_t movestop;
pthread_barrier_t barrier;
struct sched_param parameters;
static bool running, quit;

int initphys(data** object)
{
	*object = calloc(option->obj+100,sizeof(data));
	for(int i = 0; i < option->obj + 1; i++ ) {
		(*object)[i].links = calloc(100,sizeof(unsigned int));
	}
	
	pprintf(8, "Allocated %lu bytes to object array.\n", \
	((option->obj+1)*sizeof(data)+(option->obj+1)*sizeof(float)));
	
	int online_cores = 0;
	
	#ifdef __linux__
		online_cores = sysconf(_SC_NPROCESSORS_ONLN);
	#endif
	
	if(option->avail_cores == 0 && online_cores != 0 ) {
		option->avail_cores = online_cores;
		pprintf(PRI_VERYHIGH, "Detected %i threads, will use all.\n", online_cores);
	} else if( option->avail_cores != 0 && online_cores != 0 && online_cores > option->avail_cores ) {
		pprintf(PRI_VERYHIGH, "Using %i out of %i threads.\n", option->avail_cores, online_cores);
	} else if(option->avail_cores == 1) {
		pprintf(PRI_VERYHIGH, "Running program in a single thread.\n");
	} else if(option->avail_cores > 1) {
		pprintf(PRI_VERYHIGH, "Running program with %i threads.\n", option->avail_cores);
	} else if(option->avail_cores == 0 ) {
		/*	Poor Mac OS...	*/
		option->avail_cores = failsafe_cores;
		pprintf(PRI_VERYHIGH, "Thread detection unavailable, running with %i thread(s).\n", failsafe_cores);
	}
	
	threads = calloc(option->avail_cores+1, sizeof(pthread_t));
	thread_opts = calloc(option->avail_cores+1, sizeof(struct thread_settings));
	
	parameters.sched_priority = 50;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	SCHED_RR - Round Robin, SCHED_FIFO - FIFO	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	return 0;
}

int threadseperate()
{
	/*
	 * Split objects equally between cores
	 * You can deliberatly load the last thread more by deducting a few objects in totcore.
	 */
	int totcore = (int)((float)option->obj/option->avail_cores);
	for(int k = 1; k < option->avail_cores + 1; k++) {
		thread_opts[k].threadid = k;
		thread_opts[k].looplimit1 = thread_opts[k-1].looplimit2 + 1;
		thread_opts[k].looplimit2 = thread_opts[k].looplimit1 + totcore - 1;
		if(k == option->avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_opts[k].looplimit2 += option->obj - thread_opts[k].looplimit2;
		}
		if(thread_opts[k].looplimit2 != 0)
			pprintf(PRI_MEDIUM, "Thread %i's objects = [%u,%u]\n", \
			k, thread_opts[k].looplimit1, thread_opts[k].looplimit2);
	}
	return 0;
}

int threadcontrol(int status, data** object)
{
	/*	Codes: 0 - unpause, 1-pause, 2-return status, 8 -start, 9 - destroy	*/
	if(status == 1 && running == 0) {
		for(int k = 1; k < option->avail_cores + 1; k++) thread_opts[k].dt = option->dt;
		running = 1;
		pthread_mutex_unlock(&movestop);
	} else if(status == 0 && running == 1) {
		running = 0;
		pthread_mutex_lock(&movestop);
	} else if(status == 2) {
		return running;
	} else if(status == 8) {
		threadseperate();
		pthread_mutex_init(&movestop, NULL);
		pthread_barrier_init(&barrier, NULL, option->avail_cores);
		running = 1;
		for(int k = 1; k < option->avail_cores + 1; k++) {
			if(thread_opts[k].obj != NULL) continue;
			thread_opts[k].dt = option->dt;
			thread_opts[k].obj = *object;
			pthread_create(&threads[k], &thread_attribs, resolveforces, (void*)(long)k);
		}
	} else if(status == 9) {
		running = 1;
		pthread_mutex_unlock(&movestop);
		quit = 1;
		for(int k = 1; k < option->avail_cores + 1; k++) {
			pthread_join(threads[k], NULL);
			thread_opts[k].obj = NULL;
		}
		pthread_barrier_destroy(&barrier);
		pthread_mutex_destroy(&movestop);
		running = 0;
		quit = 0;
	}
	return 0;
}

void *resolveforces(void *arg)
{
	struct thread_settings *thread = &thread_opts[(long)arg];
	v4sd vecnorm, accprev, Ftot, grv, ele, flj;
	double mag, grvmag, elemag, fljmag;
	
	Ftot = ele = grv = flj = (v4sd){0,0,0};
	mag = grvmag = elemag = fljmag = 0.0;
	
	long double pi = acos(-1);
	long double gconst = option->gconst, epsno = option->epsno;
	
	pthread_getcpuclockid(pthread_self(), &thread->clockid);
	
	while(!quit) {
		for(int i = thread->looplimit1; i < thread->looplimit2 + 1; i++) {
			if(thread->obj[i].ignore != '0') continue;
				if(thread->obj[i].pos[0] - thread->obj[i].radius < -100 || thread->obj[i].pos[0] + thread->obj[i].radius > 100) {
					thread->obj[i].vel[0] = -thread->obj[i].vel[0];
				}
				
				if(thread->obj[i].pos[1] - thread->obj[i].radius < -100 || thread->obj[i].pos[1] + thread->obj[i].radius > 100) {
					thread->obj[i].vel[1] = -thread->obj[i].vel[1];
				}
				
				if(thread->obj[i].pos[2] - thread->obj[i].radius < -100 || thread->obj[i].pos[2] + thread->obj[i].radius > 100) {
					thread->obj[i].vel[2] = -thread->obj[i].vel[2];
				}
			thread->obj[i].pos += (thread->obj[i].vel*thread->dt) + (thread->obj[i].acc)*((thread->dt*thread->dt)/2);
		}
		
		pthread_mutex_lock(&movestop);
		if(running) pthread_mutex_unlock(&movestop);
		pthread_barrier_wait(&barrier);
		
		for(int i = thread->looplimit1; i < thread->looplimit2 + 1; i++) {
			for(int j = 1; j < option->obj + 1; j++) {
				if(i==j) continue;
				vecnorm = thread->obj[j].pos - thread->obj[i].pos;
				mag = sqrt(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
				vecnorm /= mag;
				
				//grvmag = ((gconst*thread->obj[i].mass*thread->obj[j].mass)/(mag*mag));
				elemag = ((thread->obj[i].charge*thread->obj[j].charge)/(4*pi*epsno*mag*mag));
				//fljmag = (24/mag*mag)*(2*pow((1/mag),12.0) - pow((1/mag),6.0));
				
				//grv += grvmag*vecnorm;
				ele += -elemag*vecnorm;
				//flj += fljmag*vecnorm;
				
			}
			Ftot += ele;
			accprev = thread->obj[i].acc;
			thread->obj[i].acc = Ftot/thread->obj[i].mass;
			thread->obj[i].vel += (thread->obj[i].acc + accprev)*((thread->dt)/2);
			Ftot = ele = grv = flj = (v4sd){0,0,0};
		}
		if((long)arg == 1) thread->processed++;
		SDL_Delay(option->sleepfor);
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
