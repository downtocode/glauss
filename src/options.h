/*
 * This file is part of physengine.
 * Copyright (c) 2012 Rostislav Pehlivanov
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
#include <stdbool.h>
#define revision "physengine"

struct option_struct* option;

struct option_struct {
	float dt;
	long sleepfor;
	long double elcharge, gconst, epsno;
	unsigned int obj;
	unsigned short int avail_cores, oglmin, oglmax, verbosity;
	int width, height;
	bool vsync, novid, fullogl, moderandom, logenable;
	char fontname[200], filename[200];
	FILE *logfile;
};
