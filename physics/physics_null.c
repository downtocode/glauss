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
#include "main/options.h"

void *null_preinit(struct glob_thread_config *cfg)
{
	cfg->total_syncd_threads = 1;
	return NULL;
}

void **null_init(struct glob_thread_config *cfg)
{
	struct thread_config_null **thread_config = calloc(option->threads+1,
		sizeof(struct thread_config_nbody*));
	for (int k = 0; k < option->threads + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_null));
	}
	
	/* Init mutex */
	pthread_mutex_t *mute = calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(mute, NULL);
	
	int totcore = (int)((float)option->obj/option->threads);
	
	for (int k = 1; k < option->threads + 1; k++) {
		thread_config[k]->glob_stats = cfg->stats;
		thread_config[k]->stats = &cfg->stats->t_stats[k];
		thread_config[k]->ctrl = cfg->ctrl;
		thread_config[k]->mute = mute;
		thread_config[k]->id = k;
		thread_config[k]->quit = cfg->quit;
		thread_config[k]->obj = cfg->obj;
		thread_config[k]->objs_low = thread_config[k-1]->objs_high + 1;
		thread_config[k]->objs_high = thread_config[k]->objs_low + totcore - 1;
		if (k == option->threads) {
			/*	Takes care of rounding problems with odd numbers.	*/
			thread_config[k]->objs_high += option->obj - thread_config[k]->objs_high;
		}
	}
	
	option->stats_null = true;
	
	return (void**)thread_config;
}

void null_quit(void **threads)
{
	struct thread_config_null **t = (struct thread_config_null **)threads;
	option->stats_null = false;
	pthread_mutex_destroy(t[1]->mute);
	free(t[1]->mute);
	for (int k = 0; k < option->threads + 1; k++) {
		free(t[k]);
	}
	free(t);
	return;
}

void *thread_stats(void *thread_setts)
{
	struct thread_config_null *t = thread_setts;
	/* We need to play along with the control thread, so continue running. */
	while (!*t->quit) {
		vec3 vecnorm = (vec3){0};
		double dist = 0.0, avg_dist = 0.0, max_dist = 0.0;
		for (unsigned int i = t->objs_low; i < t->objs_high + 1; i++) {
			for (unsigned int j = 1; j < option->obj + 1; j++) {
				if (i==j)
					continue;
				vecnorm = t->obj[j].pos - t->obj[i].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] +\
							vecnorm[1]*vecnorm[1] +\
							vecnorm[2]*vecnorm[2]);
				max_dist = fmax(max_dist, dist);
				avg_dist = (dist + avg_dist)/2;
			}
		}
		
		t->stats->null_avg_dist = avg_dist;
		t->stats->null_max_dist = max_dist;
		
		pthread_barrier_wait(t->ctrl);
		
		/* Racy as hell */
		pthread_mutex_lock(t->mute);
			t->glob_stats->null_avg_dist = (avg_dist+t->glob_stats->null_avg_dist)/2;
			t->glob_stats->null_max_dist = fmax(t->glob_stats->null_max_dist, max_dist);
		pthread_mutex_unlock(t->mute);
	}
	return 0;
}

void *thread_null(void *thread_setts)
{
	return 0;
}
