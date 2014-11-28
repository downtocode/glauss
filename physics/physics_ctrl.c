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
#include "physics_aux.h"
#include "physics_ctrl.h"
#include "main/msg_phys.h"

struct glob_thread_config *ctrl_preinit(struct global_statistics *stats, phys_obj *obj)
{
	struct glob_thread_config *cfg = calloc(1, sizeof(struct glob_thread_config));
	
	cfg->total_syncd_threads = option->threads + 1;
	cfg->obj = obj;
	cfg->stats = stats;
	
	cfg->algo_thread_stats_map = calloc(option->threads+1, sizeof(struct parser_map *));
	
	/* Reinit stats */
	cfg->stats->t_stats = calloc(option->threads+1, sizeof(struct thread_statistics));
	
	for (int k = 0; k < option->threads; k++) {
		cfg->stats->t_stats[k].thread_stats_map = \
			allocate_parser_map((struct parser_map []){
				{"clockid",   P_TYPE(cfg->stats->t_stats[k].clockid)   },
				{0},
			});
	}
	
	return cfg;
}

struct glob_thread_config *ctrl_init(struct glob_thread_config *cfg)
{
	/* Transfer global stat map */
	if (cfg->algo_global_stats_map) {
		register_parser_map(cfg->algo_global_stats_map,
							&cfg->stats->global_stats_map);
	}
	
	/* Transfer thread stat map */
	if (cfg->algo_thread_stats_map) {
		for (int k = 0; k < option->threads; k++) {
			register_parser_map(cfg->algo_thread_stats_map[k],
								&cfg->stats->t_stats[k].thread_stats_map);
		}
	}
	
	cfg->threads = calloc(option->threads, sizeof(pthread_t));
	
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
	if (cfg->algo_global_stats_map) {
		unregister_parser_map(cfg->algo_global_stats_map, &cfg->stats->global_stats_map);
		/* Free all maps set by any threads */
		free(cfg->algo_global_stats_map);
	}
	
	if (cfg->algo_thread_stats_map) {
		for (int k = 0; k < option->threads; k++) {
			free(cfg->algo_thread_stats_map[k]);
			free(cfg->stats->t_stats[k].thread_stats_map);
		}
		free(cfg->algo_thread_stats_map);
	}
	free(cfg->stats->t_stats);
	cfg->stats->t_stats = NULL;
	
	/* Free global sync barrier */
	pthread_barrier_destroy(cfg->ctrl);
	free(cfg->ctrl);
	
	/* Free IO mutex */
	pthread_mutex_destroy(cfg->io_halt);
	free(cfg->io_halt);
	
	/* Free anything else */
	free((void *)cfg->quit);
	free((void *)cfg->pause);
	free(cfg);
	return;
}

void *thread_ctrl(void *thread_setts)
{
	struct glob_thread_config *t = thread_setts;
	const double dt = option->dt;
	unsigned int xyz_counter = 0, sshot_counter = 0, funct_counter = 0;
	unsigned int thread_fn_counter = 0;
	long long unsigned int t1 = phys_gettime_us(), t2 = 0;
	
	while (!*t->quit) {
		/* Pause all threads by stalling unlocking t->ctrl */
		while (*t->pause) {
			phys_sleep_msec(50);
		}
		
		/* Check if someone else needs to have IO */
		while (pthread_mutex_trylock(t->io_halt)) {
			*t->pause = true;
			phys_sleep_msec(50);
			*t->pause = false;
		}
		
		/* Update progress */
		t->stats->total_steps++;
		t2 = phys_gettime_us();
		t->stats->time_per_step = (t2 - t1)*1.0e-6;
		t1 = t2;
		t->stats->progress += dt;
		
		/* Dump XYZ file, will not increment timer if !option->dump_xyz */
		if (phys_timer_exec(option->dump_xyz, &xyz_counter)) {
			out_write_xyz(t->obj, option->xyz_temp, NULL);
		}
		
		/* Signal to create a screenshot next frame */
		if (phys_timer_exec(option->dump_sshot, &sshot_counter)) {
			option->write_sshot_now = true;
		}
		
		/* Lua function execution */
		if (phys_timer_exec(option->exec_funct_freq, &funct_counter)) {
			lua_exec_funct(option->timestep_funct, t->obj, t->stats);
		}
		
		if (t->thread_sched_fn && phys_timer_exec(t->thread_sched_fn_freq,
												  &thread_fn_counter)) {
			t->thread_sched_fn(t->threads_conf);
		}
		
		/* Unlock IO mutex */
		pthread_mutex_unlock(t->io_halt);
		
		/* Unblock and hope the other threads follow */
		pthread_barrier_wait(t->ctrl);
	}
	
	return 0;
}
