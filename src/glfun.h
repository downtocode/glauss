#ifndef FUNCTIONS_P_INCLUDED
#define FUNCTIONS_P_INCLUDED

#include <GLES2/gl2.h>
#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library library;
FT_Face face;
FT_GlyphSlot g;


void render_text(const char *text, float x, float y, float sx, float sy);
void make_x_rot_matrix(GLfloat angle, GLfloat *m);
void make_y_rot_matrix(GLfloat angle, GLfloat *m);
void make_z_rot_matrix(GLfloat angle, GLfloat *m);
void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m);
void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b);
void adjust_rot(void);
void drawcircle(float posx, float posy, float radius);
void create_shaders(void);

#endif
