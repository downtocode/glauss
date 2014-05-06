/*
 * This file is part of physengine.
 * Copyright (c) 2013 Rostislav Pehlivanov <atomnuker@gmail.com>
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
#include "physics.h"
#include "options.h"
#include "msg_phys.h"

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2

/*	Indexing of cores = 1, 2, 3...	*/
pthread_t *threads;
pthread_attr_t thread_attribs;
pthread_mutex_t movestop;
pthread_barrier_t barrier;
struct sched_param parameters;
static bool running, quit;

#define sigma 0.5
#define epsilon 0.0001

int initphys(data** object)
{
	*object = calloc(option->obj+100,sizeof(data));
	
	if(*object != NULL) pprintf(PRI_OK, "Allocated %lu bytes(%u objects) to object array at %p.\n", \
	(option->obj+1)*sizeof(data), option->obj+1, *object);
	
	int online_cores = 0;
	
	#ifdef __linux__
		online_cores = sysconf(_SC_NPROCESSORS_ONLN);
	#endif
	
	if(option->avail_cores == 0 && online_cores != 0 ) {
		option->avail_cores = online_cores;
		pprintf(PRI_OK, "Detected %i threads, will use all.\n", online_cores);
	} else if( option->avail_cores != 0 && online_cores != 0 && online_cores > option->avail_cores ) {
		pprintf(PRI_VERYHIGH, "Using %i out of %i threads.\n", option->avail_cores, online_cores);
	} else if(option->avail_cores == 1) {
		pprintf(PRI_VERYHIGH, "Running program in a single thread.\n");
	} else if(option->avail_cores > 1) {
		pprintf(PRI_VERYHIGH, "Running program with %i threads.\n", option->avail_cores);
	} else if(option->avail_cores == 0 ) {
		/*	Poor Mac OS...	*/
		option->avail_cores = failsafe_cores;
		pprintf(PRI_WARN, "Thread detection unavailable, running with %i thread(s).\n", failsafe_cores);
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

int distribute_objects()
{
	/* TODO: Once proper object distribution is implemented rewrite this function.
	 * limits_up and limits_down are from the old implementation. */
	unsigned int *limits_up = calloc(option->avail_cores, sizeof(unsigned int));
	unsigned int *limits_down = calloc(option->avail_cores, sizeof(unsigned int));
	
	int totcore = (int)((float)option->obj/option->avail_cores);
	for(int k = 1; k < option->avail_cores + 1; k++) {
		limits_down[k] = limits_up[k-1] + 1;
		limits_up[k] = limits_down[k] + totcore - 1;
		if(k == option->avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			limits_up[k] += option->obj - limits_up[k];
		}
		thread_opts[k].objcount = limits_up[k] - limits_down[k];
		thread_opts[k].indices = calloc(thread_opts[k].objcount, sizeof(unsigned int));
		
		for(int i = 0; i < thread_opts[k].objcount + 1; i++) {
			thread_opts[k].indices[i] = limits_down[k];
			limits_down[k]++;
		}
	}
	return 0;
}

int threadcontrol(int status, data** object)
{
	if(status == PHYS_UNPAUSE && running == 0) {
		for(int k = 1; k < option->avail_cores + 1; k++) thread_opts[k].dt = option->dt;
		running = 1;
		pthread_mutex_unlock(&movestop);
	} else if(status == PHYS_PAUSE && running == 1) {
		running = 0;
		pthread_mutex_lock(&movestop);
	} else if(status == PHYS_STATUS) {
		return running;
	} else if(status == PHYS_START) {
		distribute_objects();
		pthread_mutex_init(&movestop, NULL);
		pthread_barrier_init(&barrier, NULL, option->avail_cores);
		running = 1;
		for(int k = 1; k < option->avail_cores + 1; k++) {
			pprintf(PRI_ESSENTIAL, "Starting thread %i...", k);
			thread_opts[k].dt = option->dt;
			thread_opts[k].obj = *object;
			pthread_create(&threads[k], &thread_attribs, resolveforces, (void*)(long)k);
			pprintf(PRI_OK, "\n");
		}
	} else if(status == PHYS_SHUTDOWN) {
		/* Shutting down's always been unstable as we use a mutex for pausing. Current code works fine. */
		running = 1;
		pthread_mutex_unlock(&movestop);
		quit = 1;
		for(int k = 1; k < option->avail_cores + 1; k++) {
			pprintf(PRI_ESSENTIAL, "Shutting down thread %i...", k);
			pthread_join(threads[k], NULL);
			thread_opts[k].obj = NULL;
			pprintf(PRI_OK, "\n");
		}
		pthread_barrier_destroy(&barrier);
		pthread_mutex_destroy(&movestop);
		free(threads);
		running = 0;
		quit = 0;
	}
	return 0;
}

void *resolveforces(void *arg)
{
	struct thread_settings *thread = &thread_opts[(long)arg];
	v4sd vecnorm, accprev;
	double dist;
	const double pi = acos(-1);
	const long double gconst = option->gconst, epsno = option->epsno;
	const bool nogrv = option->nogrv, noele = option->noele, noflj = option->noflj;
	
	pthread_getcpuclockid(pthread_self(), &thread->clockid);
	
	while(!quit) {
		for(int i = 0; i < thread->objcount + 1; i++) {
			if(thread->obj[thread->indices[i]].ignore) continue;
			thread->obj[thread->indices[i]].pos += (thread->obj[thread->indices[i]].vel*thread->dt) +\
				(thread->obj[thread->indices[i]].acc)*((thread->dt*thread->dt)/2);
		}
		
		pthread_mutex_lock(&movestop);
		if(running) pthread_mutex_unlock(&movestop);
		pthread_barrier_wait(&barrier);
		
		for(int i = 0; i < thread->objcount + 1; i++) {
			accprev = thread->obj[thread->indices[i]].acc;
			for(int j = 1; j < option->obj + 1; j++) {
				if(thread->indices[i]==j) continue;
				vecnorm = thread->obj[j].pos - thread->obj[thread->indices[i]].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
				vecnorm /= dist;
				
				if(!nogrv)
					thread->obj[thread->indices[i]].acc += vecnorm*(double)(gconst*thread->obj[j].mass)/(dist*dist);
				if(!noele)
					thread->obj[thread->indices[i]].acc += -vecnorm*(double)((thread->obj[thread->indices[i]].charge*\
						thread->obj[j].charge)/(4*pi*epsno*dist*dist*thread->obj[thread->indices[i]].mass));
				if(!noflj)
					thread->obj[thread->indices[i]].acc += vecnorm*(4*epsilon*(12*(pow(sigma, 12)/pow(dist, 13)) -\
						6*(pow(sigma, 6)/pow(dist, 7)))/thread->obj[thread->indices[i]].mass);
			}
			thread->obj[thread->indices[i]].vel += (thread->obj[thread->indices[i]].acc + accprev)*((thread->dt)/2);
		}
		if((long)arg == 1) option->processed++;
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
