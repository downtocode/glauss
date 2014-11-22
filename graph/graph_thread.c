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
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "graph.h"
#include "graph_sdl.h"
#include "graph_thread.h"
#include "input/parser.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "physics/physics.h"
#include "physics/physics_aux.h"
#include "input/graph_input.h"

struct graph_cfg *global_cfg = NULL;

graph_window **graph_thread_init(data *object)
{
	if (global_cfg) {
		return NULL;
	}
	
	struct graph_cfg *cfg = calloc(1, sizeof(struct graph_cfg));
	
	cfg->obj = object;
	cfg->status = true;
	cfg->selfquit = false;
	
	pthread_create(&cfg->graph, NULL, graph_thread, cfg);
	
	global_cfg = cfg;
	
	return &cfg->win;
}

void graph_thread_quit(void)
{
	if (!global_cfg) {
		return;
	}
	
	global_cfg->status = false;
	
	void *val = NULL, *res = NULL;
	
	while (!global_cfg->selfquit) {
		global_cfg->status = false;
	}
	
	if (!global_cfg->selfquit) {
		pthread_cancel(global_cfg->graph);
		val = PTHREAD_CANCELED;
	}
	
	pthread_join(global_cfg->graph, &res);
	
	if (res != val) {
		pprint_err("Error joining with graphics thread!\n");
	}
	
	graph_sdl_deinit(global_cfg->win);
	
	/* Free resources */
	free(global_cfg);
	
	global_cfg = NULL;
	
	return;
}

void *graph_thread(void *thread_setts)
{
	struct graph_cfg *t = thread_setts;
	
	/* SDL */
	t->win = graph_sdl_init(t->obj);
	graph_sdl_resize_wind(t->win);
	
	/* OpenGL */
	graph_init();
	
	/* Cancel policy */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	/* FPS */
	unsigned int frames = 0;
	unsigned long long int time = 0, t1 = 0, t2 = 0;
	
	t1 = phys_gettime_us();
	
	while (t->status) {
		/* Get input from SDL */
		while (SDL_PollEvent(t->win->event)) {
			switch (t->win->event->type) {
				case SDL_WINDOWEVENT:
					if (t->win->event->window.event == SDL_WINDOWEVENT_RESIZED)
						graph_sdl_resize_wind(t->win);
					break;
				case SDL_MOUSEBUTTONDOWN:
					graph_press_mouse(t->win);
					break;
				case SDL_MOUSEBUTTONUP:
					graph_release_mouse(t->win);
					break;
				case SDL_MOUSEWHEEL:
					graph_adj_zoom_mwheel(t->win);
					break;
				case SDL_KEYDOWN:
					if (graph_scan_keypress(t->win)) {
						t->selfquit = 1;
						raise(SIGINT);
					}
					break;
				case SDL_QUIT:
					t->selfquit = 1;
					raise(SIGINT);
					break;
			}
		}
		
		/* FPS */
		/* Update timer */
		t2 = phys_gettime_us();
		time += t2 - t1;
		if (time > 1.0/1.0e-6) {
			t->win->fps = frames/(time*1.0e-6);
			time = frames = 0;
		}
		t1 = t2;
		frames++;
		
		/* Move camera */
		graph_sdl_move_cam(t->win);
		
		/* Draw scene */
		graph_draw_scene(t->win);
		
		/* Swap Front and Back buffers */
		graph_sdl_swapwin(t->win);
	}
	
	t->selfquit = 1;
	
	return 0;
}
