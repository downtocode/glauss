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
#include "options.h"
#include "config.h"
#include "graph_sdl.h"

struct graph_window *graph_sdl_init(bool no_win)
{
	struct graph_window *win = calloc(1, sizeof(struct graph_window));
	win->event = calloc(1, sizeof(SDL_Event));
	if(no_win) return win;
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

void graph_sdl_deinit(struct graph_window *win, bool no_win) {
	if(no_win) goto free;
	SDL_GL_DeleteContext(win->context);
	SDL_DestroyWindow(win->window);
	SDL_Quit();
	free:
		free(win->event);
		free(win);
}
