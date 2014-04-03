#include <string.h>
#include <stdio.h>
#include "elements.h"

int init_elements() {
	atom_prop = calloc(120, sizeof(struct atomic_cont));
	short unsigned int index;
	long double mass;
	char str[500];
	float colR, colG, colB, colA;
	FILE *inelements = fopen("./resources/elements.conf", "r");
	while(fgets (str, sizeof(str), inelements)!=NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "index=%hi; mass=%Lf; color={%f, %f, %f, %f};", &index, &mass, &colR, &colG, &colB, &colA);
			atom_prop[index].mass = mass;
			atom_prop[index].color[0] = colR;
			atom_prop[index].color[1] = colG;
			atom_prop[index].color[2] = colB;
			atom_prop[index].color[3] = colA;
			printf("Color %i = %f, %f, %f, %f\n", index, colR, colG, colB, colA);
		}
	}
	fclose(inelements);
	return 0;
}

unsigned short int return_atom_num(char element[2]) {
	if(strcmp(element, "H")==0) return 1;
	if(strcmp(element, "HE")==0) return 2;
	if(strcmp(element, "LI")==0) return 3;
	if(strcmp(element, "BE")==0) return 4;
	if(strcmp(element, "B")==0) return 5;
	if(strcmp(element, "C")==0) return 6;
	if(strcmp(element, "N")==0) return 7;
	if(strcmp(element, "O")==0) return 8;
	if(strcmp(element, "F")==0) return 9;
	if(strcmp(element, "NE")==0) return 10;
	if(strcmp(element, "NA")==0) return 11;
	if(strcmp(element, "MG")==0) return 12;
	if(strcmp(element, "AL")==0) return 13;
	return 0;
}

int return_atom_char(char *element, unsigned short int index) {
	if(index == 0) sprintf(element, "0");
	if(index == 1) sprintf(element, "H");
	if(index == 6) sprintf(element, "C");
	return 0;
}
