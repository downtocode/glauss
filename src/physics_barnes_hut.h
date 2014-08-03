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

/* Octree structure */
typedef struct phys_barnes_hut_octree {
	unsigned short score, depth;
	v4sd origin;
	double halfdim;
	bool leaf;
	data *data, cellsum;
	struct phys_barnes_hut_octree *cells[8];
} bh_octree;

/* Thread assignment tree structure */
typedef struct thread_alloc_tree {
	int assigned;
	bool leaf;
	bh_octree *mapped;
	struct thread_config_bhut *thread, *assign[9];
	struct thread_alloc_tree *parent, *subdiv[8];
} bh_thread;

/* Thread configuration struct */
struct thread_config_bhut {
	data* obj;
	struct phys_barnes_hut_octree *root, *octrees[8];
	unsigned int id, objs_low, objs_high;
	struct thread_statistics *stats;
};

/* Returns flux of octrees */
unsigned int bh_build_octree(data* object, bh_octree *octree, bh_octree *root);
unsigned int bh_cleanup_octree(bh_octree *octree, bh_octree *root);

/* Completely removes octree */
void bh_decimate_octree(bh_octree *octree);

/* Prints any cells and objects still inside octree */
void bh_print_octree(bh_octree *octree);

/* Used during init only to get a valid octree data */
double bh_max_displacement(data *object, bh_octree *octree);
void bh_update_center_of_mass(data *object, bh_octree *octree);

/* Init a tree(start of one - NOT A SUBCELL) */
bh_octree *bh_init_tree();
/* Get octant an object is in(mostly used for direction) */
short bh_get_octant(v4sd *pos, bh_octree *octree);
/* Checks if object is within a specific octree */
bool bh_recurse_check_obj(data *object, bh_octree *target, bh_octree *root);

/* Start - Thread - Stop */
void** bhut_init(data** object, struct thread_statistics **stats);
void *thread_barnes_hut(void *thread_setts);
void bhut_quit(void **threads);

#endif
