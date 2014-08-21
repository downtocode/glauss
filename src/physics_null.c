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
	
	/* Init dummy resource */
	volatile long long int *shared_int = calloc(1, sizeof(long long int));
	
	for(int k = 1; k < option->threads + 1; k++) {
		thread_config[k]->ctrl = cfg->ctrl;
		thread_config[k]->mute = mute;
		thread_config[k]->id = k;
		thread_config[k]->shr = shared_int;
	}
	
	return (void**)thread_config;
}

void null_quit(void **threads)
{
	struct thread_config_null **t = (struct thread_config_null **)threads;
	free((void*)t[1]->shr);
	pthread_mutex_destroy(t[1]->mute);
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
	/* We need to play along with the control thread, so continue running. */
	while(1) {
		pthread_mutex_lock(t->mute);
			/* Just something to keep it busy */
			*t->shr += 1;
		pthread_mutex_unlock(t->mute);
		pthread_barrier_wait(t->ctrl);
		pthread_testcancel();
	}
	return 0;
}
