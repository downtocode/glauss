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
#include "physics_n_body.h"

void** nbody_init(data** object, struct thread_statistics **stats)
{
	struct thread_config_nbody **thread_config = calloc(option->avail_cores+1, sizeof(struct thread_config_nbody*));
	for(int k = 0; k < option->avail_cores + 1; k++) {
		thread_config[k] = calloc(1, sizeof(struct thread_config_nbody));
	}
	
	int totcore = (int)((float)option->obj/option->avail_cores);
	
	for(int k = 1; k < option->avail_cores + 1; k++) {
		thread_config[k]->stats = stats[k];
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

void *thread_nbody(void *thread_setts)
{
	struct thread_config_nbody *thread = (struct thread_config_nbody *)thread_setts;
	v4sd vecnorm, accprev;
	double dist;
	const double pi = acos(-1);
	const long double gconst = option->gconst, epsno = option->epsno;
	const bool nogrv = option->nogrv, noele = option->noele, noflj = option->noflj;
	
	while(!quit) {
		for(int i = thread->objs_low; i < thread->objs_high + 1; i++) {
			if(thread->obj[i].ignore) continue;
			thread->obj[i].pos += (thread->obj[i].vel*option->dt) +\
			(thread->obj[i].acc)*((option->dt*option->dt)/2);
		}
		
		pthread_barrier_wait(&barrier);
		
		for(int i = thread->objs_low; i < thread->objs_high + 1; i++) {
			accprev = thread->obj[i].acc;
			for(int j = 1; j < option->obj + 1; j++) {
				if(i==j) continue;
				vecnorm = thread->obj[j].pos - thread->obj[i].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
				vecnorm /= dist;
				
				if(!nogrv)
					thread->obj[i].acc += vecnorm*(double)(gconst*thread->obj[j].mass)/(dist*dist);
				if(!noele)
					thread->obj[i].acc += -vecnorm*(double)((thread->obj[i].charge*\
					thread->obj[j].charge)/(4*pi*epsno*dist*dist*thread->obj[i].mass));
				if(!noflj)
					thread->obj[i].acc += vecnorm*(4*epsilon*(12*(pow(sigma, 12)/pow(dist, 13)) -\
					6*(pow(sigma, 6)/pow(dist, 7)))/thread->obj[i].mass);
			}
			thread->obj[i].vel += (thread->obj[i].acc + accprev)*((option->dt)/2);
		}
		thread->stats->progress += option->dt;
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
