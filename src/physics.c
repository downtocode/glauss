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
	for(int i = 0; i < option->obj + 1; i++ ) {
		(*object)[i].links = calloc(option->obj+1,sizeof(bool));
	}
	
	pprintf(5, "[\033[032m OK! \033[0m] Allocated %lu bytes to object array at %p.\n", \
	((option->obj+1)*sizeof(data)+(option->obj+1)*sizeof(float)), *object);
	
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
		pprintf(PRI_VERYHIGH, "[\033[033m Warning! \033[0m] Thread detection unavailable, running with %i thread(s).\n", failsafe_cores);
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
		threadseperate();
		pthread_mutex_init(&movestop, NULL);
		pthread_barrier_init(&barrier, NULL, option->avail_cores);
		running = 1;
		for(int k = 1; k < option->avail_cores + 1; k++) {
			if(thread_opts[k].obj != NULL) continue;
			thread_opts[k].dt = option->dt;
			thread_opts[k].obj = *object;
			pthread_create(&threads[k], &thread_attribs, resolveforces, (void*)(long)k);
			pprintf(5, "[\033[032m OK! \033[0m] Thread %i - %p started using object array at %p.\n", \
			k, &threads[k], *object);
		}
	} else if(status == PHYS_SHUTDOWN) {
		/* Shutting down's always been unstable as we use a mutex for pausing. */
		running = 1;
		pthread_mutex_unlock(&movestop);
		quit = 1;
		for(int k = 1; k < option->avail_cores + 1; k++) {
			pprintf(5, "Thread %i - %p shutting down...", k, &threads[k]);
			pthread_join(threads[k], NULL);
			thread_opts[k].obj = NULL;
			pprintf(5, "\r[\033[032m OK! \033[0m] Thread %i - %p shut down.\n", k, &threads[k]);
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
	/*
	 * There shouldn't be any need to use a mutex for synchronizing access to the object array.
	 * Each thread reads every object(including other threads' objects) however it only writes
	 * to its own set of objects. Therefore synchronizing common objects reads, which is the
	 * only time threads overlap, is not required.
	 */
	
	struct thread_settings *thread = &thread_opts[(long)arg];
	v4sd vecnorm, accprev, grv, ele, flj;
	double dist;
	
	ele = grv = flj = (v4sd){0,0,0};
	
	long double pi = acos(-1);
	long double gconst = option->gconst, epsno = option->epsno;
	
	pthread_getcpuclockid(pthread_self(), &thread->clockid);
	
	while(!quit) {
		for(int i = thread->looplimit1; i < thread->looplimit2 + 1; i++) {
			if(thread->obj[i].ignore) continue;
			thread->obj[i].pos += (thread->obj[i].vel*thread->dt) + (thread->obj[i].acc)*((thread->dt*thread->dt)/2);
		}
		
		pthread_mutex_lock(&movestop);
		/* Mutex here is only used for pausing the execution. Using it as a barrier not possible.(?) */
		if(running) pthread_mutex_unlock(&movestop);
		pthread_barrier_wait(&barrier);
		
		for(int i = thread->looplimit1; i < thread->looplimit2 + 1; i++) {
			for(int j = 1; j < option->obj + 1; j++) {
				if(i==j) continue;
				vecnorm = thread->obj[j].pos - thread->obj[i].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
				vecnorm /= dist;
				
				//grvmag = ((gconst*thread->obj[i].mass*thread->obj[j].mass)/(dist*dist));
				ele += -vecnorm*(double)((thread->obj[i].charge*thread->obj[j].charge)/(4*pi*epsno*dist*dist));
				flj += vecnorm*(4*epsilon*(12*(pow(sigma, 12)/pow(dist, 13)) - 6*(pow(sigma, 6)/pow(dist, 7))));
				
			}
			accprev = thread->obj[i].acc;
			thread->obj[i].acc = (ele+flj)/thread->obj[i].mass;
			thread->obj[i].vel += (thread->obj[i].acc + accprev)*((thread->dt)/2);
			ele = flj = (v4sd){0,0,0};
		}
		if((long)arg == 1) option->processed++;
		/* Using SDL_Delay because it's thread-safe. */
		SDL_Delay(option->sleepfor);
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
