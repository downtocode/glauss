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
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "config.h"
#include "physics.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "input/sighandle.h"
#include "physics_ctrl.h"
#include "physics_null.h"
#include "physics_n_body.h"
#include "physics_barnes_hut.h"

/* "none" algorithm description */
#define PHYS_NONE {\
		.name = "none",\
		.version = PACKAGE_VERSION,\
		.desc = NULL,\
		.author = NULL,\
		.thread_configuration = NULL,\
		.thread_location = (thread_function)1,\
		.thread_destruction = NULL,\
	}

/*	Default threads to use when system != linux.	*/
#define failsafe_cores 2

/*	Indexing of cores = 1, 2, 3...	*/
static pthread_t *threads, control_thread;
static pthread_attr_t thread_attribs;
static struct sched_param parameters;
static unsigned int old_threads = 0;

/* Thread statistics and primary struct */
struct global_statistics *phys_stats;
struct glob_thread_config *cfg;

/* Only sent to deinit function */
void **thread_conf;

/* Populate structure with names and function pointers */
phys_algorithm phys_algorithms[] = {
	PHYS_NONE,
	PHYS_NULL,
	PHYS_NBODY,
	PHYS_BHUT,
	{0}
};

phys_algorithm *phys_find_algorithm(const char *name)
{
	for(phys_algorithm *i = phys_algorithms; i->name; i++) {
		if(!strcmp(i->name, name)) {
			return i;
		}
	}
	return NULL;
}

void phys_list_algo()
{
	pprintf(PRI_ESSENTIAL, "Implemented algorithms:\n");
	pprintf(PRI_ESSENTIAL, "    name		version			description\n");
	for(int n = 0; phys_algorithms[n].name; n++) {
		pprintf(PRI_ESSENTIAL, "    %s		%s		%s\n",
			   phys_algorithms[n].name,
			   phys_algorithms[n].version,
			   phys_algorithms[n].desc);
	}
}

int phys_init(data** object)
{
	/* Check if physics algorithm is valid */
	if(phys_find_algorithm(option->algorithm) == NULL) {
		pprintf(PRI_ERR,
				"Algorithm \"%s\" not found!\n",
				option->algorithm);
		phys_list_algo();
		exit(1);
	}
	
	/* Allocate memory for all the objects */
	*object = calloc(option->obj, sizeof(data));
	
	if(*object) {
		pprintf(PRI_OK, "Allocated %lu bytes(%u objects) to object array at %p.\n",
				option->obj*sizeof(data), option->obj, *object);
	} else {
		return 3; /* Impossible, should never ever happen. */
	}
	
	/* Seed RNG even though it's only used when option->bh_random_assign */
	srand(time(NULL));
	
	/* Set the amount of threads */
	unsigned short online_cores = 0;
	
#ifdef __linux__
	online_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	
	/* Threads */
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
	
	phys_stats = calloc(1, sizeof(struct global_statistics));
	add_to_free_queue(phys_stats);
	
	phys_stats_init();
	
	/* pthreads configuration */
	parameters.sched_priority = 50;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	SCHED_RR - Round Robin, SCHED_FIFO - FIFO	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	return 0;
}

int phys_stats_init()
{
	unsigned short int threads = option->threads;
	if(old_threads == 0) {
		phys_stats->t_stats = calloc(threads+1, sizeof(struct thread_statistics *));
		for(int k = 1; k < threads + 1; k++) {
			phys_stats->t_stats[k] = calloc(1, sizeof(struct thread_statistics));
		}
	} else if(old_threads != threads) {
		for(int k = 1; k < old_threads + 1; k++) {
			free(phys_stats->t_stats[k]);
		}
		free(phys_stats->t_stats);
		phys_stats->t_stats = calloc(threads+1, sizeof(struct thread_statistics *));
		for(int k = 1; k < threads + 1; k++) {
			phys_stats->t_stats[k] = calloc(1, sizeof(struct thread_statistics));
		}
	}
	old_threads = threads;
	return 0;
}

int phys_ctrl(int status, data** object)
{
	if(!option->threads) return 0;
	phys_algorithm *algo = phys_find_algorithm(option->algorithm);
	if(!algo) {
		pprintf(PRI_ERR, "Algorithm %s not found!\n", option->algorithm);
		return 1;
	}
	/* It's how we define a NULL function */
	if(algo->thread_location && !algo->thread_destruction) return 1;
	if(algo->thread_location && !algo->thread_configuration) return 1;
	switch(status) {
		case PHYS_STATUS:
			return option->status;
			break;
		case PHYS_PAUSESTART:
			option->paused = !option->paused;
			break;
		case PHYS_START:
			if(option->status) return 1;
			
			/* Reinit stats */
			phys_stats_init();
			
			threads = calloc(option->threads+1, sizeof(pthread_t));
			
			/* Create configuration */
			cfg = calloc(1, sizeof(struct glob_thread_config));
			cfg->stats = phys_stats;
			cfg->obj = *object;
			thread_conf = algo->thread_configuration(ctrl_init(cfg));
			
			/* Start threads */
			pprintf(PRI_ESSENTIAL, "Starting threads...");
			option->status = false;
			option->paused = true;
			for(int k = 1; k < option->threads + 1; k++) {
				if(pthread_create(&threads[k], &thread_attribs, algo->thread_location, thread_conf[k])) {
					pprintf(PRI_ERR, "Creating thread %i failed!\n", k);
					return 1;
				} else {
					pthread_getcpuclockid(threads[k], &phys_stats->t_stats[k]->clockid);
					pprintf(PRI_ESSENTIAL, "%i...", k);
				}
			}
			if(pthread_create(&control_thread, &thread_attribs, thread_ctrl, cfg)) {
				pprintf(PRI_ERR, "Creating control thread failed!\n");
				return 1;
			} else {
				pprintf(PRI_ESSENTIAL, "C...");
			}
			option->paused = false;
			option->status = true;
			pprintf(PRI_OK, "\n");
			/* Start threads */
			
			break;
		case PHYS_SHUTDOWN:
			if(!option->status) return 1;
			if(option->paused) {
				option->paused = false;
				sleep(1);
			}
			
			/* Stop threads */
			pprintf(PRI_ESSENTIAL, "Stopping threads...");
			pthread_cancel(control_thread);
			for(int k = 1; k < option->threads + 1; k++) {
				pthread_cancel(threads[k]);
			}
			void *res = NULL;
			for(int k = 1; k < option->threads + 1; k++) {
				pthread_join(threads[k], &res);
				if(res != PTHREAD_CANCELED) {
					pprintf(PRI_ERR, "Error joining with thread %i!\n", k);
					return 1;
				} else {
					pprintf(PRI_ESSENTIAL, "%i...", k);
				}
			}
			pthread_join(control_thread, &res);
			if(res != PTHREAD_CANCELED) {
				pprintf(PRI_ERR, "Error joining with control thread!\n");
				return 1;
			} else {
				pprintf(PRI_ESSENTIAL, "C...");
			}
			option->status = false;
			pprintf(PRI_OK, "\n");
			/* Stop threads */
			
			algo->thread_destruction(thread_conf);
			
			ctrl_quit(cfg);
			free(cfg);
			free(threads);
			break;
	}
	return 0;
}
