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
#ifndef PHYSENGINE_PHYS_BARNES_HUT
#define PHYSENGINE_PHYS_BARNES_HUT

struct phys_barnes_hut_octree {
	unsigned int depth;
	v4sd origin, halfdim;
	bool leaf;
	data *data, cellsum;
	struct phys_barnes_hut_octree *cells[8];
};

void insert_into_octree(data *object, struct phys_barnes_hut_octree *octree);
double max_disp_from_origin(data *object);
void build_octree(data* object, struct phys_barnes_hut_octree *octree);
void *thread_barnes_hut(void *thread_setts);

#endif
