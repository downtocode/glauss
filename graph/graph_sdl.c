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
#include <stdbool.h>
/*	Dependencies	*/
#include <SDL2/SDL.h>
#include <signal.h>

#include "main/options.h"
#include "config.h"
#include "main/msg_phys.h"
#include "main/out_xyz.h"
#include "physics/physics.h"
#include "physics/physics_aux.h"
#include "graph.h"
#include "graph_sdl.h"
#include "input/sighandle.h"

graph_window *main_win = NULL;
static bool sdl_initd = NULL;

graph_window *graph_sdl_init(data *object)
{
	graph_window *win = calloc(1, sizeof(graph_window));
	win->event = calloc(1, sizeof(SDL_Event));
	add_to_free_queue(win->event);
	add_to_free_queue(win);
	if(!sdl_initd) {
		SDL_Init(SDL_INIT_VIDEO);
		sdl_initd = true;
		main_win = win;
	}
	
	/* Default options */
	win->camera = (struct graph_cam_view){ 32.0, 315.0, 0, 0, 0, 0, 0.1 };
	win->camera.scalefactor = 0.005;
	win->numbers.final_digit = 0;
	win->object = object;
	strcpy(win->currentsel, "Select object:");
	
	/* Usually helps */
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	/* OGL settings */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_ES);
	/* OGL version */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	/* Spawn window */
	win->window = SDL_CreateWindow(PACKAGE_STRING, \
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, option->width,
		option->height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|\
		SDL_WINDOW_ALLOW_HIGHDPI);
	/* Create GL context */
	win->context = SDL_GL_CreateContext(win->window);
	return win;
}

void graph_sdl_move_cam(graph_window *win)
{
	if(!win) return;
	if(win->flicked || win->translate) {
		SDL_GetRelativeMouseState(&win->mousex, &win->mousey);
		if(win->flicked) {
			win->camera.view_roty += (float)win->mousex/4;
			win->camera.view_rotx += (float)win->mousey/4;
		}
		if(win->translate) {
			win->camera.tr_x += -(float)win->mousex/100;
			win->camera.tr_y += (float)win->mousey/100;
		}
	}
	if(win->chosen) {
		win->camera.tr_x = win->object[win->chosen].pos[0];
		win->camera.tr_y = win->object[win->chosen].pos[1];
		win->camera.tr_z = win->object[win->chosen].pos[2];
	}
	graph_view(&win->camera);
}

void graph_sdl_swapwin(graph_window *win)
{
	SDL_GL_SwapWindow(win->window);
}

void graph_sdl_deinit(graph_window *win) {
	SDL_GL_DeleteContext(win->context);
	SDL_DestroyWindow(win->window);
	if(main_win == win)
		SDL_Quit();
	free(win->event);
	free(win);
}
