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
#include "options.h"
#include "msg_phys.h"
#include "physics.h"
#include "physics_barnes_hut.h"

//Indent string when verbosely outputting octrees to stdout
#define INDENT "  "

//Thread local storage allocation stats
static _Thread_local unsigned int allocated_cells;

static void bh_dist_tree_threads(struct thread_config_bhut **thread_config, 
								 bh_octree *root, unsigned int threads) {
	if(threads == 1) {
		printf("Thr has entire tree\n");
		thread_config[1]->octree = root;
		return;
	}
	short distrb = 8/threads, remain = 8%threads, oct = 0;
	for(int k = 1; k < threads + 1; k++) {
		int thread_conts = distrb + ((remain-- > 0) ? 1 : 0);
		printf("Thr %i has %i ( ", k, thread_conts);
		for(int l = 0; l < thread_conts; l++) {
			thread_config[k]->octrees[oct] = root->cells[oct] = bh_init_tree();
			printf("octang %i ", oct++);
		}
		printf(")\n");
	}
	return;
}

static void bh_master_dist(struct thread_config_bhut **thread_config, 
						   bh_octree *root, unsigned int threads) {
	float thread_divs = (float)threads/8;
	for(int m = 0; m < thread_divs; m++) {
		if(threads > 8) {
			bh_dist_tree_threads(thread_config, root, 8);
			threads -= 8;
		} else bh_dist_tree_threads(thread_config, root, threads);
	}
}

void** bhut_init(data** object, struct thread_statistics **stats)
{	
	/* Use this size for glibc's fastbins. In theory any memory below this size
	 * will not be consolidated together, allowing us to allocate and free
	 * memory real fast. Not portable. */
	mallopt(M_MXFAST, sizeof(struct phys_barnes_hut_octree));
	
	struct thread_config_bhut **thread_config = calloc(option->avail_cores+1,
											sizeof(struct thread_config_bhut*));
	for(int k = 0; k < option->avail_cores + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_bhut));
	}
	
	bh_octree *root_octree = bh_init_tree();
	
	int totcore = (int)((float)option->obj/option->avail_cores);
	for(int k = 1; k < option->avail_cores + 1; k++) {
		thread_config[k]->stats = stats[k];
		thread_config[k]->root_octree = root_octree;
		thread_config[k]->obj = *object;
		thread_config[k]->id = k;
		thread_config[k]->objs_low = thread_config[k-1]->objs_high + 1;
		thread_config[k]->objs_high = thread_config[k]->objs_low + totcore - 1;
		if(k == option->avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_config[k]->objs_high += option->obj - thread_config[k]->objs_high;
		}
	}
	
	bh_master_dist(thread_config, root_octree, option->avail_cores);
	printf("\n\n\n");
	bh_print_octree(root_octree);
	
	exit(0);
	
	
	return (void**)thread_config;
}

static void bh_decimate_octree(bh_octree *octree) {
	for(short i=0; i < 8; i++) {
		if(octree->cells[i]) {
			bh_decimate_octree(octree->cells[i]);
			free(octree->cells[i]);
			allocated_cells--;
		}
	}
}

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

unsigned int bh_cleanup_octree(bh_octree *octree)
{
	unsigned int prev_allocated_cells = allocated_cells;
	bh_clean_octree(octree);
	return prev_allocated_cells - allocated_cells;
}

static short bh_get_octant(data *object, bh_octree *octree)
{
	short oct = 0;
	if(object->pos[0] >= octree->origin[0]) oct |= 4;
	if(object->pos[1] >= octree->origin[1]) oct |= 2;
	if(object->pos[2] >= octree->origin[2]) oct |= 1;
	return oct;
}


static void bh_init_cell(bh_octree *octree, short k)
{
	if(!octree->cells[k]) {
		octree->cells[k] = calloc(1, sizeof(struct phys_barnes_hut_octree));
		octree->cells[k]->depth = octree->depth+1;
		octree->cells[k]->leaf = 1;
		allocated_cells++;
	}
	octree->cells[k]->score = option->bh_lifetime;
	octree->cells[k]->halfdim = octree->halfdim/2;
	octree->cells[k]->origin = octree->origin + (octree->halfdim*\
				(v4sd){
						(k&4 ? .5f : -.5f),
						(k&2 ? .5f : -.5f),
						(k&1 ? .5f : -.5f)
				});
}

static void bh_insert_object(data *object, bh_octree *octree)
{
	//Update octree mass/center of mass.
	octree->cellsum.pos = (octree->cellsum.pos+object->pos)/2;
	octree->cellsum.mass += object->mass;
	if(!octree->data && octree->leaf) {
		//This cell has no object or subcells
		octree->data = object;
	} else if(octree->data && octree->leaf) {
		//This cell has object but no subcells
		short oct_current_obj = bh_get_octant(object, octree);
		short oct_octree_obj = bh_get_octant(octree->data, octree);
		bh_init_cell(octree, oct_octree_obj);
		bh_insert_object(octree->data, octree->cells[oct_octree_obj]);
		octree->data = NULL;
		octree->leaf = 0;
		bh_init_cell(octree, oct_current_obj);
		bh_insert_object(object, octree->cells[oct_current_obj]);
	} else {
		//This cell has subcells with objects
		short oct_current_obj = bh_get_octant(object, octree);
		bh_init_cell(octree, oct_current_obj);
		bh_insert_object(object, octree->cells[oct_current_obj]);
	}
}

void bh_print_octree(bh_octree *octree)
{
	if(octree->data) {
		for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
		pprintf(PRI_SPAM, "Object %i(pos = {%f, %f, %f}) is at cell level %i\n",
				octree->data->id, octree->data->pos[0], octree->data->pos[1],
				octree->data->pos[2], octree->depth);
	} else {
		for(int i=0; i < 8; i++) {
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

double bh_max_displacement(data *object, bh_octree *octree)
{
	double maxdist = 0.0;
	v4sd dist;
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

bh_octree *bh_init_tree()
{
	bh_octree *octree = calloc(1,
		sizeof(struct phys_barnes_hut_octree));
	octree->leaf = 1;
	return octree;
}

void bh_build_octree(data* object, bh_octree *octree)
{
	/* The cleanup function could delete the octree */
	if(!octree) octree = bh_init_tree();
	/* The distribution will change so we need to account for this. */
	octree->origin = octree->cellsum.pos;
	octree->halfdim = bh_max_displacement(object, octree);
	for(int i = 1; i < option->obj + 1; i++) {
		bh_insert_object(&object[i], octree);
	}
}

static void bh_calculate_force(data* object, bh_octree *octree)
{
	if(octree->leaf && !octree->data) return;
	if(octree->data == object) return;
	v4sd vecnorm = object->pos - octree->origin;
	double dist = sqrt(vecnorm[0]*vecnorm[0] +\
					   vecnorm[1]*vecnorm[1] +\
					   vecnorm[2]*vecnorm[2]);
	vecnorm /= dist;
	if(octree->data) {
		object->acc += -vecnorm*\
						(option->gconst*octree->data->mass)/(dist*dist);
	} else if((octree->halfdim/dist) < option->bh_ratio) {
		object->acc += -vecnorm*\
						(option->gconst*octree->cellsum.mass)/(dist*dist);
	} else {
		for(int i=0; i < 8; i++) {
			if(octree->cells[i]) {
				bh_calculate_force(object, octree->cells[i]);
			}
		}
	}
}

void *thread_barnes_hut(void *thread_setts)
{
	struct thread_config_bhut *thread = thread_setts;
	v4sd accprev;
	
	while(!quit) {
		for(unsigned int i = thread->objs_low; i < thread->objs_high + 1; i++) {
			if(thread->obj[i].ignore) continue;
			thread->obj[i].pos += (thread->obj[i].vel*option->dt) +\
				(thread->obj[i].acc)*((option->dt*option->dt)/2);
		}
		
		if(thread->id == 1) bh_build_octree(thread->obj, thread->root_octree);
		thread->stats->bh_allocated = allocated_cells;
		thread->stats->bh_heapsize = sizeof(struct phys_barnes_hut_octree)*allocated_cells;
		
		pthread_barrier_wait(&barrier);
		
		for(unsigned int i = thread->objs_low; i < thread->objs_high + 1; i++) {
			accprev = thread->obj[i].acc;
			bh_calculate_force(&thread->obj[i], thread->root_octree);
			thread->obj[i].vel += (thread->obj[i].acc + accprev)*((option->dt)/2);
		}
		
		thread->stats->progress += option->dt;
		
		if(thread->id == 1) {
			thread->stats->bh_cleaned = bh_cleanup_octree(thread->root_octree);
		}
		
		pthread_barrier_wait(&barrier);
	}
	if(thread->id == 1) bh_decimate_octree(thread->root_octree);
	thread->stats->bh_allocated = allocated_cells;
	return 0;
}
