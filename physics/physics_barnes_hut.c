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
#include <stdlib.h>
#include <tgmath.h>
#include <pthread.h>
#include <malloc.h>
#include <limits.h>
#include "main/options.h"
#include "main/msg_phys.h"
#include "physics.h"
#include "physics_barnes_hut.h"

/* Repeated indent string when printing */
#define INDENT "  "

/* Thread local storage allocation stats */
static _Thread_local unsigned int allocated_cells;

/* Will be replaced with atomics as soon as Clang supports stdatomic.h */
static pthread_mutex_t root_lock;

/* Used in qsort to assign threads in octrees */
static int thread_cmp_assigned(const void *a, const void *b)
{ return ((*(bh_thread **)a)->assigned - (*(bh_thread **)b)->assigned); }

/* Will distribute the threads in each assignment tree */
static void bh_add_thread(bh_thread *root, 
						  bh_octree *octree, struct thread_config_bhut *thread)
{
	/* Not the easiest code to understand without comments. */
	for(short i=0; i < 8; i++) {
		if(!root->subdiv[i])
			root->subdiv[i] = calloc(1, sizeof(bh_thread));
		if(!octree->cells[i]) {
			octree->cells[i] = bh_init_tree();
			octree->cells[i]->halfdim = octree->halfdim/2;
			octree->cells[i]->origin = octree->origin + (double)octree->halfdim*0.5*\
				(vec3){ (i&4 ? 1 : -1), (i&2 ? 1 : -1), (i&1 ? 1 : -1) };
		}
		octree->cells[i]->depth = octree->depth + 1;
		root->subdiv[i]->mapped = octree->cells[i];
		root->subdiv[i]->parent = root;
		root->subdiv[i]->assigned = 0;
	}
	/* Assign the thread to the root octree */
	root->assign[root->assigned] = thread;
	/* Wipe other threads' previous octree assignments */
	for(short v = 0; v < root->assigned + 1; v++) {
		if(root->assign[v]) {
			for(short d = 0; d < 8; d++) {
				root->assign[v]->octrees[d] = NULL;
			}
		}
	}
	/* Split the octree and assign the threads */
	short distrb = 8/root->assigned, remain = 8%root->assigned, oct = 0;
	for(int k = 1; k < root->assigned + 1; k++) {
		/* Calculate the threads' share. Add the calculated remain to the first ones. */
		int thread_conts = distrb + ((remain-- > 0) ? 1 : 0);
		for(int l = 0; l < thread_conts; l++) {
			/* Map the octrees to the real BH octree */
			root->assign[k]->octrees[oct] = root->subdiv[oct]->mapped;
			/* Assign the threads within the octree from the root. */
			root->subdiv[oct]->assign[++root->subdiv[oct]->assigned] = root->assign[k];
			oct++;
		}
	}
	return;
}

/* Recursive function to assign threads to octrees */
static void bh_assign_thread(bh_thread *root,
							 bh_octree *octree, struct thread_config_bhut *thread)
{
	if(!thread || !root) return;
	else if(++root->assigned < option->bh_tree_limit + 1) {
		bh_add_thread(root, octree, thread);
	} else {
		/* Create backup to prevent qsort from scrambling them.
		 * Otherwise we'd have to recursively update their origins. Not fun. */
		struct thread_alloc_tree *keep[8];
		for(short i=0; i < 8; i++) keep[i] = root->subdiv[i];
		
		/* Sort: lowest to highest, [0]'s always the least assigned one. */
		qsort(keep, 8, sizeof(bh_thread *), thread_cmp_assigned);
		
		/* Find the lowest occupied subcell's original octree subcell */
		short int i;
		for(i=0; i < 8; i++) if(root->subdiv[i] == keep[0]) break;
		
		/* Insert the thread into it */
		bh_assign_thread(root->subdiv[i], root->subdiv[i]->mapped, thread);
	}
	return;
}

static void bh_decimate_assignment_tree(bh_thread *root)
{
	for(short i=0; i < 8; i++) {
		if(root->subdiv[i]) bh_decimate_assignment_tree(root->subdiv[i]);
	}
	free(root);
}

static void bh_print_thread_tree(struct thread_config_bhut **thread)
{
	for(unsigned int m=1; m < option->threads + 1; m++) {
		pprintf(PRI_HIGH, "   %02i   | ", m);
		for(short h=0; h < 8; h++) {
			if(thread[m]->octrees[h]) {
				pprintf(PRI_HIGH, "%i(%u) ", h, thread[m]->octrees[h]->depth);
			}
		}
		pprintf(PRI_HIGH, "\n");
	}
}

void **bhut_init(struct glob_thread_config *cfg)
{
#ifdef __linux__
	/* Use this size for glibc's fastbins. In theory any memory below this size
	 * will not be consolidated together, allowing us to allocate and free
	 * memory real fast. Not portable. */
	mallopt(M_MXFAST, sizeof(bh_octree));
#endif
	
	struct thread_config_bhut **thread_config = calloc(option->threads+1,
											sizeof(struct thread_config_bhut*));
	for(int k = 0; k < option->threads + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_bhut));
	}
	
	/* Check if options are within their limits */
	if(option->bh_tree_limit  < 2 || option->bh_tree_limit > 8) {
		pprintf(PRI_ERR, "[BH] Option bh_tree_limit must be within [2,8]!");
		exit(1);
	}
	
	/* Init mutex */
	pthread_mutex_init(&root_lock, NULL);
	
	/* Init barrier */
	pthread_barrier_t *barrier = calloc(1, sizeof(pthread_barrier_t));
	pthread_barrier_init(barrier, NULL, option->threads);
	
	/* Create the root octree */
	bh_octree *root_octree = bh_init_tree();
	bh_init_center_of_mass(cfg->obj, root_octree);
	root_octree->halfdim = bh_init_max_displacement(cfg->obj, root_octree);
	
	/* Create temporary assignment tree for threads */
	bh_thread *thread_tree = calloc(1, sizeof(bh_thread));
	
	int totcore = (int)((float)option->obj/option->threads);
	for(int k = 1; k < option->threads + 1; k++) {
		thread_config[k]->root = root_octree;
		/* Recursively insert the threads into assignment octree and map it to
		 * the root octree */
		bh_assign_thread(thread_tree, root_octree, thread_config[k]);
		thread_config[k]->stats = cfg->stats[k];
		thread_config[k]->obj = cfg->obj;
		thread_config[k]->ctrl = cfg->ctrl;
		thread_config[k]->barrier = barrier;
		thread_config[k]->id = k;
		thread_config[k]->objs_low = thread_config[k-1]->objs_high + 1;
		thread_config[k]->objs_high = thread_config[k]->objs_low + totcore - 1;
		if(k == option->threads) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_config[k]->objs_high += option->obj - thread_config[k]->objs_high;
		}
	}
	/* Workaround when only a single thread is available.
	 * Reason why we couldn't have this in the bh_assign_thread function:
	 * this happens ONLY once in this extremely unique corner case. */
	if(option->threads == 1 && option->bh_single_assign) {
		for(short h=0; h < 8; h++) {
			thread_config[1]->octrees[h] = NULL;
		}
		thread_config[1]->octrees[0] = root_octree;
	} else {
		pprintf(PRI_HIGH, " Thread | Assignments: oct(lvl)\n");
		bh_print_thread_tree(thread_config);
	}
	
	/* Free assignment octree since all's been done with */
	bh_decimate_assignment_tree(thread_tree);
	
	/* Turn on BH specific stats */
	option->stats_bh = true;
	
	return (void**)thread_config;
}

/* Deinit function */
void bhut_quit(void **threads) {
	struct thread_config_bhut **t = (struct thread_config_bhut **)threads;
	/* Root mutex */
	pthread_mutex_destroy(&root_lock);
	
	/* Barrier */
	pthread_barrier_destroy(t[1]->barrier);
	free(t[1]->barrier);
	
	/* Main octree */
	bh_decimate_octree(t[1]->root);
	
	for(int k = 0; k < option->threads + 1; k++) {
		free(t[k]);
	}
	free(t);
	option->stats_bh = false;
	return;
}

/* INIT ONLY: Returns furthest object's furthest position from an octree. */
double bh_init_max_displacement(data *object, bh_octree *octree)
{
	double maxdist = 0.0;
	vec3 dist;
	for(int i = 1; i < option->obj + 1; i++) {
		dist = object[i].pos - octree->origin;
		for(int j = 0; j < 3; j++) {
			if(dist[j] > maxdist) {
				maxdist = dist[j];
			}
		}
	}
	return maxdist;
}

/* INIT ONLY: Will calculate and update an octree's center of mass */
void bh_init_center_of_mass(data *object, bh_octree *octree)
{
	vec3 center = (vec3){0,0,0};
	for(int i = 1; i < option->obj + 1; i++) {
		center = (center + object[i].pos)/2;
	}
	octree->origin = octree->cellsum.pos = center;
}

/* Completely free an octree */
void bh_decimate_octree(bh_octree *octree)
{
	for(short i=0; i < 8; i++) {
		if(octree->cells[i]) {
			bh_decimate_octree(octree->cells[i]);
			allocated_cells--;
		}
	}
	free(octree);
}

/* Recursive function to scrub off all objects, update scores and reset mass. */
static bool bh_clean_octree(bh_octree *octree)
{
	octree->cellsum.mass = 0;
	if(octree->data) {
		/* Object was here, chances are cell will be used, do not free yet */
		octree->data = NULL;
		return 0;
	} else {
		/* No object, if end node we reduce its score and check if 0 */
		if(octree->leaf) return !octree->score-- ? 1 : 0;
		/* Not end node, recursively check non-empty cells */
		short filled_cells = 8;
		for(short i=0; i < 8; i++) {
			if(octree->cells[i]) {
				if(bh_clean_octree(octree->cells[i])) {
					/* Since cell is empty, free it */
					free(octree->cells[i]);
					octree->cells[i] = NULL;
					filled_cells--;
					allocated_cells--;
				}
			} else filled_cells--;
		}
		if(!filled_cells) {
			/* No child cells, set as leaf and reduce score */
			octree->leaf = 1;
			return !octree->score-- ? 1 : 0;
		} else return 0;
		
	}
}

/* Wrapper function to return a threads' new allocations */
unsigned int bh_cleanup_octree(bh_octree *octree)
{
	if(!octree) return 0;
	unsigned int prev_allocated_cells = allocated_cells;
	bh_clean_octree(octree);
	return prev_allocated_cells - allocated_cells;
}

/* Returns the octant of an octree a position is in */
short bh_get_octant(vec3 *pos, bh_octree *octree)
{
	short oct = 0;
	if((*pos)[0] >= octree->origin[0]) oct |= 4;
	if((*pos)[1] >= octree->origin[1]) oct |= 2;
	if((*pos)[2] >= octree->origin[2]) oct |= 1;
	return oct;
}

/* Inits a cell, in case it isn't, updates score and position */
static void bh_init_cell(bh_octree *octree, short k)
{
	if(allocated_cells*sizeof(bh_octree) > option->bh_heapsize_max) {
		pprintf(PRI_ERR, "Reached maximum octree heapsize of %lu bytes!\n",
				option->bh_heapsize_max);
		exit(3);
	}
	if(!octree->cells[k]) {
		octree->cells[k] = calloc(1, sizeof(bh_octree));
		octree->cells[k]->depth = octree->depth+1;
		octree->cells[k]->leaf = 1;
		octree->cells[k]->score = option->bh_lifetime;
		allocated_cells++;
	}
	/* Scoring system: object is in therefore increase score */
	octree->cells[k]->score++;
	octree->cells[k]->halfdim = octree->halfdim/2;
	octree->cells[k]->origin = octree->origin + (double)octree->halfdim*0.5*\
		(vec3){ (k&4 ? 1 : -1), (k&2 ? 1 : -1), (k&1 ? 1 : -1) };
}

/* Recursive function to insert object into an octree */
static void bh_insert_object(data *object, bh_octree *octree)
{
	/* Update octree mass/center of mass. */
	octree->cellsum.pos = (octree->cellsum.pos+object->pos)/2;
	octree->cellsum.mass += object->mass;
	if(!octree->data && octree->leaf) {
		/* This cell has no object or subcells */
		octree->data = object;
	} else if(octree->data && octree->leaf) {
		/* This cell has object but no subcells */
		short oct_current_obj = bh_get_octant(&object->pos, octree);
		short oct_octree_obj = bh_get_octant(&octree->data->pos, octree);
		bh_init_cell(octree, oct_octree_obj);
		bh_insert_object(octree->data, octree->cells[oct_octree_obj]);
		octree->data = NULL;
		octree->leaf = 0;
		bh_init_cell(octree, oct_current_obj);
		bh_insert_object(object, octree->cells[oct_current_obj]);
	} else {
		/* This cell has subcells with objects */
		short oct_current_obj = bh_get_octant(&object->pos, octree);
		bh_init_cell(octree, oct_current_obj);
		bh_insert_object(object, octree->cells[oct_current_obj]);
	}
}

/* Prints octree, with indentation defined by INDENT macro */
void bh_print_octree(bh_octree *octree)
{
	if(octree->data) {
		for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
		pprintf(PRI_SPAM, "Object %i(pos = {%f, %f, %f}) is at cell level %i\n",
				octree->data->id, octree->data->pos[0], octree->data->pos[1],
				octree->data->pos[2], octree->depth);
	} else {
		for(short i=0; i < 8; i++) {
			if(octree->cells[i]) {
				for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
				pprintf(PRI_SPAM, 
						"Cell(pos = {%f, %f, %f},\
						mass(center) = {%f, %f, %f},\
						mass = %lf) contents:\n", octree->cells[i]->origin[0],
						octree->cells[i]->origin[1],
						octree->cells[i]->origin[2],
						octree->cells[i]->cellsum.pos[0],
						octree->cells[i]->cellsum.pos[1],
						octree->cells[i]->cellsum.pos[2],
						octree->cells[i]->cellsum.mass);
				bh_print_octree(octree->cells[i]);
			} else {
				for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
				pprintf(PRI_SPAM, "Cell empty.\n",  octree->origin[0],
						octree->origin[1], octree->origin[2]);
			}
		}
	}
}

/* Just a debug tool */
void bh_depth_print(bh_octree *octree)
{
	if(!octree) return;
	printf("Oct depth = %i, halfdim = %Lf\n", octree->depth, octree->halfdim);
	for(short i=0; i < 8; i++) {
		if(octree->cells[i]) bh_depth_print(octree->cells[i]);
	}
}

/* Will init a sub-root octree as root. We have to do the sync ourselves too. */
bh_octree *bh_init_tree()
{
	bh_octree *octree = calloc(1, sizeof(bh_octree));
	octree->score = USHRT_MAX;
	octree->leaf = 0;
	return octree;
}

/* Check whether the object is in the target octree. Returns 1 in case it is. */
bool bh_recurse_check_obj(data *object, bh_octree *target, bh_octree *root)
{
	if(!root) return 0;
	if(root->depth >= target->depth)
		return root == target;
	/* Cascade the return value */
	else return bh_recurse_check_obj(object, target,
								root->cells[bh_get_octant(&object->pos, root)]);
}

/* Recursive function, basis of the BH algorithm */
static void bh_calculate_force(data* object, bh_octree *octree)
{
	vec3 vecnorm = object->pos - octree->origin;
	double dist = sqrt(vecnorm[0]*vecnorm[0] +\
					   vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
	vecnorm /= dist;
	if(octree->data) {
		if(octree->data == object) return;
		object->acc += -vecnorm*\
		(option->gconst*octree->data->mass)/(dist*dist);
	} else if((octree->halfdim/dist) < option->bh_ratio) {
		object->acc += -vecnorm*\
		(option->gconst*octree->cellsum.mass)/(dist*dist);
	} else {
		for(short i=0; i < 8; i++) {
			if(octree->cells[i]) {
				bh_calculate_force(object, octree->cells[i]);
			}
		}
	}
}

/* Checks whether an object belongs to a specific octree and returns 1 if so. */
unsigned int bh_build_octree(data* object, bh_octree *octree, bh_octree *root)
{
	unsigned int prev_allocated_cells = allocated_cells;
	if(!octree || !root || !object) return 0;
	for(int i = 1; i < option->obj + 1; i++) {
		if(bh_recurse_check_obj(&object[i], octree, root)) {
			bh_insert_object(&object[i], octree);
		}
	}
	return allocated_cells - prev_allocated_cells;
}

/* Will sync the centre of mass and origin of all related octrees root-target */
static void bh_cascade_mass(bh_octree *target, bh_octree *root)
{
	/* Will not update target */
	if(!root || !target) return;
	if(target->depth >= root->depth) return;
	pthread_mutex_lock(&root_lock);
		root->cellsum.pos = (root->cellsum.pos+target->cellsum.pos)/2;
		root->cellsum.mass += target->cellsum.mass;
	pthread_mutex_unlock(&root_lock);
	bh_cascade_mass(target, root->cells[bh_get_octant(&target->origin, root)]);
}

/* Sets the root's position and dimensions, resets mass */
static void bh_atomic_update_root(double dimension, bh_octree *root)
{
	if(!root) return;
	pthread_mutex_lock(&root_lock);
		root->cellsum.mass = 0;
		root->origin = root->cellsum.pos;
		if(dimension > root->halfdim) root->halfdim = dimension;
	pthread_mutex_unlock(&root_lock);
}

/* Sets the origins and dimensions of any thread octrees and in between root. */
static void bh_cascade_position(bh_octree *target, bh_octree *root)
{
	/* Will update the target as well */
	if(!root || !target) return;
	if(!target->depth) return;
	
	short oct = bh_get_octant(&target->origin, root);
	if(!root->cells[oct]) return;
	
	pthread_mutex_lock(&root_lock);
		root->cellsum.mass = 0;
		root->cells[oct]->halfdim = root->halfdim/2;
		root->cells[oct]->origin = root->origin + (double)root->halfdim*0.5*\
			(vec3){ (oct&4 ? 1 : -1), (oct&2 ? 1 : -1), (oct&1 ? 1 : -1) };
	pthread_mutex_unlock(&root_lock);
	
	if(root->cells[oct]->depth >= target->depth) return;
	else bh_cascade_position(target, root->cells[oct]);
}

void *thread_bhut(void *thread_setts)
{
	struct thread_config_bhut *t = thread_setts;
	vec3 accprev, dist;
	
	while(1) {
		unsigned int new_alloc = 0, new_cleaned = 0;
		double maxdist = 0.0;
		/* Move objects */
		for(unsigned int i = t->objs_low; i < t->objs_high + 1; i++) {
			if(t->obj[i].ignore) continue;
			t->obj[i].pos += (t->obj[i].vel*option->dt) +\
				(t->obj[i].acc)*((option->dt*option->dt)/2);
		}
		
		pthread_barrier_wait(t->barrier);
		
		/* Build octree */
		for(short s=0; s < 8; s++) {
			new_alloc += bh_build_octree(t->obj, t->octrees[s], t->root);
			/* Sync mass and center of mass with any higher trees */
			bh_cascade_mass(t->octrees[s], t->root);
		}
		
		pthread_barrier_wait(t->barrier);
		
		/* Calculate force */
		for(unsigned int i = t->objs_low; i < t->objs_high + 1; i++) {
			accprev = t->obj[i].acc;
			bh_calculate_force(&t->obj[i], t->root);
			t->obj[i].vel += (t->obj[i].acc + accprev)*((option->dt)/2);
			/* Get updated maximum for root while we're at it. */
			dist = t->obj[i].pos - t->root->origin;
			for(int j = 0; j < 3; j++) if(dist[j] > maxdist) maxdist = dist[j];
		}
		
		/* Sync & wakeup control thread */
		pthread_barrier_wait(t->ctrl);
		
		/* Insert updated halfdim into root */
		bh_atomic_update_root(maxdist, t->root);
		
		/* Update the positions of octrees and their halfdims + cleanup */
		for(short s=0; s < 8; s++) {
			new_cleaned += bh_cleanup_octree(t->octrees[s]);
			bh_cascade_position(t->octrees[s], t->root);
		}
		
		/* Update statistics */
		t->stats->bh_total_alloc  =  allocated_cells;
		t->stats->bh_new_alloc    =  new_alloc;
		t->stats->bh_new_cleaned  =  new_cleaned;
		t->stats->bh_heapsize     =  sizeof(bh_octree)*allocated_cells;
		
		/* Quit if requested */
		pthread_testcancel();
	}
	return 0;
}