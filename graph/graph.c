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
#include <png.h>
#include "graph.h"
#include "graph_sdl.h"
#include "graph_objects.h"
#include "graph_fonts.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "input/parser.h"

/* UI POSITIONS */

/* FPS counter */
#define FPSx -0.95
#define FPSy  0.85
#define FPSs  1.0

/* Object count */
#define OBJx -0.95
#define OBJy  0.75
#define OBJs  1.0

/* Time counter */
#define TIMEx -0.95
#define TIMEy  0.65
#define TIMEs  1.0

/* dt display */
#define DTx -0.95
#define DTy  0.55
#define DTs  1.0

/* Algorithm display */
#define ALGx  -0.95
#define ALGy  0.45
#define ALGs  1.0

/* Simulation status */
#define SIMx 0.65
#define SIMy -0.95
#define SIMs  1.0

/* Algorithm stats */
#define STATSx -0.95
#define STATSy -0.41
#define STATSs  0.75

/* Object selection */
#define OSLx  0.70
#define OSLy -0.76
#define OSLs  1.00

/* Thread time counters */
#define THRx  0.73
#define THRy  0.95
#define THRs  0.75

/* Axis scale */
#define AXISs 0.25

/* Background color */
#define BACKr 0.06
#define BACKg 0.06
#define BACKb 0.06
#define BACKa 1.00

/* OSD Buffer size */
#define OSD_BUFFER 25

/* UI POSITIONS */

/* COLORS */
const GLfloat COL_WHITE[]   =  {  1.0f,   1.0f,   1.0f,   1.0f  };
const GLfloat COL_RED[]     =  {  1.0f,   0.0f,   0.0f,   1.0f  };
const GLfloat COL_GREEN[]   =  {  0.0f,   1.0f,   0.0f,   1.0f  };
const GLfloat COL_BLUE[]    =  {  0.0f,   0.0f,   1.0f,   1.0f  };
const GLfloat COL_YELLOW[]  =  {  1.0f,   1.0f,   0.0f,   1.0f  };
const GLfloat COL_ORANGE[]  =  { 0.82f,  0.41f,  0.11f,   1.0f  };
/* COLORS */

static GLuint pointvbo, textvbo;
static GLuint object_shader, text_shader;
static GLint trn_matrix, rot_matrix, scl_matrix, per_matrix;
static GLfloat *mat = NULL, *rotx = NULL, *roty = NULL, *rotz = NULL;
static GLfloat *rotation = NULL, *scale = NULL, *transl = NULL, *pers = NULL;

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

static void make_translation_matrix(GLfloat xpos, GLfloat ypos,
									GLfloat zpos, GLfloat *m)
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

static void make_pers_matrix(GLfloat fov, GLfloat aspect, GLfloat near,
							 GLfloat far, GLfloat *m) {
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

void graph_reset_viewport(void)
{
	glViewport(0, 0, option->width, option->height);
}

void graph_set_view(graph_window *win)
{
	make_translation_matrix(win->camera.tr_x, win->camera.tr_y, win->camera.tr_z, transl);
	
	make_scale_matrix(win->camera.aspect_ratio*win->camera.scalefactor, win->camera.scalefactor,
					  win->camera.scalefactor, scale);
	
	make_pers_matrix(10, option->width/option->height, -1, 10, pers);
	
	make_x_rot_matrix(win->camera.view_rotx, rotx);
	make_y_rot_matrix(win->camera.view_roty, roty);
	make_z_rot_matrix(win->camera.view_rotz, rotz);
	mul_matrix(mat, roty, rotx);
	mul_matrix(rotation, mat, rotz);
	glUniformMatrix4fv(trn_matrix, 1, GL_FALSE, transl);
	glUniformMatrix4fv(scl_matrix, 1, GL_FALSE, scale);
	glUniformMatrix4fv(rot_matrix, 1, GL_FALSE, rotation);
	glUniformMatrix4fv(per_matrix, 1, GL_FALSE, pers);
}

unsigned int graph_compile_shader(const char *src_vert_shader,
								  const char *src_frag_shader)
{
	GLint status_vert, status_frag;
	GLuint program = glCreateProgram();
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	
	glShaderSource(vert_shader, 1, &src_vert_shader, NULL);
	glShaderSource(frag_shader, 1, &src_frag_shader, NULL);
	glCompileShader(vert_shader);
	glCompileShader(frag_shader);
	glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &status_vert);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status_frag);
	if(!status_vert || !status_frag) {
		if(!status_vert) pprintf(PRI_ERR, "[GL] VS did not compile.\n");
		if(!status_frag) pprintf(PRI_ERR, "[GL] FS did not compile.\n");
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
		pprintf(PRI_ERR, "[GL] Linking:\n%s\n", log);
		exit(1);
	}
	
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

void graph_draw_scene(graph_window *win)
{
	if(!win) return;
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	char osdtext[OSD_BUFFER];
	struct timespec ts;
	const GLfloat *fpscolor;
	
	/*	Text/static drawing	*/
	{
		/* Set shader and vbo */
		glUseProgram(text_shader);
		glBindBuffer(GL_ARRAY_BUFFER, textvbo);
		
		/* FPS */
		fpscolor = (win->fps < 25) ? COL_RED : (win->fps < 48) ? COL_BLUE : COL_GREEN;
		snprintf(osdtext, OSD_BUFFER, "FPS = %3.2f", win->fps);
		graph_display_text(osdtext, FPSx, FPSy, FPSs, fpscolor);
		
		/* Objects */
		snprintf(osdtext, OSD_BUFFER, "Objects = %u", option->obj);
		graph_display_text(osdtext, OBJx, OBJy, OBJs, COL_WHITE);
		
		/* Timestep display */
		snprintf(osdtext, OSD_BUFFER, "Timestep = %0.4Lf", phys_stats->progress);
		graph_display_text(osdtext, DTx, DTy, DTs, COL_WHITE);
		
		/* Time constant */
		snprintf(osdtext, OSD_BUFFER, "dt = %lf", option->dt);
		graph_display_text(osdtext, TIMEx, TIMEy, TIMEs, COL_WHITE);
		
		/* Algorithm display */
		snprintf(osdtext, OSD_BUFFER, "Algorithm = %s", option->algorithm);
		graph_display_text(osdtext, ALGx, ALGy, ALGs, COL_WHITE);
		
		/* Object selection */
		if (win->start_selection)
			graph_display_text(win->currentsel, OSLx, OSLy, OSLs, COL_WHITE);
		
		/* Chosen object */
		if (win->chosen)
			graph_display_object_info(win->object, win->chosen);
		
		int status = phys_ctrl(PHYS_STATUS, NULL);
		if (status == PHYS_STATUS_RUNNING || status == PHYS_STATUS_PAUSED) {
			/* Only displayed if running */
			
			if (status == PHYS_STATUS_PAUSED) {
				graph_display_text("Simulation paused", SIMx, SIMy, SIMs, COL_YELLOW);
			}
			
			/* Thread time stats */
			for (short i = 1; i < option->threads + 1; i++) {
				if (phys_stats->t_stats) {
					clock_gettime(phys_stats->t_stats[i].clockid, &ts);
				} else {
					ts = (struct timespec){0};
				}
				snprintf(osdtext, OSD_BUFFER,
						 "Thread %i = %ld.%ld", i, ts.tv_sec, ts.tv_nsec / 1000000);
				graph_display_text(osdtext, THRx, THRy-((float)i/14), THRs, COL_WHITE);
			}
			
		} else {
			/* Only displayed if not running */
			
			/* Simulation status */
			graph_display_text("Simulation stopped", SIMx, SIMy, SIMs, COL_RED);
		}
		
		/* Algorithm stats */
		graph_display_text("Algorithm stats:", STATSx, STATSy, STATSs, COL_WHITE);
		graph_display_text("Name", STATSx, STATSy-.05, STATSs, COL_ORANGE);
		graph_display_text("Value", STATSx+.25, STATSy-.05, STATSs, COL_YELLOW);
		
		unsigned int count = 0;
		for (struct parser_map *i = phys_stats->global_stats_map; i->name; i++) {
			char res[50];
			parser_get_value_str(*i, res, 50);
			graph_display_text(i->name, STATSx, STATSy-((float)count/18)-.10, STATSs, COL_ORANGE);
			graph_display_text(res, STATSx+.25, STATSy-((float)count++/18)-.10, STATSs, COL_YELLOW);
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	/*	Text/static drawing	*/
	
	/*	Dynamic drawing	*/
	{
		/* Shader and VBO */
		glUseProgram(object_shader);
		glBindBuffer(GL_ARRAY_BUFFER, pointvbo);
		
		/* Axis */
		draw_obj_axis(AXISs);
		
		/* Objects(as points) */
		draw_obj_col_points(win->object);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	/* Take screenshot if signaled by physics ctrl thread */
	if (option->write_sshot_now) {
		graph_sshot(phys_stats->progress);
		option->write_sshot_now = false;
	}
	/*	Dynamic drawing	*/
}

void graph_quit(void)
{
	free(mat);
	free(rotx);
	free(roty);
	free(rotz);
	free(rotation);
	free(scale);
	free(pers);
	free(transl);
}

void graph_init(void)
{
	mat       =  calloc(16, sizeof(GLfloat));
	rotx      =  calloc(16, sizeof(GLfloat));
	roty      =  calloc(16, sizeof(GLfloat));
	rotz      =  calloc(16, sizeof(GLfloat));
	rotation  =  calloc(16, sizeof(GLfloat));
	scale     =  calloc(16, sizeof(GLfloat));
	pers      =  calloc(16, sizeof(GLfloat));
	transl    =  calloc(16, sizeof(GLfloat));
	
	object_shader = graph_init_objects();
	text_shader = graph_init_freetype(graph_init_fontconfig());
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glActiveTexture(GL_TEXTURE0);
	glGenBuffers(1, &pointvbo);
	glGenBuffers(1, &textvbo);
	glClearColor(BACKr, BACKg, BACKb, BACKa);
	pprintf(PRI_VERYLOW, "[GL] OpenGL Version %s\n", glGetString(GL_VERSION));
	
	trn_matrix = glGetUniformLocation(object_shader, "translMat");
	rot_matrix = glGetUniformLocation(object_shader, "rotationMat");
	scl_matrix = glGetUniformLocation(object_shader, "scalingMat");
	per_matrix = glGetUniformLocation(object_shader, "perspectiveMat");
}

int graph_sshot(long double arg)
{
	#if HAS_PNG
	/* Open file */
	char filename[32];
	int w = option->width, h = option->height;
	
	/* Open file */
	snprintf(filename, sizeof(filename), option->sshot_temp, arg);
	if (!access(filename, R_OK))
		return 2;
	
	FILE *fshot = fopen(filename, "w");
	if (!fshot)
		return 2;
	
	/* Get pixels */
	unsigned char *pixels = malloc(sizeof(unsigned char)*w*h*4);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		return 1;
	
	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, &info);
		return 1;
	}
	
	png_init_io(png, fshot);
	png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_colorp palette = png_malloc(png, PNG_MAX_PALETTE_LENGTH*sizeof(png_color));
	if (!palette) {
		fclose(fshot);
		png_destroy_write_struct(&png, &info);
		return 1;
	}
	
	png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
	png_write_info(png, info);
	png_set_packing(png);
	
	png_bytepp rows = png_malloc(png, h*sizeof(png_bytep));
	for (int r = 0; r < h; r++)
		rows[r] = (pixels + (h - r)*w*4);
	
	png_write_image(png, rows);
	png_write_end(png, info);
	
	png_free(png, rows);
	png_free(png, palette);
	png_destroy_write_struct(&png, &info);
	
	fclose(fshot);
	free(pixels);
	
	pprintf(PRI_MEDIUM, "Wrote screenshot %s\n", filename);
	return 0;
	#endif
	return 1;
}
