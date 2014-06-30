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
#include "options.h"
#include "msg_phys.h"
#include "physics.h"
#include "physics_barnes_hut.h"

//Indent string when verbosely outputting octrees to stdout
#define INDENT "  "

//Octree's initial score, decremented every time it's empty, once it reaches 0 it's freed
#define OCTREE_INIT_SCORE 48

static unsigned int allocated_cells;

void** bhut_init(data** object)
{
	struct thread_config_bhut **thread_config = calloc(option->avail_cores+1, sizeof(struct thread_config_bhut*));
	for(int k = 0; k < option->avail_cores + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_bhut));
	}
	
	thread_config[1]->root_octree = bh_init_tree();
	
	int totcore = (int)((float)option->obj/option->avail_cores);
	
	for(int k = 1; k < option->avail_cores + 1; k++) {
		thread_config[k]->root_octree = thread_config[1]->root_octree;
		thread_config[k]->octree = bh_init_tree();
		thread_config[k]->obj = *object;
		thread_config[k]->id = k;
		thread_config[k]->objs_low = thread_config[k-1]->objs_high + 1;
		thread_config[k]->objs_high = thread_config[k]->objs_low + totcore - 1;
		if(k == option->avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_config[k]->objs_high += option->obj - thread_config[k]->objs_high;
		}
	}
	
	return (void**)thread_config;
}

static int bh_clean_octree(struct phys_barnes_hut_octree *octree)
{
	octree->cellsum = (data){{0}};
	if(octree->data != NULL) {
		/* Remove object, reduce score */
		octree->data = NULL;
		return !octree->score-- ? 1 : 0;
	} else {
		int filled_cells = 8;
		for(int i=0; i < 8; i++) {
			if(octree->cells[i] != NULL) {
				if(bh_clean_octree(octree->cells[i])) {
					/* Since cell is empty, free it */
					free(octree->cells[i]);
					octree->cells[i] = NULL;
					filled_cells--;
					allocated_cells--;
				}
			} else filled_cells--;
		}
		return !filled_cells ? 1 : 0;
	}
}

unsigned int bh_cleanup_octree(struct phys_barnes_hut_octree *octree)
{
	unsigned int prev_allocated_cells = allocated_cells;
	bh_clean_octree(octree);
	return prev_allocated_cells - allocated_cells;
}

static short bh_get_octant(data *object, const struct phys_barnes_hut_octree *octree)
{
	short oct = 0;
	if(object->pos[0] >= octree->origin[0]) oct |= 4;
	if(object->pos[1] >= octree->origin[1]) oct |= 2;
	if(object->pos[2] >= octree->origin[2]) oct |= 1;
	return oct;
}

static double doublemax;

static void bh_init_cell(struct phys_barnes_hut_octree *octree, short cell)
{
	
	/* TODO: maybe try to use a gigantic array of structs and glue them together in post */
	
	if(allocated_cells++ > 2000000) {
		printf("Too many cells! Total allocated cells = %i\n", allocated_cells);
		printf("Total size of all cells = %lf Mb\n", (sizeof(struct phys_barnes_hut_octree)*allocated_cells)/1048576.0);
		printf("Maxdist = %f\n", doublemax);
		exit(0);
	}
	octree->cells[cell] = calloc(1, sizeof(struct phys_barnes_hut_octree));
	octree->cells[cell]->depth = octree->depth+1;
	octree->cells[cell]->data = NULL;
	octree->cells[cell]->leaf = 1;
	octree->cells[cell]->score = OCTREE_INIT_SCORE;
	for(int i=0; i < 8; i++) octree->cells[cell]->cells[i] = NULL;
	v4sd newpos = octree->origin;
	newpos[0] += octree->halfdim * (cell&4 ? .5f : -.5f);
	newpos[1] += octree->halfdim * (cell&2 ? .5f : -.5f);
	newpos[2] += octree->halfdim * (cell&1 ? .5f : -.5f);
	octree->cells[cell]->origin = newpos;
	octree->cells[cell]->halfdim = octree->halfdim/2;
}

static void bh_insert_object(data *object, struct phys_barnes_hut_octree *octree)
{
	//Update octree mass/center of mass.
	octree->cellsum.pos = (octree->cellsum.pos+object->pos)/2;
	octree->cellsum.mass += object->mass;
	octree->cellsum.charge += object->charge;
	if(octree->data == NULL && octree->leaf) {
		//This cell has no object or subcells
		octree->data = object;
		pprintf(PRI_SPAM, "Object %i put in lvl %i\n", object->id, octree->depth);
	} else if(octree->data != NULL && octree->leaf) {
		//This cell has object but no subcells
		short oct_current_obj = bh_get_octant(object, octree);
		short oct_octree_obj = bh_get_octant(octree->data, octree);
		
		bh_init_cell(octree, oct_octree_obj);
		bh_insert_object(octree->data, octree->cells[oct_octree_obj]);
		octree->data = NULL;
		octree->leaf = 0;
		if(octree->cells[oct_current_obj] == NULL) bh_init_cell(octree, oct_current_obj);
		bh_insert_object(object, octree->cells[oct_current_obj]);
		
	} else {
		//This cell has subcells with objects
		short oct_current_obj = bh_get_octant(object, octree);
		if(octree->cells[oct_current_obj] == NULL) bh_init_cell(octree, oct_current_obj);
		bh_insert_object(object, octree->cells[oct_current_obj]);
	}
}

void bh_print_octree(struct phys_barnes_hut_octree *octree)
{
	if(octree->data != NULL) {
		for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
		pprintf(PRI_SPAM, "Object %i(pos = {%f, %f, %f}) is at cell level %i\n", octree->data->id, octree->data->pos[0], octree->data->pos[1], octree->data->pos[2], octree->depth);
	} else {
		for(int i=0; i < 8; i++) {
			if(octree->cells[i] != NULL) {
				for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
				pprintf(PRI_SPAM, "Cell(pos = {%f, %f, %f}, mass(center) = {%f, %f, %f}, mass = %lf) contents:\n", octree->cells[i]->origin[0], octree->cells[i]->origin[1], octree->cells[i]->origin[2],\
				octree->cells[i]->cellsum.pos[0], octree->cells[i]->cellsum.pos[1], octree->cells[i]->cellsum.pos[2], octree->cells[i]->cellsum.mass);
				bh_print_octree(octree->cells[i]);
			} else {
				for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, INDENT);
				pprintf(PRI_SPAM, "Cell empty.\n",  octree->origin[0], octree->origin[1], octree->origin[2]);
			}
		}
	}
}

double bh_max_displacement(data *object)
{
	double maxdist = 0.0;
	for(int i = 1; i < option->obj + 1; i++) {
		for(int j = 0; j < 3; j++) {
			if(object[i].pos[j] > maxdist) {
				maxdist = object[i].pos[j];
			}
		}
	}
	doublemax = maxdist;
	return doublemax;
}

struct phys_barnes_hut_octree *bh_init_tree()
{
	struct phys_barnes_hut_octree *octree = calloc(1, sizeof(struct phys_barnes_hut_octree));
	octree->depth=0;
	octree->data = NULL;
	octree->leaf = 1;
	octree->origin = (v4sd){0,0,0};
	for(int i=0; i < 8; i++) octree->cells[i] = NULL;
	return octree;
}

void bh_build_octree(data* object, struct phys_barnes_hut_octree *octree)
{
	/* The distribution will change so we need to account for this. */
	octree->halfdim = bh_max_displacement(object);
	for(int i = 1; i < option->obj + 1; i++) {
		bh_insert_object(&object[i], octree);
	}
}

static void bh_calculate_force(data* object, struct phys_barnes_hut_octree *octree)
{
	if(octree->leaf && octree->data == NULL) return;
	if(octree->data == object) return;
	v4sd vecnorm = object->pos - octree->origin;
	double dist = sqrt(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
	vecnorm /= dist;
	if(octree->data != NULL) {
		object->acc += vecnorm*(double)(option->gconst*octree->data->mass)/(dist*dist);
	} else if((octree->halfdim/dist) < option->bh_ratio) {
		object->acc += vecnorm*(double)(option->gconst*octree->cellsum.mass)/(dist*dist);
	} else {
		for(int i=0; i < 8; i++) {
			if(octree->cells[i] != NULL) {
				bh_calculate_force(object, octree->cells[i]);
			}
		}
	}
}

void *thread_barnes_hut(void *thread_setts)
{
	struct thread_config_bhut *thread = (struct thread_config_bhut *)thread_setts;
	v4sd accprev;
	
	while(!quit) {
		for(int i = thread->objs_low; i < thread->objs_high + 1; i++) {
			if(thread->obj[i].ignore) continue;
			thread->obj[i].pos += (thread->obj[i].vel*option->dt) +\
			(thread->obj[i].acc)*((option->dt*option->dt)/2);
		}
		
		if(thread->id == 1) bh_build_octree(thread->obj, thread->root_octree);
		
		pthread_barrier_wait(&barrier);
		
		for(int i = thread->objs_low; i < thread->objs_high + 1; i++) {
			accprev = thread->obj[i].acc;
			bh_calculate_force(&thread->obj[i], thread->root_octree);
			thread->obj[i].vel += (thread->obj[i].acc + accprev)*((option->dt)/2);
		}
		
		if(thread->id == 1) option->processed++;
		
		if(thread->id == 1) bh_cleanup_octree(thread->root_octree);
		
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
