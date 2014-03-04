#ifndef FUNCTIONS_P_INCLUDED
#define FUNCTIONS_P_INCLUDED

#include <GLES2/gl2.h>
#include <ft2build.h>
#include "physics.h"
#include FT_FREETYPE_H

FT_Library library;
FT_Face face;
FT_GlyphSlot g;

struct character_info {
  float ax; // advance.x
  float ay; // advance.y
 
  float bw; // bitmap.width;
  float bh; // bitmap.rows;
 
  float bl; // bitmap_left;
  float bt; // bitmap_top;
 
  float tx; // x offset of glyph in texture coordinates
} c[128];

void make_x_rot_matrix(GLfloat angle, GLfloat *m);
void make_y_rot_matrix(GLfloat angle, GLfloat *m);
void make_z_rot_matrix(GLfloat angle, GLfloat *m);
void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m);
void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b);
void adjust_rot(void);
void drawobject(data object);
void render_text(const char *text, float x, float y, float sx, float sy);
void selected_box_text(data object);
int create_shaders(void);

#endif
