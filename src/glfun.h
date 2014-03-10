#ifndef FUNCTIONS_P_INCLUDED
#define FUNCTIONS_P_INCLUDED

#include <GLES2/gl2.h>
#include <ft2build.h>
#include "physics.h"
#include FT_FREETYPE_H

FT_Library library;
FT_Face face;
FT_GlyphSlot g;

void make_x_rot_matrix(GLfloat angle, GLfloat *m);
void make_y_rot_matrix(GLfloat angle, GLfloat *m);
void make_z_rot_matrix(GLfloat angle, GLfloat *m);
void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m);
void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b);
void adjust_rot(void);
void drawobject(data object);
void drawlinks(data* object, unsigned int linkcount);
void render_text(const char *text, float x, float y, float sx, float sy, unsigned int col);
void selected_box_text(data object);
int resize_wind();
void create_shaders(void);

#endif
