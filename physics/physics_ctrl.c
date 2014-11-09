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
#include "input/input_thread.h"
#include "main/options.h"
#include "main/output.h"
#include "physics.h"
#include "physics_ctrl.h"

struct glob_thread_config *ctrl_init(struct glob_thread_config *cfg)
{
	cfg->threads = calloc(option->threads+1, sizeof(pthread_t));
	
	cfg->ctrl = calloc(1, sizeof(pthread_barrier_t));
	pthread_barrier_init(cfg->ctrl, NULL, cfg->total_syncd_threads);
	
	cfg->io_halt = calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(cfg->io_halt, NULL);
	
	cfg->quit = calloc(1, sizeof(bool));
	cfg->pause = calloc(1, sizeof(bool));
	
	return cfg;
}

void ctrl_quit(struct glob_thread_config *cfg)
{
	pthread_barrier_destroy(cfg->ctrl);
	pthread_mutex_destroy(cfg->io_halt);
	
	free(cfg->io_halt);
	free(cfg->ctrl);
	free(cfg->quit);
	free(cfg->pause);
	free(cfg);
	return;
}

void *thread_ctrl(void *thread_setts)
{
	struct glob_thread_config *t = thread_setts;
	const double dt = option->dt;
	unsigned int xyz_counter = 0, sshot_counter = 0, funct_counter = 0, stats_counter = 0;
	unsigned int thread_fn_counter = 0;
	long long unsigned int t1 = phys_gettime_us(), t2 = 0;
	
	while (!*t->quit) {
		/* Pause all threads by stalling unlocking t->ctrl */
		while (*t->pause) {
			usleep(25);
		}
		
		/* Check if someone else needs to have IO */
		while (pthread_mutex_trylock(t->io_halt)) {
			*t->pause = true;
			usleep(25);
			*t->pause = false;
		}
		
		/* Update progress */
		t->stats->steps++;
		t2 = phys_gettime_us();
		t->stats->time_per_step = (t2 - t1)*1.0e-6;
		t1 = t2;
		t->stats->progress += dt;
		
		if (t->thread_sched_fn) {
			if (t->thread_sched_fn_freq && ++thread_fn_counter >= t->thread_sched_fn_freq) {
				t->thread_sched_fn(t->threads_conf);
				thread_fn_counter = 0;
			}
		}
		
		/* Dump XYZ file, will not increment timer if !option->dump_xyz */
		if (option->dump_xyz && ++xyz_counter >= option->dump_xyz) {
			out_write_xyz(t->obj, option->xyz_temp, NULL);
			xyz_counter = 0;
		}
		
		/* Signal to create a screenshot next frame */
		if (option->dump_sshot && ++sshot_counter >= option->dump_sshot) {
			option->write_sshot_now = true;
			sshot_counter = 0;
		}
		
		/* Lua function execution */
		if (option->exec_funct_freq && ++funct_counter >= option->exec_funct_freq) {
			lua_exec_funct(option->timestep_funct, t->obj);
			funct_counter = 0;
		}
		
		/* Reset some stats */
		if (option->reset_stats_freq && ++stats_counter >= option->reset_stats_freq) {
			if (option->stats_null) {
				t->stats->null_max_dist = 0;
				t->stats->null_avg_dist = 0;
			}
			if (option->stats_bh) {
				t->stats->bh_total_alloc = 0;
				t->stats->bh_new_alloc = 0;
				t->stats->bh_new_cleaned = 0;
				t->stats->bh_heapsize = 0;
			}
		}
		
		/* Unlock IO mutex */
		pthread_mutex_unlock(t->io_halt);
		
		/* Unblock and hope the other threads follow */
		pthread_barrier_wait(t->ctrl);
	}
	
	return 0;
}
