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
#include "glfun.h"
#include "physics.h"
#include "parser.h"
#include "options.h"
#include "elements.h"

#define numshaders 2

static GLint trn_matrix, rot_matrix, scl_matrix, per_matrix;
static GLint objattr_pos, objattr_color;
static GLint textattr_coord, textattr_texcoord, textattr_tex, textattr_color;


static float aspect_ratio;
static GLfloat *mat, *rotx, *roty, *rotz, *rotation, *scale, *transl, *pers;
static const GLfloat textcolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
static const GLfloat red[] = {1.0f, 0.0f, 0.0f, 1.0f};
static const GLfloat green[] = {0.0f, 1.0f, 0.0f, 1.0f};
static const GLfloat blue[] = {0.0f, 0.0f, 1.0f, 1.0f};

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

static void transformpoint(GLfloat *p, GLfloat *m)
{
	GLfloat tempmat[16] = {0};
	tempmat[0] = p[0];
	tempmat[5] = p[1];
	tempmat[10] = p[2];
	
	mul_matrix(tempmat, mat, tempmat);
	p[0] = tempmat[0];
	p[1] = tempmat[5];
	p[2] = tempmat[10];
}

void adjust_rot(GLfloat view_rotx, GLfloat view_roty, GLfloat view_rotz, \
				GLfloat scalefactor, GLfloat tr_x, GLfloat tr_y, GLfloat tr_z)
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

void drawaxis()
{
	GLfloat axis[6][3] = {
			{0,0,0},
			{1,0,0},
			{0,0,0},
			{0,1,0},
			{0,0,0},
			{0,0,1},
	};
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(objattr_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis), axis, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_LOOP, 0, 6);
	glDisableVertexAttribArray(objattr_pos);
}

void render_text(const char *text, float x, float y, float sx, float sy, unsigned int col)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glVertexAttribPointer(textattr_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
	if(col == 0) glUniform4fv(textattr_color, 1, textcolor);
	if(col == 1) glUniform4fv(textattr_color, 1, red);
	if(col == 2) glUniform4fv(textattr_color, 1, green);
	if(col == 3) glUniform4fv(textattr_color, 1, blue);
	glVertexAttribPointer(textattr_color, 4, GL_FLOAT, GL_FALSE, 0, textcolor);
	glEnableVertexAttribArray(textattr_coord);
	glEnableVertexAttribArray(textattr_color);
	for(const char *p = text; *p; p++) {
		if(FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, g->bitmap.width, g->bitmap.rows, 0, GL_ALPHA,GL_UNSIGNED_BYTE, g->bitmap.buffer);
		
		float x2 = x + g->bitmap_left * sx;
		float y2 = -y - g->bitmap_top * sy;
		float w = g->bitmap.width * sx;
		float h = g->bitmap.rows * sy;
		
		GLfloat box[4][4] = {
			{x2,     -y2    , 0, 0},
			{x2 + w, -y2    , 1, 0},
			{x2,     -y2 - h, 0, 1},
			{x2 + w, -y2 - h, 1, 1},
		};
		
		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		x += (g->advance.x >> 6) * sx;
		y += (g->advance.y >> 6) * sy;
	}
	glDisableVertexAttribArray(textattr_coord);
	glDisableVertexAttribArray(textattr_color);
	glDisable(GL_BLEND);
	if(col != 0) glUniform4fv(textattr_color, 1, textcolor);
}

void draw_obj_sphere(data object)
{
	/* Decrease dj to get better spheres. */
	float dj = 0.5, pi=acos(-1);
	int pointcount = 0;
	float points[(int)((pi/dj)*((2*pi/dj)+1)+30)][3];
	
	glUniform4fv(objattr_color, 1, atom_prop[object.atomnumber].color);
	
	for(float i = 0; i < pi; i+=dj) {
		for(float j = 0; j < 2*pi; j+=dj) {
			points[pointcount][0] = object.pos[0] + object.radius*sin(i)*cos(j);
			points[pointcount][1] = object.pos[1] + object.radius*sin(i)*sin(j);
			points[pointcount][2] = object.pos[2] + object.radius*cos(i);
			pointcount++;
		}
	}
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(objattr_color, 4, GL_FLOAT, GL_FALSE, 0, atom_prop[object.atomnumber].color);
	glEnableVertexAttribArray(objattr_pos);
	glEnableVertexAttribArray(objattr_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_DYNAMIC_DRAW);
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, pointcount);
	
	glDisableVertexAttribArray(objattr_pos);
	glDisableVertexAttribArray(objattr_color);
	glUniform4fv(objattr_color, 1, textcolor);
}

void draw_obj_points(data* object)
{
	float points[option->obj][3];
	
	glUniform4fv(objattr_color, 1, textcolor);
	
	for(int i = 1; i < option->obj+1; i++) {
		points[i-1][0] = object[i].pos[0];
		points[i-1][1] = object[i].pos[1];
		points[i-1][2] = object[i].pos[2];
	}
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(objattr_color, 4, GL_FLOAT, GL_FALSE, 0, textcolor);
	glEnableVertexAttribArray(objattr_pos);
	glEnableVertexAttribArray(objattr_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_DYNAMIC_DRAW);
	
	glDrawArrays(GL_POINTS, 0, option->obj);
	
	glDisableVertexAttribArray(objattr_pos);
	glDisableVertexAttribArray(objattr_color);
	glUniform4fv(objattr_color, 1, textcolor);
}

unsigned int createlinks(data* object, GLfloat (**links)[3])
{
	v4sd distvec;
	double dist;
	
	unsigned int count = 0;
	for(int i = 1; i < option->obj+1; i++) {
		for(int j = 1; j < option->obj+1; j++) {
			if(i<j) continue;
			if(count>9999) continue;
			distvec = object[j].pos - object[i].pos;
			dist = sqrt(distvec[0]*distvec[0] + distvec[1]*distvec[1] + distvec[2]*distvec[2]);
			if(dist < 1.5 && object[j].atomnumber == 6) {
				(*links)[count][0] = object[i].pos[0];
				(*links)[count][1] = object[i].pos[1];
				(*links)[count][2] = object[i].pos[2];
				count++;
				(*links)[count][0] = object[j].pos[0];
				(*links)[count][1] = object[j].pos[1];
				(*links)[count][2] = object[j].pos[2];
				count++;
			}
		}
	}
	return count-1;
}

void draw_obj_links(GLfloat (**links)[3], unsigned int linkcount)
{	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(objattr_pos);
	glBufferData(GL_ARRAY_BUFFER, 10000*3*sizeof(GLfloat), **links, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_LOOP, 0, linkcount);
	glDisableVertexAttribArray(objattr_pos);
}

void selected_box_text(data object)
{
	float boxsize = 0.13*scale[5]*(object.radius*15);
	GLfloat objpoint[3] = {0,0,0};
	
	transformpoint(objpoint, mat);
	
	GLfloat chosenbox[4][2] = {
		{objpoint[0] - aspect_ratio*boxsize, objpoint[1] - boxsize},
		{objpoint[0] - aspect_ratio*boxsize, chosenbox[1][1] = objpoint[1] + boxsize},
		{objpoint[0] + aspect_ratio*boxsize, objpoint[1] + boxsize},
		{objpoint[0] + aspect_ratio*boxsize, chosenbox[3][1] = objpoint[1] - boxsize},
	};

	char osdstr[20];
	sprintf(osdstr, "Atom=%s", atom_prop[object.atomnumber].name);
	
	render_text(osdstr, objpoint[0] + object.radius*scale[5], \
	objpoint[1] + object.radius*scale[5], 1.0/option->width, 1.0/option->height, GL_RED);
	
	/*if(object.totlinks != 0) {
		sprintf(osdstr, "Links: %u", object.totlinks);
		render_text(osdstr, objpoint[0] + object.radius*scale[5], \
		objpoint[1] + object.radius*scale[5] - 0.055, 1.0/option->width, 1.0/option->height, GL_BLUE);
	}*/
	
	glEnableVertexAttribArray(textattr_coord);
	glVertexAttribPointer(textattr_coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(chosenbox), chosenbox, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glDisableVertexAttribArray(textattr_coord);
}

int resize_wind()
{
	aspect_ratio = (float)option->height/option->width;
	glViewport(0, 0, option->width, option->height);
	return 0;
}

void create_shaders(GLuint** shaderprogs)
{
	GLint statObj, statText;
	const char *srcVertShaderObject = readshader("./resources/shaders/object_vs.glsl");
	const char *srcFragShaderObject = readshader("./resources/shaders/object_fs.glsl");
	const char *srcVertShaderText = readshader("./resources/shaders/text_vs.glsl");
	const char *srcFragShaderText = readshader("./resources/shaders/text_fs.glsl");
	
	GLuint fragShaderObj = glCreateShader(GL_FRAGMENT_SHADER), vertShaderObj = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShaderText = glCreateShader(GL_FRAGMENT_SHADER), vertShaderText = glCreateShader(GL_VERTEX_SHADER);
	
	*shaderprogs = calloc(numshaders, sizeof(GLuint));
	
	(*shaderprogs)[0] = glCreateProgram();
	(*shaderprogs)[1] = glCreateProgram();
	
	glShaderSource(fragShaderObj, 1, (const char **) &srcFragShaderObject, NULL);
	glShaderSource(fragShaderText, 1, (const char **) &srcFragShaderText, NULL);
	glCompileShader(fragShaderObj);
	glCompileShader(fragShaderText);
	glGetShaderiv(fragShaderObj, GL_COMPILE_STATUS, &statObj);
	glGetShaderiv(fragShaderText, GL_COMPILE_STATUS, &statText);
	if(!statObj || !statText) {
		if(!statObj) printf("Error: object fragment shader did not compile!\n");
		if(!statText) printf("Error: text fragment shader did not compile!\n");
		exit(1);
	}
	
	glShaderSource(vertShaderObj, 1, (const char **) &srcVertShaderObject, NULL);
	glShaderSource(vertShaderText, 1, (const char **) &srcVertShaderText, NULL);
	glCompileShader(vertShaderObj);
	glCompileShader(vertShaderText);
	glGetShaderiv(vertShaderObj, GL_COMPILE_STATUS, &statObj);
	glGetShaderiv(vertShaderText, GL_COMPILE_STATUS, &statText);
	if(!statObj || !statText) {
		if(!statObj) printf("Error: object vertex shader did not compile!\n");
		if(!statText) printf("Error: text vertex shader did not compile!\n");
		exit(1);
	}
	
	glAttachShader((*shaderprogs)[1], fragShaderText);
	glAttachShader((*shaderprogs)[1], vertShaderText);
	glAttachShader((*shaderprogs)[0], fragShaderObj);
	glAttachShader((*shaderprogs)[0], vertShaderObj);
	glLinkProgram((*shaderprogs)[0]);
	glLinkProgram((*shaderprogs)[1]);
	
	glGetProgramiv((*shaderprogs)[0], GL_LINK_STATUS, &statObj);
	glGetProgramiv((*shaderprogs)[1], GL_LINK_STATUS, &statText);
	if(!statObj || !statText) {
		char log[1000];
		GLsizei len;
		if(!statObj) glGetProgramInfoLog((*shaderprogs)[0], 1000, &len, log);
		else glGetProgramInfoLog((*shaderprogs)[1], 1000, &len, log);
		printf("Error: linking:\n%s\n", log);
		exit(1);
	}
	
	glDeleteShader(fragShaderObj);
	glDeleteShader(vertShaderObj);
	glDeleteShader(fragShaderText);
	glDeleteShader(vertShaderText);
	
	/*	Cast to void * to strip qualifiers	*/
	free((void *)srcVertShaderObject);
	free((void *)srcFragShaderObject);
	free((void *)srcVertShaderText);
	free((void *)srcFragShaderText);
	
	glBindAttribLocation((*shaderprogs)[0], objattr_pos, "pos");
	glBindAttribLocation((*shaderprogs)[0], objattr_color, "objcolor");
	glLinkProgram((*shaderprogs)[0]);
	trn_matrix = glGetUniformLocation((*shaderprogs)[0], "translMat");
	rot_matrix = glGetUniformLocation((*shaderprogs)[0], "rotationMat");
	scl_matrix = glGetUniformLocation((*shaderprogs)[0], "scalingMat");
	per_matrix = glGetUniformLocation((*shaderprogs)[0], "perspectiveMat");
	objattr_color = glGetUniformLocation((*shaderprogs)[0], "objcolor");
	
	glBindAttribLocation((*shaderprogs)[1], textattr_coord, "coord");
	glBindAttribLocation((*shaderprogs)[1], textattr_texcoord, "textcolor");
	glBindAttribLocation((*shaderprogs)[1], textattr_tex, "tex");
	glBindAttribLocation((*shaderprogs)[1], textattr_color, "color");
	glLinkProgram((*shaderprogs)[1]);
	textattr_tex = glGetUniformLocation((*shaderprogs)[1], "tex");
	textattr_color = glGetUniformLocation((*shaderprogs)[1], "textcolor");
	
	mat = calloc(16, sizeof(GLfloat));
	rotx = calloc(16, sizeof(GLfloat));
	roty = calloc(16, sizeof(GLfloat));
	rotz = calloc(16, sizeof(GLfloat));
	rotation = calloc(16, sizeof(GLfloat));
	scale = calloc(16, sizeof(GLfloat));
	pers = calloc(16, sizeof(GLfloat));
	transl = calloc(16, sizeof(GLfloat));
	
	glUniform1i(textattr_tex, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}
