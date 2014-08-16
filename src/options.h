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

/* Options struct */
/* "//--" denotes internal, non-parser settable variables */

extern struct option_struct {
	/* Frontend */
	unsigned short verbosity;
	int width, height, fontsize;
	bool nogrv, noele, logenable;
	char *fontname, *filename, *algorithm, *sshot_temp;
	FILE *logfile; //--
	
	/* Physics */
	bool status, paused; //--
	float dt;
	unsigned int obj; //--
	unsigned short threads;
	double elcharge, gconst, epsno;
	
	/* Barnes-Hut algorithm specifics */
	float bh_ratio;
	unsigned short bh_tree_limit;
	unsigned short bh_lifetime;
	size_t bh_heapsize_max;
	bool bh_single_assign;
	
	/* Physics - misc */
	unsigned int dump_xyz, dump_sshot;
	bool write_sshot_now; //--
} *option;
