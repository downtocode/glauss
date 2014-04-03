#include <string.h>
#include <stdio.h>
#include "elements.h"

int init_elements() {
	atom_prop = calloc(120, sizeof(struct atomic_cont));
	short unsigned int index;
	long double mass;
	char str[500], name[2];
	int colR, colG, colB, colA;
	FILE *inelements = fopen("./resources/elements.conf", "r");
	while(fgets (str, sizeof(str), inelements)!=NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "index=%hi; mass=%Lf; color={%i, %i, %i, %i}; name=%s", \
						&index, &mass, &colR, &colG, &colB, &colA, name);
			atom_prop[index].mass = mass;
			strcpy(atom_prop[index].name, name);
			atom_prop[index].color[0] = (float)colR/255;
			atom_prop[index].color[1] = (float)colG/255;
			atom_prop[index].color[2] = (float)colB/255;
			atom_prop[index].color[3] = (float)colA/255;
			printf("Color %i, Name %s, Mass %Lf = %i, %i, %i, %i\n", index, name, mass, colR, colG, colB, colA);
		}
	}
	fclose(inelements);
	return 0;
}

/*
 * Used only in molreader.c and parser.c, before the physics have even initialized.
 * We can afford to waste time looping through all.
 */
unsigned short int return_atom_num(char element[2]) {
	for(int i=1; i<121; i++) {
		if(strcmp(element, atom_prop[i].name) == 0) {
			return i;
		}
	}
	return 0;
}
