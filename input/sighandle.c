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
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "physics/physics.h"
#include "parser.h"
#include "sighandle.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "graph/graph_thread.h"
#include "input_thread.h"

/* Report stats on command line */
void on_usr1_signal(int signo)
{
	pprintf(PRI_ESSENTIAL, "\n");
	if(!signo)
		pprintf(PRI_ESSENTIAL, "USR1 signal received, current stats:\n");
	pprintf(PRI_ESSENTIAL, "Time running = %Lf\n", phys_stats->time_running);
	pprintf(PRI_ESSENTIAL, "Progress = %Lf\n", phys_stats->progress);
	if(phys_ctrl(PHYS_STATUS, NULL) == PHYS_STATUS_RUNNING) {
		pprintf(PRI_ESSENTIAL, "CPU time:\n Thread |  Time\n");
		struct timespec ts;
		for(int i = 1; i < option->threads + 1; i++) {
			clock_gettime(phys_stats->t_stats[i].clockid, &ts);
			pprintf(PRI_ESSENTIAL, "   %02i   |  ", i);
			pprintf(PRI_ESSENTIAL, "%ld.%ld\n",  ts.tv_sec, ts.tv_nsec / 1000000);
		}
	}
	if(option->stats_bh) {
		pprintf(PRI_ESSENTIAL, "BH Tree stats:\n Thread |  Total   New  Cleaned    Size\n");
		for(int i = 1; i < option->threads + 1; i++) {
			pprintf(PRI_ESSENTIAL, "   %02i   |  ", i);
			pprintf(PRI_ESSENTIAL, "%u    %u    %u       %lu\n", phys_stats->t_stats[i].bh_total_alloc,
				   phys_stats->t_stats[i].bh_new_alloc, phys_stats->t_stats[i].bh_new_cleaned,
				   phys_stats->t_stats[i].bh_heapsize);
		}
		pprintf(PRI_ESSENTIAL, "Glob: %u    %u    %u       %0.3lf(MiB)\n", phys_stats->bh_total_alloc,
			   phys_stats->bh_new_alloc, phys_stats->bh_new_cleaned, phys_stats->bh_heapsize/1048576.0);
	}
	if(option->stats_null) {
		pprintf(PRI_ESSENTIAL, "Null stats:\n Thread |  Avg dist   Max dist\n");
		for(int i = 1; i < option->threads + 1; i++) {
			pprintf(PRI_ESSENTIAL, "   %02i   |  ", i);
			pprintf(PRI_ESSENTIAL, "%lf    %lf\n", phys_stats->t_stats[i].null_avg_dist,
				   phys_stats->t_stats[i].null_max_dist);
		}
		pprintf(PRI_ESSENTIAL, "Glob: %lf    %lf\n", phys_stats->null_avg_dist, phys_stats->null_max_dist);
	}
}

/* Function given to signal handler */
void on_quit_signal(int signo)
{
	pprintf(PRI_ESSENTIAL, "\nSignal to quit %i received!\n", signo);
	
	/* Physics */
	phys_ctrl(PHYS_SHUTDOWN, NULL);
	
	pprintf(PRI_ESSENTIAL, "Closing: ");
	
	/* Lua */
	pprintf(PRI_ESSENTIAL, "Lua: ");
	parse_lua_close();
	free_input_parse_opts();
	pprintf(PRI_OK, "");
	
	/* Logfile */
	if(option->logenable) {
		pprintf(PRI_ESSENTIAL, "&& Log: ");
		fclose(option->logfile);
		option->logenable = false;
		pprintf(PRI_OK, "");
	}
	
	/* Input thread */
	if(option->input_thread_enable) {
		pprintf(PRI_ESSENTIAL, "&& Input: ");
		input_thread_quit();
		pprintf(PRI_OK, "");
	}
	
	/* Window */
	if(!option->novid) {
		pprintf(PRI_ESSENTIAL, "&& Window: ");
		graph_thread_quit();
		option->novid = true;
		pprintf(PRI_OK, "");
	}
	
	/* Main */
	option->quit_main_now = true;
}

void on_alrm_signal(int signo)
{
	pprint_warn("Watchdog timer kicked in! Program probably hanged up.\n");
}
