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

#define DIGITS_MAX 20

#include "physics.h"

extern struct atomic_cont {
	const char *name;
	double mass;
	double charge;
	float color[4];
} *atom_prop;

enum {
	NUM_ANOTHER,
	NUM_GIVEME,
	NUM_REMOVE,
};

struct numbers_selection {
	int digits[DIGITS_MAX];
	unsigned short int final_digit;
};

/* Elements system. Submit a NULL filepath to use compiled internal DB */
int init_elements(const char *filepath);
unsigned short int return_atom_num(const char *name);
const char *return_atom_str(unsigned int num);

/* Shuffle through all algorithms available and change option->algorithm */
void phys_shuffle_algorithms();

/* Function to turn individual numbers into a single number. See graph_input
 * for example of how to use */
int getnumber(struct numbers_selection *numbers, int currentdigit, int status);

/* Rotate vector by angles specified in rot1 */
void rotate_vec(vec3 *vec, vec3 *rot1);

#endif
