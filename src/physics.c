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

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2

/*	Indexing of cores = 1, 2, 3...	*/
static pthread_t *threads;
static pthread_attr_t thread_attribs;
static struct sched_param parameters;

int initphys(data** object)
{
	/* Check if physics algorithm is valid */
	if(phys_find_algorithm(option->algorithm) == NULL) {
		pprintf(PRI_ERR, "Algorithm \"%s\" not found! Implemented algorithms:\n", option->algorithm);
		for(int n = 0; phys_algorithms[n].name; n++) {
			printf("    %s\n", phys_algorithms[n].name);
		}
		exit(1);
	}
	
	/* Calloc the object struct */
	*object = calloc(option->obj+1,sizeof(data));
	
	if(*object != NULL) pprintf(PRI_OK, "Allocated %lu bytes(%u objects) to object array at %p.\n", \
	(option->obj+1)*sizeof(data), option->obj+1, *object);
	
	/* Set the amount of threads */
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
	
	/* pthreads configuration */
	parameters.sched_priority = 50;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	SCHED_RR - Round Robin, SCHED_FIFO - FIFO	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	return 0;
}

int threadcontrol(int status, data** object)
{
	if(!option->avail_cores) return 0;
	switch(status) {
		case PHYS_UNPAUSE:
			if(running) return 1;
			running = 1;
			pthread_mutex_unlock(&movestop);
			break;
		case PHYS_PAUSE:
			if(!running) return 1;
			running = 0;
			pthread_mutex_lock(&movestop);
		case PHYS_STATUS:
			return running;
			break;
		case PHYS_START:
			threads = calloc(option->avail_cores+1, sizeof(pthread_t));
			
			if(!strcmp(option->algorithm, "n-body")) {
				thread_opts_nbody = calloc(option->avail_cores+1, sizeof(struct thread_config_nbody));
				distribute_nbody(thread_opts_nbody);
			} else if(!strcmp(option->algorithm, "barnes-hut")) {
				struct phys_barnes_hut_octree *octree = calloc(1, sizeof(struct phys_barnes_hut_octree));
				octree->depth = 0;
				octree->data = NULL;
				octree->leaf = 1;
				for(int i=0; i < 8; i++) octree->cells[i] = NULL;
				octree->origin = (v4sd){0,0,0};
				octree->halfdim = max_disp_from_origin(*object);
				build_octree(*object, octree);
			}
			
			pthread_mutex_init(&movestop, NULL);
			pthread_barrier_init(&barrier, NULL, option->avail_cores);
			for(int k = 1; k < option->avail_cores + 1; k++) {
				pprintf(PRI_ESSENTIAL, "Starting thread %i...", k);
				thread_opts_nbody[k].obj = *object;
				thread_opts_nbody[k].id = k;
				pthread_create(&threads[k], &thread_attribs, thread_nbody, &thread_opts_nbody[k]);
				pthread_getcpuclockid(threads[k], &thread_opts_nbody[k].clockid);
				pprintf(PRI_OK, "\n");
			}
			running = 1;
			break;
		case PHYS_SHUTDOWN:
			/* Shutting down's always been unstable as we use a mutex for pausing. Current code works fine. */
			running = 1;
			pthread_mutex_unlock(&movestop);
			quit = 1;
			for(int k = 1; k < option->avail_cores + 1; k++) {
				pprintf(PRI_ESSENTIAL, "Shutting down thread %i...", k);
				pthread_join(threads[k], NULL);
				thread_opts_nbody[k].obj = NULL;
				pprintf(PRI_OK, "\n");
			}
			pthread_barrier_destroy(&barrier);
			pthread_mutex_destroy(&movestop);
			
			if(!strcmp(option->algorithm, "n-body")) {
				free(thread_opts_nbody->indices);
				free(thread_opts_nbody);
			}
			
			free(threads);
			running = 0;
			quit = 0;
			break;
	}
	return 0;
}

const struct list_algorithms phys_algorithms[] = {
	{"n-body",		(void*)thread_nbody,	NULL},
	{"barnes-hut",	(void*)thread_barnes_hut,	NULL},
	{0}
};

void *phys_find_algorithm(const char *name)
{
	for(const struct list_algorithms *i = phys_algorithms; i->name; i++) {
		if(!strcmp(i->name, name))
			return i->thread_location;
	}
	return NULL;
}

void *phys_find_config(const char *name)
{
	for(const struct list_algorithms *i = phys_algorithms; i->name; i++) {
		if(!strcmp(i->name, name))
			return i->thread_config;
	}
	return NULL;
}
