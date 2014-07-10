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
	unsigned short score, depth;
	v4sd origin;
	double halfdim;
	bool leaf;
	data *data, cellsum;
	struct phys_barnes_hut_octree *cells[8];
};

struct thread_config_bhut {
	data* obj;
	struct phys_barnes_hut_octree *octree, *root_octree, *octrees[8];
	unsigned int id, objs_low, objs_high;
	struct thread_statistics *stats;
};

void** bhut_init(data** object, struct thread_statistics **stats);
unsigned int bh_cleanup_octree(struct phys_barnes_hut_octree *octree);
void bh_print_octree(struct phys_barnes_hut_octree *octree);
double bh_max_displacement(data *object, struct phys_barnes_hut_octree *octree);
struct phys_barnes_hut_octree *bh_init_tree();
void bh_build_octree(data* object, struct phys_barnes_hut_octree *octree);
void *thread_barnes_hut(void *thread_setts);

#endif
