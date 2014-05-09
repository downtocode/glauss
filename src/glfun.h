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

#include <GLES2/gl2.h>
#include <ft2build.h>
#include "physics.h"
#include FT_FREETYPE_H

#define GL_WHITE 0
#define GL_RED 1
#define GL_GREEN 2
#define GL_BLUE 3

FT_Library library;
FT_Face face;
FT_GlyphSlot g;

void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b);
void adjust_rot(GLfloat view_rotx, GLfloat view_roty, GLfloat view_rotz, \
				GLfloat scalefactor, GLfloat tr_x, GLfloat tr_y, GLfloat tr_z);

void drawaxis();
void draw_obj_sphere(data object);
void draw_obj_points(data* object);
unsigned int createlinks(data* object, GLfloat (**links)[3]);
void draw_obj_links(GLfloat (**links)[3], unsigned int linkcount);
void render_text(const char *text, float x, float y, float sx, float sy, unsigned int col);
void selected_box_text(data object);

int resize_wind();
void create_shaders(GLuint** shaderprogs);

#endif
