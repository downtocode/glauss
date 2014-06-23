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
#include <string.h>
#include <tgmath.h>
#include <GLES2/gl2.h>
#include "physics.h"
#include "msg_phys.h"
#include "parser.h"
#include "graph.h"
#include "graph_objects.h"
#include "graph_fonts.h"
#include "options.h"

#define GL_WHITE 0
#define GL_RED 1
#define GL_GREEN 2
#define GL_BLUE 3

static float aspect_ratio;
static GLuint pointvbo, textvbo;
static GLuint object_shader, text_shader;
static GLint trn_matrix, rot_matrix, scl_matrix, per_matrix;
static GLfloat *mat, *rotx, *roty, *rotz, *rotation, *scale, *transl, *pers;

static void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b)
{
	#define A(row,col)  a[(col<<2)+row]
	#define B(row,col)  b[(col<<2)+row]
	#define P(row,col)  p[(col<<2)+row]
	GLfloat p[16];
	for(int i = 0; i < 4; i++) {
		const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
		P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
	memcpy(prod, p, sizeof(p));
	#undef A
	#undef B
	#undef PROD
}

static void make_z_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[10] = m[15] = 1;
	m[0] = c;
	m[1] = s;
	m[4] = -s;
	m[5] = c;
}

static void make_x_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[0] = m[15] = 1;
	m[5] = c;
	m[6] = s;
	m[9] = -s;
	m[10] = c;
}

static void make_y_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[5] = m[15] = 1;
	m[0] = c;
	m[2] = -s;
	m[8] = s;
	m[10] = c;
}

static void make_translation_matrix(GLfloat xpos, GLfloat ypos, GLfloat zpos, GLfloat *m)
{
	m[0] = 1;
	m[3] = xpos;
	m[5] = 1;
	m[7] = ypos;
	m[10] = 1;
	m[11] = zpos;
	m[15] = 1;
}

static void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m)
{
	m[0] = xs;
	m[5] = ys;
	m[10] = zs;
	m[15] = 1;
}

static void make_pers_matrix(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far, GLfloat *m) {
	float D2R = acos(-1)/180.0;
	float yScale = 1.0/tan(D2R * fov / 2);
	float xScale = yScale/aspect;
	float nearmfar = near - far;
	m[0] = xScale;
	m[5] = yScale;
	m[11] = (far + near) / nearmfar;
	m[12] = -1;
	m[15] = 2*far*near / nearmfar;
}

void graph_view(float view_rotx, float view_roty, float view_rotz, float scalefactor, float tr_x, float tr_y, float tr_z)
{
	make_translation_matrix(tr_x, tr_y, tr_z, transl);
	
	make_scale_matrix(aspect_ratio*scalefactor, scalefactor, scalefactor, scale);
	
	make_pers_matrix(10, option->width/option->height, -1, 10, pers);
	
	make_x_rot_matrix(view_rotx, rotx);
	make_y_rot_matrix(view_roty, roty);
	make_z_rot_matrix(view_rotz, rotz);
	mul_matrix(mat, roty, rotx);
	mul_matrix(rotation, mat, rotz);
	glUniformMatrix4fv(trn_matrix, 1, GL_FALSE, transl);
	glUniformMatrix4fv(scl_matrix, 1, GL_FALSE, scale);
	glUniformMatrix4fv(rot_matrix, 1, GL_FALSE, rotation);
	glUniformMatrix4fv(per_matrix, 1, GL_FALSE, pers);
}

void graph_resize_wind()
{
	/* Usually it's the other way around */
	aspect_ratio = (float)option->height/option->width;
	glViewport(0, 0, option->width, option->height);
}

unsigned int graph_compile_shader(const char *vertpath, const char *fragpath)
{
	GLint status_vert, status_frag;
	GLuint program = glCreateProgram();
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *src_vert_shader = readshader(vertpath);
	const char *src_frag_shader = readshader(fragpath);
	
	glShaderSource(vert_shader, 1, &src_vert_shader, NULL);
	glShaderSource(frag_shader, 1, &src_frag_shader, NULL);
	glCompileShader(vert_shader);
	glCompileShader(frag_shader);
	glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &status_vert);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status_frag);
	if(!status_vert || !status_frag) {
		if(!status_vert) pprintf(PRI_ERR, "VS %s did not compile.\n", vertpath);
		if(!status_frag) pprintf(PRI_ERR, "FS %s did not compile.\n", fragpath);
		exit(1);
	}
	
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	
	glGetProgramiv(program, GL_LINK_STATUS, &status_vert);
	if(!status_vert) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		pprintf(PRI_ERR, "Linking:\n%s\n", log);
		exit(1);
	}
	
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	/*	Casting to void* to prevent compiler from complaining.	*/
	free((void *)src_vert_shader);
	free((void *)src_frag_shader);
	return program;
}

void graph_draw_scene(data **object, float fps)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	char osdtime[50], osdfps[30];
	int fpscolor;
	
	/*	Text/static drawing	*/
	glUseProgram(text_shader);
	glBindBuffer(GL_ARRAY_BUFFER, textvbo);
	{
		/*	FPS	*/
		if(fps < 25) fpscolor = GL_RED;
		else if(fps < 48) fpscolor = GL_BLUE;
		else fpscolor = GL_GREEN;
		sprintf(osdfps, "FPS = %3.2f", fps);
		graph_display_text(osdfps, -0.95, 0.85, 1.0/option->width, 1.0/option->height, fpscolor);
		
		/* Timestep display */
		snprintf(osdtime, sizeof(osdtime), "Timestep = %llu", option->processed);
		graph_display_text(osdtime, -0.95, 0.65, 1.0/option->width, 1.0/option->height, GL_WHITE);
		
		/* Simulation status */
		if(!threadcontrol(PHYS_STATUS, NULL))
			graph_display_text("Simulation stopped", -0.95, -0.95, 1.0/option->width, 1.0/option->height, GL_RED);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	/*	Text/static drawing	*/
	
	/*	Dynamic drawing	*/
	glUseProgram(object_shader);
	glBindBuffer(GL_ARRAY_BUFFER, pointvbo);
	{
		/* Axis */
		draw_obj_axis();
		
		/* Objects(as points) */
		draw_obj_points(*object);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	/*	Dynamic drawing	*/
}

void graph_init()
{
	mat = calloc(16, sizeof(GLfloat));
	rotx = calloc(16, sizeof(GLfloat));
	roty = calloc(16, sizeof(GLfloat));
	rotz = calloc(16, sizeof(GLfloat));
	rotation = calloc(16, sizeof(GLfloat));
	scale = calloc(16, sizeof(GLfloat));
	pers = calloc(16, sizeof(GLfloat));
	transl = calloc(16, sizeof(GLfloat));
	
	graph_resize_wind();
	object_shader = graph_init_objects();
	text_shader = graph_init_freetype(graph_init_fontconfig());
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glActiveTexture(GL_TEXTURE0);
	glGenBuffers(1, &pointvbo);
	glGenBuffers(1, &textvbo);
	glClearColor(0.12, 0.12, 0.12, 1);
	pprintf(4, "OpenGL Version %s\n", glGetString(GL_VERSION));
	
	trn_matrix = glGetUniformLocation(object_shader, "translMat");
	rot_matrix = glGetUniformLocation(object_shader, "rotationMat");
	scl_matrix = glGetUniformLocation(object_shader, "scalingMat");
	per_matrix = glGetUniformLocation(object_shader, "perspectiveMat");
}


