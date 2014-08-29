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
#ifndef PHYSENGINE_GRAPH
#define PHYSENGINE_GRAPH

#include "physics/physics.h"
#include "graph_sdl.h"

/* Identifiers for colors */
enum {
	GL_WHITE,
	GL_RED,
	GL_GREEN,
	GL_BLUE,
	GL_YELLOW,
};

void graph_init();
void graph_quit();
void graph_view(struct graph_cam_view *camera);
float graph_resize_wind();
unsigned int graph_compile_shader(const char *src_vert_shader,
								  const char *src_frag_shader);
void graph_draw_scene(graph_window *win);
int graph_sshot(long double arg);

#endif