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
#ifndef PHYSENGINE_INPUT
#define PHYSENGINE_INPUT

#include "physics.h"

#define MOL_XYZ 1
#define MOL_PDB 2
#define MOL_OBJ 3

typedef struct {
	data *inf;
	char filename[100];
	double scale;
} in_file;

int in_probe_file(const char *filename);
int in_read_file(data *object, int *i, in_file file);

#endif
