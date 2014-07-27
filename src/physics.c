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
#include "physics_null.h"
#include "physics_n_body.h"
#include "physics_barnes_hut.h"

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2

/*	Indexing of cores = 1, 2, 3...	*/
static pthread_t *threads;
static pthread_attr_t thread_attribs;
static struct sched_param parameters;

/* Internal status */
bool running, quit;

/* Thread statistics */
struct thread_statistics **t_stats;

/* Only sent to quit function */
void** thread_conf;

const struct list_algorithms phys_algorithms[] = {
/*    name,           thread f-n,          init f-n      deinit f-n      */
	{ "null",         thread_null,         null_init,    null_quit    },
	{ "n-body",       thread_nbody,        nbody_init,   nbody_quit   },
	{ "barnes-hut",   thread_barnes_hut,   bhut_init,    bhut_quit    },
	{0}
};
/* Init function needs to return a double pointer, which then gets
 * distributed amongst threads as arguments. */

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

thread_destruction phys_find_quit(const char *name)
{
	for(const struct list_algorithms *i = phys_algorithms; i->name; i++) {
		if(!strcmp(i->name, name))
			return i->thread_destruction;
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
	unsigned short online_cores = 0;
	
#ifdef __linux__
	online_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	
	if(!option->threads) {
		if(online_cores) {
			option->threads = online_cores;
			pprintf(PRI_OK, "Detected %i threads, will use all.\n", online_cores);
		} else {
			option->threads = failsafe_cores;
			pprintf(PRI_WARN,
					"Core detection unavailable, running with %i thread(s).\n",
					failsafe_cores);
		}
	} else {
		if(online_cores) {
			pprintf(PRI_VERYHIGH, "Running with %i thread(s), out of %i cores.\n",
					option->threads, online_cores);
		} else {
			pprintf(PRI_VERYHIGH, "Runninh with %i threads.\n", option->threads);
		}
	}
	
	if(option->threads > option->obj) {
		pprintf(PRI_WARN, "More threads than objects. Capping threads to %i\n",
				option->obj+1);
		option->threads = option->obj+1;
	}
	
	/* Stats */
	t_stats = calloc(option->threads+1, sizeof(struct thread_statistics*));
	for(int k = 1; k < option->threads + 1; k++) {
		t_stats[k] = calloc(1, sizeof(struct thread_statistics));
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
	object = realloc(object, (--option->obj)*sizeof(data));
	if(!object) return 1;
	return 0;
}

bool phys_add_obj(data *objects, data *object) {
	objects = realloc(objects, (++option->obj)*sizeof(data));
	objects[option->obj] = *object;
	if(!objects) return 1;
	return 0;
}

int threadcontrol(int status, data** object)
{
	if(!option->threads) return 0;
	switch(status) {
		case PHYS_STATUS:
			return running;
			break;
		case PHYS_START:
			if(running) return 1;
			running = 1;
			threads = calloc(option->threads+1, sizeof(pthread_t));
			
			thread_conf = (phys_find_config(option->algorithm))(object, t_stats);
			
			for(int k = 1; k < option->threads + 1; k++) {
				pprintf(PRI_ESSENTIAL, "Starting thread %i...", k);
				pthread_create(&threads[k], &thread_attribs,
							   phys_find_algorithm(option->algorithm),
							   thread_conf[k]);
				pthread_getcpuclockid(threads[k], &t_stats[k]->clockid);
				pprintf(PRI_OK, "\n");
			}
			break;
		case PHYS_SHUTDOWN:
			if(!running) return 1;
			quit = 1;
			for(int k = 1; k < option->threads + 1; k++) {
				pprintf(PRI_ESSENTIAL, "Shutting down thread %i...", k);
				pthread_join(threads[k], NULL);
				pprintf(PRI_OK, "\n");
			}
			(phys_find_quit(option->algorithm))(thread_conf);
			free(threads);
			running = 0;
			quit = 0;
			break;
	}
	return 0;
}
