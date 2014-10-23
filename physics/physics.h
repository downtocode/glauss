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

#if (__clang_major__ >= 3) &&  (__clang_minor__ >= 5)
/* Use OpenCL's vectors when compiling with Clang
 *  since it doesn't support scalar operations on vectors. */
typedef double vec3 __attribute__((ext_vector_type(3)));
#elif (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 9)
typedef double vec3 __attribute__ ((vector_size (32)));
#else /* Compiler version */
#error You need to update your compilers. At least GCC 4.9 or Clang 3.5 required.
#endif

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

/* Status enum */
enum {
	PHYS_STATUS,
	PHYS_PAUSESTART,
	PHYS_START,
	PHYS_SHUTDOWN,
};

/* Object structure */
typedef struct {
	vec3 pos, vel, acc;
	double mass, charge;
	float radius;
	unsigned short int atomnumber, id;
	bool ignore;
} data;

/* Thread statistics structure */
struct thread_statistics {
	/* Shared across all algorithms */
	clockid_t clockid;
	
	/* physics_barnes_hut */
	unsigned int bh_total_alloc;
	unsigned int bh_new_alloc;
	unsigned int bh_new_cleaned;
	size_t bh_heapsize;
	
	/* physics_null */
	double null_avg_dist, null_max_dist;
};

/* Global statistics structure */
struct global_statistics {
	unsigned int rng_seed;
	long double progress, time_running;
	
	/* physics_barnes_hut */
	unsigned int bh_total_alloc;
	unsigned int bh_new_alloc;
	unsigned int bh_new_cleaned;
	size_t bh_heapsize;
	
	/* physics_null */
	double null_avg_dist, null_max_dist;
	
	struct thread_statistics **t_stats;
};

/* Struct sent to threads' init functions */
struct glob_thread_config {
	data *obj;
	struct global_statistics *stats;
	pthread_barrier_t *ctrl;
};

/* Algorithm structure */
typedef const struct list_algorithms {
	const char *name, *version, *desc, *author;
	void **(*thread_configuration)(struct glob_thread_config *cfg);
	void *(*thread_location)(void *thread_setts);
	void (*thread_destruction)(void **);
} phys_algorithm;

typedef void  *(*thread_function)(void*);
typedef void **(*thread_configuration)(struct glob_thread_config *cfg);
typedef void   (*thread_destruction)(void **);

/* Returns struct */
phys_algorithm *phys_find_algorithm(const char *name);

/* Statistics directly from the threads */
extern struct global_statistics *phys_stats;

/* List of algorithms and their function pointers */
extern const struct list_algorithms phys_algorithms[];

/* Function to print all avail algorithms */
void phys_list_algo();

/* External functions for control */
int phys_stats_init();
int phys_init(data** object);
int phys_ctrl(int status, data** object);

#endif
