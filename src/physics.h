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

#define PHYS_PAUSE 0
#define PHYS_UNPAUSE 1
#define PHYS_STATUS 2
#define PHYS_START 8
#define PHYS_SHUTDOWN 9

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#if (__clang_major__ >= 3) &&  (__clang_minor__ >= 5)
/* Use OpenCL's vectors when compiling with Clang
 *  since it doesn't support scalar operations on vectors. */
typedef double v4sd __attribute__((ext_vector_type(3)));
/* Clang also defines __GNUC__ however it doesn't matter
 *  since the first condition has already been met in that case. */
#elif (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 9)
typedef double v4sd __attribute__ ((vector_size (32)));
#else
#error You need to update your compilers. GCC 4.9 or Clang 3.5 required
#endif

/* Object structure */
typedef struct {
	v4sd pos, vel, acc;
	double mass, charge;
	float radius;
	unsigned short int atomnumber, id;
	bool ignore;
} data;

/* Statistics structure */
struct thread_statistics {
	long double progress;
	clockid_t clockid;
	unsigned int bh_allocated;
	unsigned int bh_cleaned;
};

/* Algorithm structure */
struct list_algorithms {
	const char *name;
	void* (*thread_location)(void *thread_setts);
	void** (*thread_configuration)(data **, struct thread_statistics **);
};

extern struct thread_statistics **t_stats;
extern const struct list_algorithms phys_algorithms[];

typedef void* (*thread_function)(void*);
typedef void** (*thread_configuration)(data **, struct thread_statistics **);

/* These functions will return a function pointer */
thread_function phys_find_algorithm(const char *name);
thread_configuration phys_find_config(const char *name);

/* Thread controls */
pthread_mutex_t movestop;
pthread_barrier_t barrier;
bool running, quit;

/* External functions for control */
int initphys(data** object);
bool phys_remove_obj(data *object, unsigned int index);
int threadcontrol(int status, data** object);

#endif
