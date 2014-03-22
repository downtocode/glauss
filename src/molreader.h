#ifndef PHYSENGINE_INPUT
#define PHYSENGINE_INPUT

#include "physics.h"

int probefile(char filename[200]);
int readmolecule(char filename[200], data *object, v4sd position, v4sd velocity, int *i);

#endif
