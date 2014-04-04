/*
 * This file is part of physengine.
 * Copyright (c) 2012 Rostislav Pehlivanov <atomnuker@gmail.com>
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
#include <string.h>
#include <stdio.h>
#include "elements.h"

int init_elements()
{
	atom_prop = calloc(120, sizeof(struct atomic_cont));
	short unsigned int index;
	double mass;
	char str[500], name[2];
	int colR, colG, colB, colA;
	FILE *inelements = fopen("./resources/elements.conf", "r");
	while(fgets (str, sizeof(str), inelements)!=NULL) {
		if(strncmp(str, "#", 1)!=0) {
			sscanf(str, "index=%hu; mass=%lf; color={%i, %i, %i, %i}; name=%s", \
						&index, &mass, &colR, &colG, &colB, &colA, name);
			atom_prop[index].mass = mass;
			strcpy(atom_prop[index].name, name);
			atom_prop[index].color[0] = (float)colR/255;
			atom_prop[index].color[1] = (float)colG/255;
			atom_prop[index].color[2] = (float)colB/255;
			atom_prop[index].color[3] = (float)colA/255;
		}
	}
	fclose(inelements);
	return 0;
}

/*
 * Used only in molreader.c and parser.c, before the physics have even initialized.
 * We can afford to waste time looping through all. Nothing faster than a C loop, right?
 */
unsigned short int return_atom_num(char element[2])
{
	for(int i=1; i<121; i++) {
		if(strcmp(element, atom_prop[i].name) == 0) {
			return i;
		}
	}
	return 0;
}
