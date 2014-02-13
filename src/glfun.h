#ifndef FUNCTIONS_P_INCLUDED
#define FUNCTIONS_P_INCLUDED

void make_z_rot_matrix(GLfloat angle, GLfloat *m);
void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m);
void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b);
void draw(void);
void create_shaders(void);

#endif
