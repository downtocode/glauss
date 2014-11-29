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
#ifndef PHYSENGINE_AUX
#define PHYSENGINE_AUX

#include "physics.h"

extern struct atomic_cont {
	char *name;
	double mass;
	double charge;
	int number;
	float color[4];
} *atom_prop;

enum num_cmd {
	NUM_ANOTHER,
	NUM_GIVEME,
	NUM_REMOVE,
};

#define DIGITS_MAX 20

struct numbers_selection {
	int digits[DIGITS_MAX];
	unsigned short int final_digit;
};

/* Elements system. Submit a NULL filepath to use compiled internal DB */
unsigned short int return_atom_num(const char *name);
char *return_atom_str(unsigned int num);
void free_elements();

/* Shuffle through all algorithms available and change option->algorithm */
void phys_shuffle_algorithms(void);

/* Function to turn individual numbers into a single number. See graph_input
 * for example of how to use */
int getnumber(struct numbers_selection *numbers, int currentdigit,
			  enum num_cmd status);

/* Rotate vector by angles specified in rot1 */
void rotate_vec(vec3 *vec, vec3 *rot1);

/* Checks for shared coordinates in entire object array */
unsigned int phys_check_collisions(phys_obj *object,
								   unsigned int low, unsigned int high);


/* Counter, will reset the counter variable once it ticks over */
bool phys_timer_exec(unsigned int freq, unsigned int *counter);

/* Check coords for any collisions */
bool phys_check_coords(vec3 *vec, phys_obj *object,
					   unsigned int low, unsigned int high);

/* Return the CLOCK_MONOTONIC time in microseconds */
unsigned long long int phys_gettime_us(void);

/* Miliseconds sleep, uses nanosleep(will also this), will carry over a second */
int phys_sleep_msec(long unsigned int time);
#endif
