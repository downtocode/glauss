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
#include <stdbool.h>

struct option_struct* option;

/* Options struct */
/* //-- marks internal, non-parser settable variables */

struct option_struct {
	/* Frontend */
	unsigned short verbosity;
	int width, height;
	bool nogrv, noele, noflj, logenable;
	char *fontname, *filename, *algorithm;
	FILE *logfile; //--
	
	/* Physics */
	float dt;
	unsigned int obj; //--
	unsigned short avail_cores;
	double elcharge, gconst, epsno;
	
	/* Barnes-Hut algorithm specifics */
	float bh_ratio;
	unsigned short bh_lifetime;
	size_t bh_heapsize_max;
};
