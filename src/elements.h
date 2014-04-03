#ifndef PHYSENGINE_ELARR
#define PHYSENGINE_ELARR

#include <GLES2/gl2.h>

struct atomic_cont {
	long double mass;
	long double charge;
	GLfloat color[4];
};

struct atomic_cont *atom_prop;

int init_elements();
unsigned short int return_atom_num(char element[2]);
int return_atom_char(char *element, unsigned short int index);

#endif
