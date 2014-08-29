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
#include <tgmath.h>
#include <GLES2/gl2.h>
#include "physics/physics.h"
#include "physics/physics_aux.h"
#include "graph/graph.h"
#include "main/options.h"
#include "graph_objects.h"

static GLint objattr_pos, objattr_color;

/* Drawing points defaults to color = black, so we need white */
static const GLfloat white[] = {1.0, 1.0, 1.0, 1.0};

void draw_obj_axis(float scale)
{
	GLfloat axis[6][3] = {
		{0    ,0    ,0    },
		{scale,0    ,0    },
		{0    ,0    ,0    },
		{0    ,scale,0    },
		{0    ,0    ,0    },
		{0    ,0    ,scale},
	};
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(objattr_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis), axis, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_LOOP, 0, 6);
	glDisableVertexAttribArray(objattr_pos);
}

void draw_obj_sphere(data* object)
{
	const GLfloat white[] = {1.0, 1.0, 1.0, 1.0};
	/* Decrease dj to get better spheres. */
	float dj = 0.5, pi=acos(-1);
	int pointcount = 0;
	float points[(int)((pi/dj)*((2*pi/dj)+1)+30)][3];
	
	glUniform4fv(objattr_color, 1, atom_prop[object->atomnumber].color);
	
	for(float i = 0; i < pi; i+=dj) {
		for(float j = 0; j < 2*pi; j+=dj) {
			points[pointcount][0] = object->pos[0] +\
												   object->radius*sin(i)*cos(j);
			points[pointcount][1] = object->pos[1] +\
												   object->radius*sin(i)*sin(j);
			points[pointcount][2] = object->pos[2] +\
												   object->radius*cos(i);
			pointcount++;
		}
	}
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(objattr_color, 4, GL_FLOAT, GL_FALSE, 0,
						  atom_prop[object->atomnumber].color);
	glEnableVertexAttribArray(objattr_pos);
	glEnableVertexAttribArray(objattr_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_DYNAMIC_DRAW);
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, pointcount);
	
	glDisableVertexAttribArray(objattr_pos);
	glDisableVertexAttribArray(objattr_color);
	glUniform4fv(objattr_color, 1, white);
}

void draw_obj_points(data* object)
{
	float points[option->obj][3];
	
	glUniform4fv(objattr_color, 1, white);
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(objattr_pos);
	
	for(int i = 1; i < option->obj+1; i++) {
		points[i-1][0] = object[i].pos[0];
		points[i-1][1] = object[i].pos[1];
		points[i-1][2] = object[i].pos[2];
	}
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_DYNAMIC_DRAW);
	
	glDrawArrays(GL_POINTS, 0, option->obj);
	
	glDisableVertexAttribArray(objattr_pos);
}

static const char object_vs[] =
// Generated from object_vs.glsl
#include "src/shaders/object_vs.h"
;

static const char object_fs[] =
// Generated from object_fs.glsl
#include "src/shaders/object_fs.h"
;

GLuint graph_init_objects()
{
	GLuint obj_program = graph_compile_shader(object_vs, object_fs);
	glBindAttribLocation(obj_program, objattr_pos, "pos");
	glBindAttribLocation(obj_program, objattr_color, "objcolor");
	objattr_color = glGetUniformLocation(obj_program, "objcolor");
	return obj_program;
}
