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
#ifndef PHYSENGINE_PHYS
#define PHYSENGINE_PHYS

#define PHYS_PAUSE 0
#define PHYS_UNPAUSE 1
#define PHYS_STATUS 2
#define PHYS_START 8
#define PHYS_SHUTDOWN 9

#include <stdbool.h>
#include <time.h>

#if (__clang_major__ >= 3) &&  (__clang_minor__ >= 5)
/* 
 * Use OpenCL's vectors when compiling with Clang since it doesn't support scalar operations on vectors.
 */
typedef double v4sd __attribute__((ext_vector_type(3)));
/*	Clang also defines __GNUC__ however it doesn't matter since the first condition has already been met in that case.	*/
#elif (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 8)
/*
 * It appears I might have been wrong about GCC. It tries its damn hardest to optimize
 * the hell out of anything, but without AVX it can't do much.
 * My Celeron B820 doesn't support AVX, so I'll have to check out whether it does well
 * with 32 bit vectors on a high end machine(read: not junk).
 */
typedef double v4sd __attribute__ ((vector_size (32)));
#else
#error You need to update your compilers. Needs at least gcc 4.8.2 or clang 3.5 to compile.
#endif

typedef struct {
	v4sd pos, vel, acc;
	double mass, charge;
	float radius;
	unsigned short int atomnumber;
	bool ignore;
	unsigned int totlinks;
	bool *links;
} data;

struct thread_settings {
	data* obj;
	float dt;
	unsigned int looplimit1, looplimit2, threadid;
	clockid_t clockid;
};

struct thread_settings *thread_opts;

int initphys(data** object);
int threadseperate();
int threadcontrol(int status, data** object);
void *resolveforces(void *arg);

#endif
