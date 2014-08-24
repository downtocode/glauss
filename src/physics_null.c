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
#include <unistd.h>
#include <stdio.h>
#include <complex.h>
#include <tgmath.h>
#include <stdlib.h>
#include <pthread.h>
#include "physics.h"
#include "physics_null.h"
#include "options.h"

void **null_init(struct glob_thread_config *cfg)
{
	struct thread_config_null **thread_config = calloc(option->threads+1,
		sizeof(struct thread_config_nbody*));
	for(int k = 0; k < option->threads + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_null));
	}
	
	/* Init mutex */
	pthread_mutex_t *mute = calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(mute, NULL);
	
	double *dists = malloc(option->obj*option->obj*sizeof(double));
	double *maxdist = malloc(sizeof(double));
	
	int totcore = (int)((float)option->obj/option->threads);
	
	for(int k = 1; k < option->threads + 1; k++) {
		thread_config[k]->ctrl = cfg->ctrl;
		thread_config[k]->mute = mute;
		thread_config[k]->dists = dists;
		thread_config[k]->maxdist = maxdist;
		thread_config[k]->id = k;
		thread_config[k]->obj = cfg->obj;
		thread_config[k]->objs_low = thread_config[k-1]->objs_high + 1;
		thread_config[k]->objs_high = thread_config[k]->objs_low + totcore - 1;
		if(k == option->threads) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_config[k]->objs_high += option->obj - thread_config[k]->objs_high;
		}
	}
	
	return (void**)thread_config;
}

void null_quit(void **threads)
{
	struct thread_config_null **t = (struct thread_config_null **)threads;
	pthread_mutex_destroy(t[1]->mute);
	free(t[1]->dists);
	free(t[1]->maxdist);
	free(t[1]->mute);
	for(int k = 0; k < option->threads + 1; k++) {
		free(t[k]);
	}
	free(t);
	return;
}

void *thread_null(void *thread_setts)
{
	struct thread_config_null *t = thread_setts;
	vec3 vecnorm;
	double dist;
	/* We need to play along with the control thread, so continue running. */
	while(1) {
		for(unsigned int i = t->objs_low; i < t->objs_high + 1; i++) {
			for(unsigned int j = 1; j < option->obj + 1; j++) {
				if(i==j) continue;
				vecnorm = t->obj[j].pos - t->obj[i].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] +\
							vecnorm[1]*vecnorm[1] +\
							vecnorm[2]*vecnorm[2]);
				if(dist > *t->maxdist) {
					pthread_mutex_lock(t->mute);
					*t->maxdist = dist;
					pthread_mutex_unlock(t->mute);
				}
				t->dists[j + (i-1)*option->obj] = dist;
			}
		}
		pthread_barrier_wait(t->ctrl);
		pthread_testcancel();
	}
	return 0;
}
