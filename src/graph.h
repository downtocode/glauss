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

#include "physics.h"

void graph_view(float view_rotx, float view_roty, float view_rotz, float scalefactor, float tr_x, float tr_y, float tr_z);
void graph_resize_wind();
void graph_draw_scene(data **object, float fps);
unsigned int graph_compile_shader(const char *vertpath, const char *fragpath);
void graph_init();

#endif
