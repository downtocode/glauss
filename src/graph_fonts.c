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
#include <GLES2/gl2.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "msg_phys.h"
#include "graph.h"
#include "graph_objects.h"
#include "graph_fonts.h"
#include "physics.h"
#include "physics_aux.h"
#include "options.h"

static const char text_vs[] =
// Generated from text_vs.glsl
#include "shaders/text_vs.h"
;

static const char text_fs[] =
// Generated from text_fs.glsl
#include "shaders/text_fs.h"
;

static FT_Library library;
static FT_Face face;
static FT_GlyphSlot g;

static GLint textattr_coord, textattr_texcoord, textattr_tex, textattr_color;

static const GLfloat textcolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
static const GLfloat red[] = {1.0f, 0.0f, 0.0f, 1.0f};
static const GLfloat green[] = {0.0f, 1.0f, 0.0f, 1.0f};
static const GLfloat blue[] = {0.0f, 0.0f, 1.0f, 1.0f};

const char *graph_init_fontconfig()
{
	FcConfig *fc_config = FcInitLoadConfigAndFonts();
	FcPattern *fc_pattern = FcPatternCreate();
	FcPatternAddString(fc_pattern, FC_FULLNAME, (const FcChar8 *)"Arial");
	FcPatternAddBool(fc_pattern, FC_ANTIALIAS, 1);
	FcResult fc_result;
	FcDefaultSubstitute(fc_pattern);
	FcConfigSubstitute(fc_config, fc_pattern, FcMatchPattern);
	FcPattern *fc_font_chosen = FcFontMatch(fc_config, fc_pattern, &fc_result);
	FcValue fc_value;
	FcPatternGet(fc_font_chosen, "file", 0, &fc_value);
	return (const char *)fc_value.u.s;
}

unsigned int graph_init_freetype(const char *fontname)
{
	if(FT_Init_FreeType(&library)) {
		pprintf(PRI_ERR, "Could not init freetype library.\n");
		exit(1);
	}
	if(FT_New_Face(library, fontname, 0, &face)) {
		pprintf(PRI_ERR, "Could not open font.\n");
		exit(1);
	}
	FT_Set_Pixel_Sizes(face, 0, 34);
	g = face->glyph;
	
	GLuint text_program = graph_compile_shader(text_vs, text_fs);
	
	glBindAttribLocation(text_program, textattr_coord, "coord");
	glBindAttribLocation(text_program, textattr_texcoord, "textcolor");
	glBindAttribLocation(text_program, textattr_tex, "tex");
	glBindAttribLocation(text_program, textattr_color, "color");
	textattr_tex = glGetUniformLocation(text_program, "tex");
	textattr_color = glGetUniformLocation(text_program, "textcolor");
	
	/*	OpenGL adjustments needed to properly display text */
	glUniform1i(textattr_tex, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	return text_program;
}

void graph_stop_freetype()
{
	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

void graph_display_text(const char *text, float x, float y, float s, short col)
{
	float sx = s/option->width, sy = s/option->height;
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
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, g->bitmap.width,
					 g->bitmap.rows, 0, GL_ALPHA,GL_UNSIGNED_BYTE,
					 g->bitmap.buffer);
		
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
		
		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		x += (g->advance.x >> 6) * sx;
		y += (g->advance.y >> 6) * sy;
	}
	glDisableVertexAttribArray(textattr_coord);
	glDisableVertexAttribArray(textattr_color);
	glDisable(GL_BLEND);
	if(col != 0) glUniform4fv(textattr_color, 1, textcolor);
}

void graph_display_object_info(data object)
{
	float boxsize = 0.13*(object.radius*15);
	GLfloat objpoint[3] = {0,0,0};
	
	GLfloat chosenbox[4][2] = {
		{objpoint[0] - boxsize, objpoint[1] - boxsize},
		{objpoint[0] - boxsize, chosenbox[1][1] = objpoint[1] + boxsize},
		{objpoint[0] + boxsize, objpoint[1] + boxsize},
		{objpoint[0] + boxsize, chosenbox[3][1] = objpoint[1] - boxsize},
	};
	
	char osdstr[20];
	sprintf(osdstr, "Atom=%s", atom_prop[object.atomnumber].name);
	
	graph_display_text(osdstr, objpoint[0] + object.radius,\
					   objpoint[1] + object.radius, 1.0, GL_RED);
	
	glEnableVertexAttribArray(textattr_coord);
	glVertexAttribPointer(textattr_coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(chosenbox), chosenbox, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glDisableVertexAttribArray(textattr_coord);
}
