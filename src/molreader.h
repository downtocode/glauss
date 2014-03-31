#ifndef PHYSENGINE_INPUT
#define PHYSENGINE_INPUT

#include "physics.h"

#define MOL_XYZ 1
#define MOL_PDB 2

int probefile(char filename[200], char moltype[20]);
int readmolecule(char filename[200], char moltype[20], data *object, v4sd position, v4sd velocity, int *i);

#endif
