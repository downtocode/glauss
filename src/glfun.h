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
void adjust_rot(void);

void drawaxis();
void drawobject(data object);
void drawlinks(data* object);
void render_text(const char *text, float x, float y, float sx, float sy, unsigned int col);
void selected_box_text(data object);

int resize_wind();
void create_shaders(void);

#endif
