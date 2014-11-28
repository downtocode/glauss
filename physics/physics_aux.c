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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <tgmath.h>
#include <sys/time.h>
#include "physics_aux.h"
#include "main/msg_phys.h"
#include "main/options.h"

struct atomic_cont *atom_prop;

unsigned short int return_atom_num(const char *name)
{
	if (!name || name[0] == '\0')
		return 0;
	for (int i=1; i<121; i++) {
		if (!strncasecmp(name, atom_prop[i].name, strlen(atom_prop[i].name))) {
			return i;
		}
	}
	return 0;
}

char *return_atom_str(unsigned int num)
{
	if (num > 120)
		return NULL;
	else
		return atom_prop[num].name;
}

void free_elements()
{
	for (int i=0; i<121; i++) {
		free(atom_prop[i].name);
	}
	free(atom_prop);
	atom_prop = NULL;
}

/* Function to input numbers in an array and later extract a single number out.
 * Used in selecting objects. */
int getnumber(struct numbers_selection *numbers, int currentdigit, int status)
{
	if (status == NUM_ANOTHER) {
		numbers->digits[numbers->final_digit] = currentdigit;
		numbers->final_digit+=1;
		return 0;
	} else if (status == NUM_GIVEME) {
		unsigned int result = 0;
		for(int i=0; i < numbers->final_digit; i++) {
			result*=10;
			result+=numbers->digits[i];
		}
		numbers->final_digit = 0;
		return result;
	} else if (status == NUM_REMOVE) {
		numbers->final_digit-=1;
		return 0;
	}
	return 0;
}

/* Change algorithms */
void phys_shuffle_algorithms(void)
{
	if (phys_ctrl(PHYS_STATUS, NULL) == PHYS_STATUS_RUNNING) {
		pprintf(PRI_WARN, "Physics needs to be stopped before changing modes.\n");
		return;
	}
	/* Shuffle algorithms */
	int num;
	/* Get number of algorithm */
	for (num = 0; phys_algorithms[num].name; num++) {
		if (!strcmp(option->algorithm, phys_algorithms[num].name))
			break;
	}
	/* Select next and check if we're on the last */
	if (!phys_algorithms[++num].name)
		num = 0;
	pprintf(PRI_HIGH, "Changing algorithm to \"%s\".\n",
			phys_algorithms[num].name);
	free(option->algorithm);
	option->algorithm = strdup(phys_algorithms[num].name);
}

unsigned int phys_check_collisions(data *object,
								   unsigned int low, unsigned int high)
{
	unsigned int collisions = 0;
	vec3 vecnorm = {0};
	double dist = 0.0;
	for (unsigned int i = low; i < high; i++) {
		if (!object[i].mass) {
			pprint_err("Object %i has no mass!\n", i);
			collisions++;
		}
		for (unsigned int j = low; j < high; j++) {
			if (i==j)
				continue;
			vecnorm = object[j].pos - object[i].pos;
			dist = sqrt(vecnorm[0]*vecnorm[0] +\
						vecnorm[1]*vecnorm[1] +\
						vecnorm[2]*vecnorm[2]);
			if (dist == 0.0) {
				collisions+=2;
				pprint_err("Objects %i and %i share coordinates!\n", i, j);
			}
		}
	}
	return collisions;
}

bool phys_check_coords(vec3 *vec, data *object,
					   unsigned int low, unsigned int high)
{
	vec3 vecnorm = {0};
	double dist = 0.0;
	for (unsigned int i = low; i < high; i++) {
		for (unsigned int j = low; j < high; j++) {
			vecnorm = object[j].pos - *vec;
			dist = sqrt(vecnorm[0]*vecnorm[0] +\
						vecnorm[1]*vecnorm[1] +\
						vecnorm[2]*vecnorm[2]);
			if (dist == 0.0) {
				return 1;
			}
		}
	}
	return 0;
}

/* We have to implement those here too as graphics may not be compiled */
void rotate_vec(vec3 *vec, vec3 *rot1)
{
	vec3 res = *vec, rot = *rot1;
	if (rot[0]) {
		double c = cos(rot[0]);
		double s = sin(rot[0]);
		res[1] = c*(*vec)[1] - s*(*vec)[2];
		res[2] = s*(*vec)[1] + c*(*vec)[2];
	}
	if (rot[1]) {
		vec3 tmp = res;
		double c = cos(rot[1]);
		double s = sin(rot[1]);
		res[0] = c*tmp[0] + s*tmp[2];
		res[2] = c*tmp[2] - s*tmp[0];
	}
	if (rot[2]) {
		vec3 tmp = res;
		double c = cos(rot[2]);
		double s = sin(rot[2]);
		res[0] = c*tmp[0] - s*tmp[1];
		res[1] = s*tmp[0] + c*tmp[1];
	}
	*vec = res;
}

bool phys_timer_exec(unsigned int freq, unsigned int *counter)
{
	if (!freq) {
		return false;
	} else if (freq <= ++(*counter)) {
		*counter = 0;
		return true;
	} else {
		return false;
	}
}

unsigned long long int phys_gettime_us(void)
{
	#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0 && defined(CLOCK_MONOTONIC)
		struct timespec ts;
	#if defined(CLOCK_MONOTONIC_RAW)
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
	#else
		if (clock_gettime(CLOCK_MONOTONIC, &ts))
	#endif
			abort();
		return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
	#else
		struct timeval tv;
		gettimeofday(&tv,NULL);
		return tv.tv_sec * 1000000LL + tv.tv_usec;
	#endif
}

int phys_sleep_msec(long unsigned int time)
{
	struct timespec dur = {0};
	
	/* nanosleep can't tolerate ts_nsec larger than 1 second */
	while (time*1000000 > 999999999) {
		dur.tv_sec += 1;
		time -= 1000;
	}
	
	dur.tv_nsec = time*1000000;
	
	return nanosleep(&dur, NULL);
}
