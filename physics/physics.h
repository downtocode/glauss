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
#ifndef PHYSENGINE_PHYS
#define PHYSENGINE_PHYS

#if (__clang_major__ >= 3) && (__clang_minor__ >= 5)
/* Use OpenCL's vectors when compiling with Clang
 *  since it doesn't support scalar operations on vectors. */
typedef double vec3 __attribute__((ext_vector_type(3)));
#elif (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 9)
typedef double vec3 __attribute__ ((vector_size (32)));
#else
#error Get a real compiler. GCC 4.9 or Clang 3.5 are decent.
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Status enum */
enum phys_status {
	PHYS_STATUS_RUNNING,
	PHYS_STATUS_STOPPED,
	PHYS_STATUS_PAUSED,
	PHYS_STATUS_ERROR,
	PHYS_STATUS_INIT,                   /* Somewhere in between initializaion */
	PHYS_CMD_NOT_FOUND,
	PHYS_INVALID_ALGORITHM,
};

enum phys_set_status {
	PHYS_STATUS,
	PHYS_PAUSESTART,
	PHYS_START,
	PHYS_SHUTDOWN,
};

/* Object structure - should be exactly 128 bytes */
typedef struct {
	vec3 pos, vel, acc;
	double mass, charge;
	long unsigned int id;
	float radius;
	bool ignore;
	uint8_t state, atomnumber;
} phys_obj;

/* Thread statistics structure */
struct thread_statistics {
	/* Shared across all algorithms */
	clockid_t clockid;
	
	/* Algorithm statistics */
	struct parser_map *thread_stats_map;
};

/* Global statistics structure */
struct global_statistics {
	unsigned int rng_seed, threads;
	long double progress, time_running, time_per_step;
	unsigned long long int total_steps;
	
	/* Algorithm statistics */
	struct parser_map *global_stats_map;
	
	struct thread_statistics *t_stats;
};

/* Struct sent to threads' init functions */
struct glob_thread_config {
	bool paused;
	volatile bool *quit;
	unsigned int algo_threads, total_syncd_threads;
	/* algo_threads is just the number of them at the time of start, excluding ctrl */
	pthread_t *threads, control_thread;
	pthread_mutex_t *pause;
	pthread_barrier_t *ctrl;
	pthread_spinlock_t *io_halt;
	struct global_statistics *stats;
	struct parser_map *algo_opt_map;
	phys_obj *obj;
	
	/* Thread-set variables */
	void **threads_conf; /* Returned from thread_config */
	void *returned_from_preinit; /* Returned from preinit */
	
	struct parser_map *algo_global_stats_map;     /* Preinit->ctrl_init */
	void *algo_global_stats_raw;                  /* Preinit->init */
	struct parser_map **algo_thread_stats_map;    /* Preinit->ctrl_init */
	void **algo_thread_stats_raw;                 /* Preinit->init */
	void (*thread_sched_fn)(struct glob_thread_config *);
	unsigned int thread_sched_fn_freq;
};

/*
 * Guidelines:
 * thread_preconfiguration = make adjustments to cfg before it's send to
 * ctrl thread and initialize any options here
 * thread_configuration = gets cfg and returns a void**(an array of void*),
 * which are the arguments of each thread
 * thread_function = receives a void* from config, run by pthread
 * thread_destruction = called upon stopping
 */

/* Algorithm structure */
typedef const struct list_algorithms {
	const char *name, *version, *desc, *author;
	void *(*thread_preconfiguration)(struct glob_thread_config *);
	void **(*thread_configuration)(struct glob_thread_config *);
	void *(*thread_location)(void *);
	void (*thread_destruction)(struct glob_thread_config *);
	void (*thread_sched_fn)(struct glob_thread_config *);
} phys_algorithm;

typedef void  *(*thread_preconfiguration)(struct glob_thread_config *);
typedef void **(*thread_configuration)(struct glob_thread_config *);
typedef void  *(*thread_function)(void *);
typedef void   (*thread_sched_fn)(struct glob_thread_config *);
typedef void   (*thread_destruction)(struct glob_thread_config *);

/* Returns struct */
phys_algorithm *phys_find_algorithm(const char *name);

/* Lock this to freeze everything */
extern pthread_spinlock_t *halt_objects;

/* Default thread attributes, inheritence and scheduling, init'd by phys_init() */
extern pthread_attr_t thread_attribs;

/* Statistics directly from the threads */
extern struct global_statistics *phys_stats;

/* List of algorithms and their function pointers */
extern const struct list_algorithms phys_algorithms[];

/* Used by algorithms to wait on ctrl thread */
void phys_ctrl_wait(pthread_barrier_t *barr);

/* Function to print all avail algorithms */
void phys_list_algo(void);

/* External functions for control */
int phys_init(phys_obj **object);
void phys_urgent_dump(void);
int phys_quit(phys_obj **object);
int phys_set_sched_mode(char **mode);
enum phys_status phys_ctrl(enum phys_set_status status, phys_obj **object);

#endif
