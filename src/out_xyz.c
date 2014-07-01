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
#include <string.h>
#include <stdbool.h>
#include "physics.h"
#include "out_xyz.h"
#include "msg_phys.h"
#include "physics_aux.h"

int toxyz(int obj, data *object, float timestep)
{
	const char *extension = "xyz", *file = "config";
	char filetodump[120];
	sprintf(filetodump, "%s_%0.2f.%s", file, timestep, extension);
	FILE *out = fopen ( filetodump, "w" );
	pprintf(PRI_ESSENTIAL, "Created %s\n", filetodump);
	fprintf(out, "%i\n", obj);
	fprintf(out, "#Current dump = %0.2f\n", timestep);
	for(int i = 1; i < obj + 1; i++) {
		fprintf(out, "%s %f %f %f\n", atom_prop[object[i].atomnumber].name, object[i].pos[0], object[i].pos[1], object[i].pos[2]);
	}
	fclose(out);
	return 0;
}
