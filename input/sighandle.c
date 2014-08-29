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
#include "main/options.h"
#include "main/msg_phys.h"
#include "physics/physics.h"
#include "graph/graph.h"

#define FREE_QUEUE_MAX 20
static void *to_be_freed[FREE_QUEUE_MAX] = {NULL};

int add_to_free_queue(void *p)
{
	for(int i = 0; i < FREE_QUEUE_MAX; i++) {
		if(!to_be_freed[i]) {
			to_be_freed[i] = p;
			return 0;
		}
	}
	return 1;
}

int remove_from_free_queue(void *p)
{
	for(int i = 0; i < FREE_QUEUE_MAX; i++) {
		if(to_be_freed[i] == p) {
			to_be_freed[i] = NULL;
			return 0;
		}
	}
	return 1;
}

/* Report stats on command line */
void on_usr1_signal(int signo)
{
	printf("\n");
	if(!signo) printf("USR1 signal received, current stats:\n");
	printf("Progress = %Lf\n", t_stats[1]->progress);
	if(option->stats_bh) {
		printf("BH Tree stats:\n Thread |  Total   New  Cleaned    Size\n");
		for(int i = 1; i < option->threads + 1; i++) {
			printf("   %02i   |  ", i);
			printf("%u    %u    %u       %lu\n", t_stats[i]->bh_total_alloc,
				   t_stats[i]->bh_new_alloc, t_stats[i]->bh_new_cleaned,
		  t_stats[i]->bh_heapsize);
		}
	}
}

/* Function given to signal handler */
void on_quit_signal(int signo)
{
	printf("\nSignal to quit %i received!\n", signo);
	threadcontrol(PHYS_SHUTDOWN, NULL);
	if(option->logenable)
		fclose(option->logfile);
	graph_quit();
	for(int i = 0; i < FREE_QUEUE_MAX; i++) {
		free(to_be_freed[i]);
	}
	exit(0);
}
