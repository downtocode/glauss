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
#include <SDL2/SDL.h>
#include <stdbool.h>

struct graph_window {
	SDL_Window *window;
	SDL_GLContext context;
	SDL_Event *event;
};

struct graph_window *graph_sdl_init(bool no_win);
void graph_sdl_deinit(struct graph_window *win, bool no_win);

#endif
