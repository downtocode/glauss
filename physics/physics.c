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
#include "physics_aux.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "input/sighandle.h"
#include "physics_ctrl.h"
#include "physics_null.h"
#include "physics_n_body.h"
#include "physics_barnes_hut.h"

/* "none" algorithm description */
#define PHYS_NONE {                               \
		.name = "none",                           \
		.version = PACKAGE_VERSION,               \
		.desc = "Starts absolutely no threads",   \
		.author = NULL,                           \
		.thread_configuration = NULL,             \
		.thread_location = (thread_function)1,    \
		.thread_destruction = NULL,               \
	}

/*	Default threads to use when system != linux.	*/
#define AUTO_UNAVAIL_THREADS 1

/*	Number of overallocated buffer objects(for just in case) */
#define OVERALLOC_OBJ 1

/*	Indexing of cores = 1, 2, 3...	*/
pthread_mutex_t *halt_objects = NULL;
pthread_attr_t thread_attribs;
static struct sched_param parameters;

/* Thread statistics and primary struct */
struct global_statistics *phys_stats;
struct glob_thread_config *cfg;

/* Populate structure with names and function pointers */
phys_algorithm phys_algorithms[] = {
	PHYS_NONE,
	PHYS_NULL,
	PHYS_NULL_STATS,
	PHYS_NBODY,
	PHYS_BARNES_HUT,
	{0}
};

phys_algorithm *phys_find_algorithm(const char *name)
{
	for (phys_algorithm *i = phys_algorithms; i->name; i++) {
		if (!strcmp(i->name, name)) {
			return i;
		}
	}
	return NULL;
}

void phys_list_algo(void)
{
	pprintf(PRI_ESSENTIAL, "Implemented algorithms:\n");
	pprintf(PRI_ESSENTIAL, "    name		version			description\n");
	for (int n = 0; phys_algorithms[n].name; n++) {
		pprintf(PRI_ESSENTIAL, "%i.  %s		%s		%s\n", n,
			   phys_algorithms[n].name,
			   phys_algorithms[n].version,
			   phys_algorithms[n].desc);
	}
}

int phys_init(data** object)
{
	/* Check if physics algorithm is valid */
	if (phys_find_algorithm(option->algorithm) == NULL) {
		pprintf(PRI_ERR,
				"Algorithm \"%s\" not found!\n",
				option->algorithm);
		phys_list_algo();
		exit(1);
	}
	
	/* Allocate memory for all the objects */
	*object = calloc(option->obj+OVERALLOC_OBJ, sizeof(data));
	
	if (*object) {
		pprintf(PRI_OK, "Allocated %lu bytes(%u objects) to object array at %p.\n",
				(option->obj+OVERALLOC_OBJ)*sizeof(data), option->obj, *object);
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
	if (!option->threads) {
		if (online_cores) {
			option->threads = online_cores;
			pprintf(PRI_OK, "Detected %i threads, will use all.\n", online_cores);
		} else {
			option->threads = AUTO_UNAVAIL_THREADS;
			pprintf(PRI_WARN,
					"Core detection unavailable, running with %i thread(s).\n",
					AUTO_UNAVAIL_THREADS);
		}
	} else {
		if (online_cores) {
			pprintf(PRI_VERYHIGH, "Running with %i thread(s), out of %i cores.\n",
					option->threads, online_cores);
		} else {
			pprintf(PRI_VERYHIGH, "Runninh with %i threads.\n", option->threads);
		}
	}
	
	if (option->threads > option->obj) {
		pprintf(PRI_WARN, "More threads than objects. Capping threads to %i\n",
				option->obj+1);
		option->threads = option->obj+1;
	}
	
	phys_stats = calloc(1, sizeof(struct global_statistics));
	phys_stats->t_stats = calloc(option->threads+1, sizeof(struct thread_statistics));
	
	/* pthreads configuration */
	parameters.sched_priority = 50;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setinheritsched(&thread_attribs, PTHREAD_INHERIT_SCHED);
	/*	SCHED_RR - Round Robin, SCHED_FIFO - FIFO	*/
	pthread_attr_setschedpolicy(&thread_attribs, SCHED_RR);
	pthread_attr_setschedparam(&thread_attribs, &parameters);
	return 0;
}

int phys_ctrl(int status, data** object)
{
	if (!option->threads)
		return 0;
	phys_algorithm *algo = phys_find_algorithm(option->algorithm);
	if (!algo) {
		pprintf(PRI_ERR, "Algorithm %s not found!\n", option->algorithm);
		return 1;
	}
	/* It's how we define the "null" algorithm */
	if (algo->thread_location && !algo->thread_destruction)
		return 1;
	if (algo->thread_location && !algo->thread_configuration)
		return 1;
	int retval = PHYS_CMD_NOT_FOUND;
	switch(status) {
		case PHYS_STATUS:
			if (!cfg) {
				retval = PHYS_STATUS_STOPPED;
			} else if (!cfg->pause) {
				retval = PHYS_STATUS_INIT;
			} else {
				retval = *cfg->pause ? PHYS_STATUS_PAUSED : PHYS_STATUS_RUNNING;
			}
			break;
		case PHYS_PAUSESTART:
			if (!cfg) {
				retval = PHYS_STATUS_STOPPED;
				break;
			}
			*cfg->pause = !*cfg->pause;
			retval = *cfg->pause ? PHYS_STATUS_RUNNING : PHYS_STATUS_PAUSED;
			break;
		case PHYS_START:
			if (cfg) {
				retval = PHYS_STATUS_RUNNING;
				break;
			}
			
			/* Init RNG */
			phys_stats->rng_seed = option->rng_seed ? option->rng_seed : phys_gettime_us();
			srand(phys_stats->rng_seed);
			
			/* Reinit stats */
			phys_stats->t_stats = realloc(phys_stats->t_stats, (option->threads+1)*sizeof(struct thread_statistics));
			
			/* Create configuration */
			cfg = calloc(1, sizeof(struct glob_thread_config));
			cfg->stats = phys_stats;
			cfg->obj = *object;
			cfg->total_syncd_threads = option->threads+1;
			cfg->thread_sched_fn = algo->thread_sched_fn;
			if (algo->thread_preconfiguration) {
				algo->thread_preconfiguration(cfg);
			}
			cfg = ctrl_init(cfg);
			halt_objects = cfg->io_halt;
			cfg->threads_conf = algo->thread_configuration(cfg);
			
			/* Check for errors */
			if (!cfg->threads_conf) {
				pprint_err("Algorithm's config f-n returned NULL, failure.\n");
				ctrl_quit(cfg);
				return 1;
			}
			
			/* Start threads */
			pprintf(PRI_ESSENTIAL, "Starting threads...");
			*cfg->pause = true;
			for (int k = 1; k < option->threads + 1; k++) {
				if (pthread_create(&cfg->threads[k], &thread_attribs, algo->thread_location, cfg->threads_conf[k])) {
					pprintf(PRI_ERR, "Creating thread %i failed!\n", k);
					return 1;
				} else {
					pthread_getcpuclockid(cfg->threads[k], &phys_stats->t_stats[k].clockid);
					pprintf(PRI_ESSENTIAL, "%i...", k);
				}
			}
			if (pthread_create(&cfg->control_thread, &thread_attribs, thread_ctrl, cfg)) {
				pprintf(PRI_ERR, "Creating control thread failed!\n");
				return 1;
			} else {
				pprintf(PRI_ESSENTIAL, "C...");
			}
			*cfg->pause = false;
			pprintf(PRI_OK, "\n");
			/* Start threads */
			
			retval = PHYS_STATUS_RUNNING;
			
			break;
		case PHYS_SHUTDOWN:
			if (!cfg) {
				retval = PHYS_STATUS_STOPPED;
				break;
			}
			
			/* Stop threads */
			*cfg->pause = false;
			*cfg->quit = true;
			pprintf(PRI_ESSENTIAL, "Stopping threads...");
			void *res = NULL;
			for (int k = 1; k < option->threads + 1; k++) {
				pthread_join(cfg->threads[k], &res);
				pprintf(PRI_ESSENTIAL, "%i...", k);
			}
			pthread_join(cfg->control_thread, &res);
			pprintf(PRI_ESSENTIAL, "C...");
			pprintf(PRI_OK, "\n");
			/* Stop threads */
			
			algo->thread_destruction(cfg->threads_conf);
			
			ctrl_quit(cfg);
			halt_objects = NULL;
			cfg = NULL;
			
			retval = PHYS_STATUS_STOPPED;
			
			break;
	}
	return retval;
}
