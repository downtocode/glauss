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

int nbody_distribute(data** object, struct thread_config_nbody *thread_opts_nbody)
{
	unsigned int *limits_up = calloc(option->avail_cores+1, sizeof(unsigned int));
	unsigned int *limits_down = calloc(option->avail_cores+1, sizeof(unsigned int));
	
	int totcore = (int)((float)option->obj/option->avail_cores);
	for(int k = 1; k < option->avail_cores + 1; k++) {
		limits_down[k] = limits_up[k-1] + 1;
		limits_up[k] = limits_down[k] + totcore - 1;
		if(k == option->avail_cores) {
			/*	Takes care of rounding problems with odd numbers.	*/
			limits_up[k] += option->obj - limits_up[k];
		}
		thread_opts_nbody[k].objcount = limits_up[k] - limits_down[k];
		thread_opts_nbody[k].indices = calloc(thread_opts_nbody[k].objcount+1, sizeof(unsigned int));
		
		for(int i = 0; i < thread_opts_nbody[k].objcount + 1; i++) {
			thread_opts_nbody[k].indices[i] = limits_down[k];
			limits_down[k]++;
		}
	}
	free(limits_up);
	free(limits_down);
	return 0;
}

void *thread_nbody(void *thread_setts)
{
	struct thread_config_nbody thread = *((struct thread_config_nbody *)thread_setts);
	v4sd vecnorm, accprev;
	double dist;
	const double pi = acos(-1);
	const long double gconst = option->gconst, epsno = option->epsno;
	const bool nogrv = option->nogrv, noele = option->noele, noflj = option->noflj;
	
	while(!quit) {
		for(int i = 0; i < thread.objcount + 1; i++) {
			if(thread.obj[thread.indices[i]].ignore) continue;
			thread.obj[thread.indices[i]].pos += (thread.obj[thread.indices[i]].vel*option->dt) +\
			(thread.obj[thread.indices[i]].acc)*((option->dt*option->dt)/2);
		}
		
		if(!running) pthread_cond_wait(&thr_stop, &movestop);
		pthread_barrier_wait(&barrier);
		
		for(int i = 0; i < thread.objcount + 1; i++) {
			accprev = thread.obj[thread.indices[i]].acc;
			for(int j = 1; j < option->obj + 1; j++) {
				if(thread.indices[i]==j) continue;
				vecnorm = thread.obj[j].pos - thread.obj[thread.indices[i]].pos;
				dist = sqrt(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1] + vecnorm[2]*vecnorm[2]);
				vecnorm /= dist;
				
				if(!nogrv)
					thread.obj[thread.indices[i]].acc += vecnorm*(double)(gconst*thread.obj[j].mass)/(dist*dist);
				if(!noele)
					thread.obj[thread.indices[i]].acc += -vecnorm*(double)((thread.obj[thread.indices[i]].charge*\
					thread.obj[j].charge)/(4*pi*epsno*dist*dist*thread.obj[thread.indices[i]].mass));
				if(!noflj)
					thread.obj[thread.indices[i]].acc += vecnorm*(4*epsilon*(12*(pow(sigma, 12)/pow(dist, 13)) -\
					6*(pow(sigma, 6)/pow(dist, 7)))/thread.obj[thread.indices[i]].mass);
			}
			thread.obj[thread.indices[i]].vel += (thread.obj[thread.indices[i]].acc + accprev)*((option->dt)/2);
		}
		if(thread.id == 1) option->processed++;
		pthread_barrier_wait(&barrier);
	}
	return 0;
}
