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

static short getOctantContainingPoint(const v4sd point, const struct phys_barnes_hut_octree *octree) {
	short oct = 0;
	if(point[0] >= octree->origin[0]) oct |= 4;
	if(point[1] >= octree->origin[1]) oct |= 2;
	if(point[2] >= octree->origin[2]) oct |= 1;
	return oct;
}

static void init_octree_cell(struct phys_barnes_hut_octree *octree, short cell) {
	octree->cells[cell] = calloc(1, sizeof(struct phys_barnes_hut_octree));
	octree->cells[cell]->depth = octree->depth+1;
	octree->cells[cell]->data = NULL;
	octree->cells[cell]->leaf = 1;
	for(int i=0; i < 8; i++) octree->cells[cell]->cells[i] = NULL;
	v4sd newpos = octree->origin;
	newpos[0] += octree->halfdim * (cell&4 ? .5f : -.5f);
	newpos[1] += octree->halfdim * (cell&2 ? .5f : -.5f);
	newpos[2] += octree->halfdim * (cell&1 ? .5f : -.5f);
	octree->cells[cell]->origin = newpos;
	octree->cells[cell]->halfdim = octree->halfdim/2;
}

void insert_into_octree(data *object, struct phys_barnes_hut_octree *octree) {
	if(octree->data == NULL && octree->leaf) {
		//This cell has no object or subcells
		octree->data = object;
		pprintf(PRI_SPAM, "Object %i put in lvl %i\n", object->id, octree->depth);
	} else if(octree->data != NULL && octree->leaf) {
		//This cell has object but no subcells
		short oct_current_obj = getOctantContainingPoint(object->pos, octree);
		short oct_octree_obj = getOctantContainingPoint(octree->data->pos, octree);
		
		init_octree_cell(octree, oct_octree_obj);
		insert_into_octree(octree->data, octree->cells[oct_octree_obj]);
		octree->data = NULL;
		octree->leaf = 0;
		if(octree->cells[oct_current_obj] == NULL) init_octree_cell(octree, oct_current_obj);
		insert_into_octree(object, octree->cells[oct_current_obj]);
		
	} else {
		//This cell has subcells with objects
		short oct_current_obj = getOctantContainingPoint(object->pos, octree);
		if(octree->cells[oct_current_obj] == NULL) init_octree_cell(octree, oct_current_obj);
		insert_into_octree(object, octree->cells[oct_current_obj]);
	}
}

static void print_octree(struct phys_barnes_hut_octree *octree) {
	if(octree->data != NULL) {
		for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, "    ");
		pprintf(PRI_SPAM, "Object %i(pos = {%f, %f, %f}) is at cell level %i\n", octree->data->id, octree->data->pos[0], octree->data->pos[1], octree->data->pos[2], octree->depth);
	} else {
		for(int i=0; i < 8; i++) {
			if(octree->cells[i] != NULL) {
				for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, "    ");
				pprintf(PRI_SPAM, "Cell(pos = {%f, %f, %f}) contents:\n", octree->cells[i]->origin[0], octree->cells[i]->origin[1], octree->cells[i]->origin[2]);
				print_octree(octree->cells[i]);
			} else {
				for(int i = 0; i < octree->depth; i++) pprintf(PRI_SPAM, "    ");
				pprintf(PRI_SPAM, "Cell empty.\n",  octree->origin[0], octree->origin[1], octree->origin[2]);
			}
		}
	}
}

double max_disp_from_origin(data *object) {
	double maxdist = 0.0, dist = 0.0;
	for(int i = 1; i < option->obj + 1; i++) {
		dist = sqrt(object[i].pos[0]*object[i].pos[0] + object[i].pos[1]*object[i].pos[1] + object[i].pos[2]*object[i].pos[2]);
		if(dist > maxdist) {
			maxdist = dist;
		}
	}
	return maxdist;
}

void build_octree(data* object, struct phys_barnes_hut_octree *octree) {
	for(int i = 1; i < option->obj + 1; i++) {
		insert_into_octree(&object[i], octree);
	}
	printf("printing octree structure\n");
	print_octree(octree);
	exit(0);
}

void *thread_barnes_hut(void *thread_setts)
{
	//struct thread_config_nbody thread = *((struct thread_config_nbody *)thread_setts);
	
	while(!quit) {
		pthread_mutex_lock(&movestop);
		if(running) pthread_mutex_unlock(&movestop);
		pthread_barrier_wait(&barrier);
		
		//if(thread.id == 1) option->processed++;
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
