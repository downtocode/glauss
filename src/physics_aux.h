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
#ifndef PHYSENGINE_ELARR
#define PHYSENGINE_ELARR

extern struct atomic_cont {
	const char *name;
	double mass;
	double charge;
	float color[4];
} *atom_prop;

#define NUM_ANOTHER 1
#define NUM_GIVEME  2
#define NUM_REMOVE  3

struct numbers_selection {
	int digits[20];
	unsigned short int final_digit;
};

int init_elements(const char *filepath);
unsigned short int return_atom_num(const char *name);
int getnumber(struct numbers_selection *numbers, int currentdigit, unsigned int status);

#endif
