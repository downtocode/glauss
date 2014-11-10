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

#define PHYS_BARNES_HUT {                                                      \
		.name = "barnes-hut",                                                  \
		.version = PACKAGE_VERSION,                                            \
		.desc = "Barnes-Hut simulation, velocity verlet integration",          \
		.author = "Rostislav Pehlivanov",                                      \
		.thread_preconfiguration = bhut_preinit,                               \
		.thread_configuration = bhut_init,                                     \
		.thread_location = thread_bhut,                                        \
		.thread_destruction = bhut_quit,                                       \
		.thread_sched_fn = bhut_balance_threads                                \
	}

/* Octree structure */
typedef struct phys_barnes_hut_octree {
	unsigned short score, depth;
	vec3 origin;
	long double halfdim;
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
	data *obj;
	struct phys_barnes_hut_octree *root, *octrees[8];
	unsigned int id, objs_low, objs_high;
	struct global_statistics *glob_stats;
	struct thread_statistics *stats;
	pthread_barrier_t *ctrl, *barrier;
	pthread_mutex_t *mute, *root_lock;
	bool *quit;
};

/* Returns flux of octrees */
unsigned int bh_build_octree(data* object, bh_octree *octree, bh_octree *root);
unsigned int bh_cleanup_octree(bh_octree *octree);

/* Completely removes octree */
void bh_decimate_octree(bh_octree *octree);

/* Prints any cells and objects inside octree */
void bh_print_octree(bh_octree *octree);
/* Prints octrees and their dimensions */
void bh_depth_print(bh_octree *octree);

/* Used during init only to get a valid octree data */
double bh_init_max_displacement(data *object, bh_octree *octree);
void bh_init_center_of_mass(data *object, bh_octree *octree);

/* Init a tree(start of one - NOT A SUBCELL) */
bh_octree *bh_init_tree(void);
/* Get octant an object is in(mostly used for direction) */
short bh_get_octant(vec3 *pos, bh_octree *octree);
/* Checks if object is within a specific octree */
bool bh_recurse_check_obj(data *object, bh_octree *target, bh_octree *root);

/* Functions */
void bhut_balance_threads(void **threads);
void *bhut_preinit(struct glob_thread_config *cfg);
void **bhut_init(struct glob_thread_config *cfg);
void *thread_bhut(void *thread_setts);
void *thread_bhut_rk4(void *thread_setts);
void bhut_quit(void **threads);

#endif
