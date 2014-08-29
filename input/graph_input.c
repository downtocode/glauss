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
#include "graph_input.h"
#include "physics/physics.h"
#include "main/out_xyz.h"
#include "signal.h"
#include "graph/graph.h"
#include "graph/graph_sdl.h"
#include "main/options.h"
#include "main/msg_phys.h"

static void graph_sdl_scan_selection(graph_window *win)
{
	if(win->event->key.keysym.sym!=SDLK_RETURN && win->numbers.final_digit < 19) {
		if(win->event->key.keysym.sym==SDLK_BACKSPACE && win->numbers.final_digit > 0) {
			getnumber(&win->numbers, 0, NUM_REMOVE);
			win->currentsel[strlen(win->currentsel)-1] = '\0';
			return;
		}
		/*	sscanf will return 0 if nothing was done	*/
		if(!sscanf(SDL_GetKeyName(win->event->key.keysym.sym), "%u", &win->currentnum)) {
			return;
		} else {
			strcat(win->currentsel, SDL_GetKeyName(win->event->key.keysym.sym));
			getnumber(&win->numbers, win->currentnum, NUM_ANOTHER);
		}
		return;
	} else {
		strcpy(win->currentsel, "Select object:");
		win->start_selection = 0;
		win->chosen = getnumber(&win->numbers, 0, NUM_GIVEME);
		if(win->chosen > option->obj) {
			win->chosen = 0;
		} else {
			pprintf(PRI_HIGH, "Object %u selected.\n", win->chosen);
			return;
		}
	}
}

static void graph_press_mouse(graph_window *win)
{
	if(win->event->button.button == SDL_BUTTON_LEFT) win->flicked = 1;
	if(win->event->button.button == SDL_BUTTON_MIDDLE) {
		win->chosen = 0;
		win->translate = 1;
	}
	SDL_ShowCursor(0);
	SDL_GetMouseState(&win->initmousex, &win->initmousey);
	SDL_SetRelativeMouseMode(1);
	SDL_GetRelativeMouseState(NULL, NULL);
}


static void graph_release_mouse(graph_window *win)
{
	if(win->event->button.button == SDL_BUTTON_LEFT) win->flicked = 0;
	if(win->event->button.button == SDL_BUTTON_MIDDLE) win->translate = 0;
	if(!win->translate && !win->flicked) {
		SDL_SetRelativeMouseMode(0);
		SDL_WarpMouseInWindow(win->window, win->initmousex, win->initmousey);
		SDL_ShowCursor(1);
	}
}

static void graph_adj_zoom_mwheel(graph_window *win)
{
	if(win->event->wheel.y == 1) win->camera.scalefactor *= 1.11;
	if(win->event->wheel.y == -1) win->camera.scalefactor /= 1.11;
	if(win->camera.scalefactor < 0.005) win->camera.scalefactor = 0.005;
}

static void graph_scan_keypress(graph_window *win)
{
	/* Check if we need to get numbers for object selecting */
	if(win->start_selection) {
		graph_sdl_scan_selection(win);
	}
	/* Scan hotkeys */
	if(win->event->key.keysym.sym==SDLK_RETURN) {
		win->start_selection = 1;
		return;
	}
	if(win->event->key.keysym.sym==SDLK_RIGHTBRACKET) {
		option->dt *= 2;
		printf("dt = %f\n", option->dt);
	}
	if(win->event->key.keysym.sym==SDLK_LEFTBRACKET) {
		option->dt /= 2;
		printf("dt = %f\n", option->dt);
	}
	if(win->event->key.keysym.sym==SDLK_SPACE) {
		threadcontrol(PHYS_PAUSESTART, NULL);
	}
	if(win->event->key.keysym.sym==SDLK_r) {
		win->camera = (struct graph_cam_view)\
		{ 32.0, 315.0, 0, 0, 0, 0, 0.1 };
		win->chosen = 0;
	}
	if(win->event->key.keysym.sym==SDLK_z) {
		toxyz(win->object);
	}
	if(win->event->key.keysym.sym==SDLK_MINUS) {
		phys_shuffle_algorithms();
	}
	if(win->event->key.keysym.sym==SDLK_BACKSPACE) {
		if(option->status) threadcontrol(PHYS_SHUTDOWN, NULL);
		else threadcontrol(PHYS_START, &win->object);
	}
	if(win->event->key.keysym.sym==SDLK_PERIOD) {
		if(win->chosen < option->obj) win->chosen++;
	}
	if(win->event->key.keysym.sym==SDLK_COMMA) {
		if(win->chosen > 0) win->chosen--;
	}
	if(win->event->key.keysym.sym==SDLK_f) {
		if(win->fullscreen) {
			SDL_SetWindowFullscreen(win->window, 0);
			win->fullscreen = 0;
		} else {
			/* Because real fullscreen sucks */
			SDL_SetWindowFullscreen(win->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			win->fullscreen = 1;
		}
		SDL_GL_GetDrawableSize(win->window, &option->width, &option->height);
		graph_resize_wind();
	}
	if(win->event->key.keysym.sym==SDLK_s) {
		graph_sshot(t_stats[1]->progress);
	}
	if(win->event->key.keysym.sym==SDLK_ESCAPE) {
		raise(SIGINT);
	}
	if(win->event->key.keysym.sym==SDLK_q) {
		raise(SIGINT);
	}
}

void graph_sdl_input_main(graph_window *win)
{
	if(!win) return;
	while(SDL_PollEvent(win->event)) {
		switch(win->event->type) {
			case SDL_WINDOWEVENT:
				if(win->event->window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_GL_GetDrawableSize(win->window, &option->width, &option->height);
					graph_resize_wind();
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				graph_press_mouse(win);
				break;
			case SDL_MOUSEBUTTONUP:
				graph_release_mouse(win);
				break;
			case SDL_MOUSEWHEEL:
				graph_adj_zoom_mwheel(win);
				break;
			case SDL_KEYDOWN:
				graph_scan_keypress(win);
				break;
			case SDL_QUIT:
				raise(SIGINT);
				break;
		}
	}
} 
