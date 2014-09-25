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
#include "main/options.h"
#include "main/msg_phys.h"
#include "physics.h"
#include "physics_n_body.h"

void **nbody_init(struct glob_thread_config *cfg)
{
	struct thread_config_nbody **thread_config = calloc(option->threads+1,
		sizeof(struct thread_config_nbody*));
	for(int k = 0; k < option->threads + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_nbody));
	}
	
	/* Init barrier */
	pthread_barrier_t *barrier = calloc(1, sizeof(pthread_barrier_t));
	pthread_barrier_init(barrier, NULL, option->threads);
	
	int totcore = (int)((float)option->obj/option->threads);
	
	for(int k = 1; k < option->threads + 1; k++) {
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
	
	return (void**)thread_config;
}

void nbody_quit(void **threads)
{
	struct thread_config_nbody **t = (struct thread_config_nbody **)threads;
	pthread_barrier_destroy(t[1]->barrier);
	free(t[1]->barrier);
	for(int k = 0; k < option->threads + 1; k++) {
		free(t[k]);
	}
	free(t);
	return;
}

void *thread_nbody(void *thread_setts)
{
	struct thread_config_nbody *t = thread_setts;
	vec3 vecnorm, accprev;
	double dist;
	float dt = option->dt;
	const double pi = acos(-1);
	const double gconst = option->gconst, epsno = option->epsno;
	const bool nogrv = option->nogrv, noele = option->noele;
	
	while(1) {
		for(unsigned int i = t->objs_low; i < t->objs_high + 1; i++) {
			if(t->obj[i].ignore) continue;
			t->obj[i].pos += (t->obj[i].vel*dt) +\
			(t->obj[i].acc)*((dt*dt)/2);
		}
		
		pthread_barrier_wait(t->barrier);
		
		for(unsigned int i = t->objs_low; i < t->objs_high + 1; i++) {
			accprev = t->obj[i].acc;
			for(unsigned int j = 1; j < option->obj + 1; j++) {
				if(i==j) continue;
				vecnorm = t->obj[j].pos - t->obj[i].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] +\
							vecnorm[1]*vecnorm[1] +\
							vecnorm[2]*vecnorm[2]);
				vecnorm /= dist;
				
				if(!nogrv)
					t->obj[i].acc += vecnorm*\
									   (gconst*t->obj[j].mass)/(dist*dist);
				if(!noele)
					t->obj[i].acc += -vecnorm*\
								((t->obj[i].charge*t->obj[j].charge)/\
									(4*pi*epsno*dist*dist*t->obj[i].mass));
			}
			t->obj[i].vel += (t->obj[i].acc + accprev)*((dt)/2);
		}
		pthread_barrier_wait(t->ctrl);
		
		/* Quit if requested */
		pthread_testcancel();
	}
	return 0;
}
