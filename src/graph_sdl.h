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
#ifndef PHYSENGINE_GRAPH_SDL
#define PHYSENGINE_GRAPH_SDL

/*	Dependencies	*/
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "physics_aux.h"

/* Camera info sent from graph_sdl */
struct graph_cam_view {
	float view_rotx, view_roty, view_rotz;
	float tr_x, tr_y, tr_z;
	float scalefactor;
};

typedef struct {
	/* TODO: Align this mess */
	SDL_Window *window;
	SDL_GLContext context;
	SDL_Event *event;
	bool flicked, translate, fullscreen, start_selection;
	int mousex, mousey, initmousex, initmousey;
	unsigned int chosen, currentnum;
	struct graph_cam_view camera;
	struct numbers_selection numbers;
	char currentsel[100];
	data *object;
	float fps;
} graph_window;

graph_window *graph_sdl_init(data *object);
void graph_sdl_move_cam(graph_window *win);
void graph_sdl_input_main(graph_window *win);
void graph_sdl_swapwin(graph_window *win);
void graph_sdl_deinit(graph_window *win);

/* I don't trust SDL2's signal handling */
int add_to_free_queue(void *p);
int remove_from_free_queue(void *p);
void on_quit_signal(int signo);

#endif
