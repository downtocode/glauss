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
#ifdef PHYSENGINE_GRAPH

#include "physics.h"

void draw_obj_axis();
void draw_obj_sphere(data object);
void draw_obj_points(data* object);
unsigned int createlinks(data* object, GLfloat (**links)[3]);
void draw_obj_links(GLfloat (**links)[3], unsigned int linkcount);
GLuint graph_init_objects();

#endif