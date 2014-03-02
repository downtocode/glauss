#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "glfun.h"
#include "physics.h"
#include "parser.h"

//Object shader global vars
GLuint programObj;
GLint u_matrix;
GLint objattr_pos, objattr_color;

//Text shader global vars
GLuint programText;
GLint textattr_coord, textattr_texcoord, textattr_tex, textattr_color;

GLfloat view_rotx, view_roty, view_rotz, scalefactor;
FT_GlyphSlot g;
FT_Face face;

static GLfloat objectcolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
static GLfloat textcolor[] = {1.0f, 1.0f, 1.0f, 1.0f};

void make_z_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[10] = m[15] = 1;
	m[0] = c;
	m[1] = s;
	m[4] = -s;
	m[5] = c;
}

void make_x_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[0] = m[15] = 1;
	m[5] = c;
	m[6] = s;
	m[9] = -s;
	m[10] = c;
}

void make_y_rot_matrix(GLfloat angle, GLfloat *m)
{
	float c = cos(angle * acos(-1) / 180.0);
	float s = sin(angle * acos(-1) / 180.0);
	m[5] = m[15] = 1;
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
	m[15] = 1;
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
	
	/* Set modelview/projection matrix */
	make_x_rot_matrix(view_rotx, rotx);
	make_y_rot_matrix(view_roty, roty);
	make_z_rot_matrix(view_rotz, rotz);
	make_scale_matrix(0.5*scalefactor, 1*scalefactor, 1*scalefactor, scale);
	mul_matrix(mat, roty, rotx);
	mul_matrix(mat, mat, rotz);
	mul_matrix(mat, mat, scale);
	glUniformMatrix4fv(u_matrix, 1, GL_FALSE, mat);
}

void render_text(const char *text, float x, float y, float sx, float sy) {
	const char *p;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniform4fv(textattr_color, 1, textcolor);
	glVertexAttribPointer(textattr_color, 4, GL_FLOAT, GL_FALSE, 0, textcolor);
	glEnableVertexAttribArray(textattr_coord);
	glEnableVertexAttribArray(textattr_color);
	glVertexAttribPointer(textattr_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
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
	glDisableVertexAttribArray(textattr_coord);
	glDisableVertexAttribArray(textattr_color);
	glDisable(GL_BLEND);
}

void drawobject(data object) {
	adjust_rot();
	int circle_precision = 9;
	GLfloat points[circle_precision][3];
	
	for(int j = 0; j < circle_precision + 1; j++) {
		float radical = ((float)j/circle_precision)*2*acos(-1);
		points[j][0] = object.pos[0] + object.radius*cos(radical);
		points[j][1] = object.pos[1] + object.radius*sin(radical);
		points[j][2] = object.pos[2];
	}
	
	glVertexAttribPointer(objattr_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(objattr_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof points, points, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_FAN, 0, circle_precision);
	glDisableVertexAttribArray(objattr_pos);
}

void create_shaders(void)
{
	GLint statObj, statText;
	const char *srcVertShaderObject = readshader("./resources/shaders/object_vs.glsl");
	const char *srcFragShaderObject = readshader("./resources/shaders/object_fs.glsl");
	const char *srcVertShaderText = readshader("./resources/shaders/text_vs.glsl");
	const char *srcFragShaderText = readshader("./resources/shaders/text_fs.glsl");
	
	GLuint fragShaderObj = glCreateShader(GL_FRAGMENT_SHADER), vertShaderObj = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShaderText = glCreateShader(GL_FRAGMENT_SHADER), vertShaderText = glCreateShader(GL_VERTEX_SHADER);
	
	programText = glCreateProgram();
	programObj = glCreateProgram();
	
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
	
	glAttachShader(programText, fragShaderText);
	glAttachShader(programText, vertShaderText);
	glAttachShader(programObj, fragShaderObj);
	glAttachShader(programObj, vertShaderObj);
	glLinkProgram(programObj);
	glLinkProgram(programText);
	
	glGetProgramiv(programObj, GL_LINK_STATUS, &statObj);
	glGetProgramiv(programText, GL_LINK_STATUS, &statText);
	if(!statObj || !statText) {
		char log[1000];
		GLsizei len;
		if(!statObj) glGetProgramInfoLog(programObj, 1000, &len, log);
		else glGetProgramInfoLog(programText, 1000, &len, log);
		printf("Error: linking:\n%s\n", log);
		exit(1);
	}
	glDeleteShader(fragShaderObj);
	glDeleteShader(vertShaderObj);
	glDeleteShader(fragShaderText);
	glDeleteShader(vertShaderText);
	
	glBindAttribLocation(programObj, objattr_pos, "pos");
	glBindAttribLocation(programObj, objattr_color, "colors");
	glBindAttribLocation(programText, textattr_coord, "coord");
	glBindAttribLocation(programText, textattr_texcoord, "texcoord");
	glBindAttribLocation(programText, textattr_tex, "tex");
	glBindAttribLocation(programText, textattr_color, "color");
	glLinkProgram(programText);
	glLinkProgram(programObj);
	u_matrix = glGetUniformLocation(programObj, "modelviewProjection");
	textattr_tex = glGetUniformLocation(programText, "tex");
	textattr_color = glGetUniformLocation(programText, "color");
}
