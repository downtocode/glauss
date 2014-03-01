#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "glfun.h"
#include "physics.h"
#include "parser.h"

GLint u_matrix;
GLint attr_pos, attr_color, attr_texcoord, attr_tex;
GLfloat view_rotx, view_roty, view_rotz;
FT_GlyphSlot g;
FT_Face face;

static const char *fragShaderText;
static const char *vertShaderText;

static GLfloat colors[] = {1.0f, 1.0f, 1.0f};

void make_z_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[0] = c;
	m[1] = s;
	m[4] = -s;
	m[5] = c;
}

void make_x_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[5] = c;
	m[6] = s;
	m[9] = -s;
	m[10] = c;
}

void make_y_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[0] = c;
	m[2] = -s;
	m[8] = s;
	m[10] = c;
}

void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m)
{
	m[0] = xs;
	m[5] = ys;
	m[10] = zs;
}


void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b)
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

void adjust_rot(void)
{
	GLfloat mat[16], rotx[16], roty[16], rotz[16], scale[16];
	mat[0] = mat[5] = mat[10] = mat[15] = 1;
	rotx[0] = rotx[5] = rotx[10] = rotx[15] = 1;
	roty[0] = roty[5] = roty[10] = roty[15] = 1;
	rotz[0] = rotz[5] = rotz[10] = rotz[15] = 1;
	scale[0] = scale[5] = scale[10] = scale[15] = 1;
	
	/* Set modelview/projection matrix */
	make_x_rot_matrix(view_rotx, rotx);
	make_y_rot_matrix(view_roty, roty);
	make_z_rot_matrix(view_rotz, rotz);
	make_scale_matrix(0.5, 1, 1, scale);
	mul_matrix(mat, roty, rotx);
	mul_matrix(mat, mat, rotz);
	mul_matrix(mat, mat, scale);
	glUniformMatrix4fv(u_matrix, 1, GL_FALSE, mat);
}

void render_text(const char *text, float x, float y, float sx, float sy) {
	const char *p;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableVertexAttribArray(attr_pos);
	glVertexAttribPointer(attr_pos, 4, GL_FLOAT, GL_FALSE, 0, 0);
	for(p = text; *p; p++) {
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
			
		glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		x += (g->advance.x >> 6) * sx;
		y += (g->advance.y >> 6) * sy;
	}
	glDisableVertexAttribArray(attr_pos);
	glDisable(GL_BLEND);
}

void drawcircle(float posx, float posy, float radius) {
	int circle_precision = 9;
	GLfloat points[circle_precision][2];
	
	for(int j = 0; j < circle_precision + 1; j++) {
		float radical = ((float)j/circle_precision)*2*acos(-1);
		points[j][0] = posx + radius*cos(radical);
		points[j][1] = posy + radius*sin(radical);
	}
	
	glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attr_pos);
	glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);
	glEnableVertexAttribArray(attr_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof points, points, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_FAN, 0, circle_precision);
	glDisableVertexAttribArray(attr_pos);
	glDisableVertexAttribArray(attr_color);
}

void create_shaders(void)
{
	fragShaderText = readshader("./resources/shaders/object.frag");
	vertShaderText = readshader("./resources/shaders/object.vert");
	
	GLuint fragShader, vertShader, program;
	GLint stat;
	
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, (const char **) &fragShaderText, NULL);
	glCompileShader(fragShader);
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &stat);
	if(!stat) {
		printf("Error: fragment shader did not compile!\n");
		exit(1);
	}
	
	vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, (const char **) &vertShaderText, NULL);
	glCompileShader(vertShader);
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &stat);
	if(!stat) {
		printf("Error: vertex shader did not compile!\n");
		exit(1);
	}

	program = glCreateProgram();
	glAttachShader(program, fragShader);
	glAttachShader(program, vertShader);
	glLinkProgram(program);
	
	glGetProgramiv(program, GL_LINK_STATUS, &stat);
	if(!stat) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		printf("Error: linking:\n%s\n", log);
		exit(1);
	}
	
	glUseProgram(program);
	
	
	glBindAttribLocation(program, attr_pos, "pos");
	glBindAttribLocation(program, attr_texcoord, "texcoord");
	glBindAttribLocation(program, attr_color, "colors");
	glBindAttribLocation(program, attr_tex, "tex");
	glLinkProgram(program);  /* needed to put attribs into effect */

	u_matrix = glGetUniformLocation(program, "modelviewProjection");
}
