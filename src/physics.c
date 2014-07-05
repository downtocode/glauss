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
#include <string.h>
#include "physics.h"
#include "options.h"
#include "msg_phys.h"
#include "physics_n_body.h"
#include "physics_barnes_hut.h"

/* Thread controls */
pthread_mutex_t movestop;
pthread_barrier_t barrier;
bool running, quit;

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2

/*	Indexing of cores = 1, 2, 3...	*/
static pthread_t *threads;
static pthread_attr_t thread_attribs;
static pthread_mutexattr_t mattr;
static struct sched_param parameters;

struct thread_statistics **t_stats;

const struct list_algorithms phys_algorithms[] = {
	{"n-body",		thread_nbody,		nbody_init},
	{"barnes-hut",	thread_barnes_hut,	bhut_init},
	{0}
};

thread_function phys_find_algorithm(const char *name)
{
	for(const struct list_algorithms *i = phys_algorithms; i->name; i++) {
		if(!strcmp(i->name, name))
			return i->thread_location;
	}
	return NULL;
}

thread_configuration phys_find_config(const char *name)
{
	for(const struct list_algorithms *i = phys_algorithms; i->name; i++) {
		if(!strcmp(i->name, name))
			return i->thread_configuration;
	}
	return NULL;
}

int initphys(data** object)
{
	/* Check if physics algorithm is valid */
	if(phys_find_algorithm(option->algorithm) == NULL) {
		pprintf(PRI_ERR,
				"Algorithm \"%s\" not found! Implemented algorithms:\n",
				option->algorithm);
		for(int n = 0; phys_algorithms[n].name; n++) {
			printf("    %s\n", phys_algorithms[n].name);
		}
		exit(1);
	}
	
	/* Allocate memory for all the objects */
	*object = calloc(option->obj+1,sizeof(data));
	
	if(*object != NULL)
		pprintf(PRI_OK,
				"Allocated %lu bytes(%u objects) to object array at %p.\n", \
				(option->obj+1)*sizeof(data), option->obj+1, *object);
	else return 1;
	
	/* Set the amount of threads */
	int online_cores = 0;
	
	#ifdef __linux__
		online_cores = sysconf(_SC_NPROCESSORS_ONLN);
	#endif
	
	if(option->avail_cores == 0 && online_cores != 0 ) {
		option->avail_cores = online_cores;
		pprintf(PRI_OK, "Detected %i threads, will use all.\n", online_cores);
	} else if( option->avail_cores != 0 && online_cores != 0\
										 && online_cores > option->avail_cores )
		pprintf(PRI_VERYHIGH, "Using %i out of %i threads.\n",
				option->avail_cores, online_cores);
	else if(option->avail_cores == 1)
		pprintf(PRI_VERYHIGH, "Running program in a single thread.\n");
	else if(option->avail_cores > 1)
		pprintf(PRI_VERYHIGH, "Running program with %i threads.\n",
				option->avail_cores);
	else if(option->avail_cores == 0 ) {
		/*	Poor OSX...	*/
		option->avail_cores = failsafe_cores;
		pprintf(PRI_WARN,
				"Thread detection unavailable, running with %i thread(s).\n",
				failsafe_cores);
	}
	
	/* pthreads configuration */
	parameters.sched_priority = 50;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	SCHED_RR - Round Robin, SCHED_FIFO - FIFO	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	return 0;
}

bool phys_remove_obj(data *object, unsigned int index) {
	object[index] = object[option->obj];
	object = realloc(object, (option->obj--)*sizeof(data));
	if(!object) return 1;
	return 0;
}

int threadcontrol(int status, data** object)
{
	if(!option->avail_cores) return 0;
	switch(status) {
		case PHYS_UNPAUSE:
			return 0;
			/*if(running) return 1;
			running = 1;
			pthread_mutex_unlock(&movestop);*/
			break;
		case PHYS_PAUSE:
			return 0;
			// Pausing temporarily disabled until a better way is found.
			/*if(!running) return 1;
			running = 0;
			pthread_mutex_lock(&movestop);*/
			break;
		case PHYS_STATUS:
			return running;
			break;
		case PHYS_START:
			if(running) return 1;
			running = 1;
			threads = calloc(option->avail_cores+1, sizeof(pthread_t));
			
			pthread_barrier_init(&barrier, NULL, option->avail_cores);
			//pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_PRIVATE);
			pthread_mutex_init(&movestop, &mattr);
			
			struct thread_statistics **stats = calloc(option->avail_cores+1,
									sizeof(struct thread_statistics*));
			for(int k = 1; k < option->avail_cores + 1; k++) {
				stats[k] = calloc(1, sizeof(struct thread_statistics));
			}
			
			void** thread_conf = 
						   (phys_find_config(option->algorithm))(object, stats);
			
			for(int k = 1; k < option->avail_cores + 1; k++) {
				pprintf(PRI_ESSENTIAL, "Starting thread %i...", k);
				pthread_create(&threads[k], &thread_attribs,
							   phys_find_algorithm(option->algorithm),
							   thread_conf[k]);
				pthread_getcpuclockid(threads[k], &stats[k]->clockid);
				pprintf(PRI_OK, "\n");
			}
			t_stats = stats;
			break;
		case PHYS_SHUTDOWN:
			if(!running) return 1;
			threadcontrol(PHYS_UNPAUSE, NULL);
			quit = 1;
			for(int k = 1; k < option->avail_cores + 1; k++) {
				pprintf(PRI_ESSENTIAL, "Shutting down thread %i...", k);
				pthread_join(threads[k], NULL);
				pprintf(PRI_OK, "\n");
			}
			
			pthread_barrier_destroy(&barrier);
			pthread_mutex_destroy(&movestop);
			
			free(threads);
			running = 0;
			quit = 0;
			break;
	}
	return 0;
}
