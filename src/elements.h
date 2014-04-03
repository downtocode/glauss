#ifndef PHYSENGINE_ELARR
#define PHYSENGINE_ELARR

#include <GLES2/gl2.h>

struct atomic_cont {
	char name[2];
	long double mass;
	long double charge;
	GLfloat color[4];
};

struct atomic_cont *atom_prop;

int init_elements();
unsigned short int return_atom_num(char element[2]);

#endif
