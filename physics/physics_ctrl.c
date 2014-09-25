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
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "input/parser.h"
#include "main/options.h"
#include "main/out_xyz.h"
#include "physics.h"
#include "physics_ctrl.h"

struct glob_thread_config *ctrl_init(struct glob_thread_config *cfg)
{
	cfg->ctrl = calloc(1, sizeof(pthread_barrier_t));
	pthread_barrier_init(cfg->ctrl, NULL, option->threads+1);
	return cfg;
}

void ctrl_quit(struct glob_thread_config *cfg)
{
	pthread_barrier_destroy(cfg->ctrl);
	free(cfg->ctrl);
	return;
}

void *thread_ctrl(void *thread_setts)
{
	struct glob_thread_config *t = thread_setts;
	unsigned int xyz_counter = 0, sshot_counter = 0, funct_counter = 0;
	
	while(1) {
		/* Pause all threads by stalling unlocking t->ctrl */
		while(option->paused) {
			usleep(100);
		}
		
		pthread_testcancel();
		
		/* Unblock and hope the other threads follow */
		pthread_barrier_wait(t->ctrl);
		
		/* Update progress */
		for(int th = 1; th < option->threads+1; th++) {
			t->stats[th]->progress += option->dt;
		}
		
		/* Dump XYZ file, will not increment timer if !option->dump_xyz */
		if(option->dump_xyz && ++xyz_counter > option->dump_xyz) {
			toxyz(t->obj);
			xyz_counter = 0;
		}
		
		/* Signal to create a screenshot next frame */
		if(option->dump_sshot && ++sshot_counter > option->dump_sshot) {
			option->write_sshot_now = true;
			sshot_counter = 0;
		}
		
		/* Lua function execution */
		if(option->exec_funct_freq && ++funct_counter > option->exec_funct_freq) {
			lua_exec_funct(option->timestep_funct);
			funct_counter = 0;
		}
		
		/* Check if we need to quit */
		pthread_testcancel();
	}
	
	return 0;
}
