#include <string.h>
#include <stdio.h>
#include "elements.h"

unsigned short int return_atom_num(char element[2]) {
	if(strcmp(element, "H")==0) return 1;
	if(strcmp(element, "C")==0) return 6;
	return 0;
}

int return_atom_char(char *element, unsigned short int index) {
	if(index == 0) sprintf(element, "0");
	if(index == 1) sprintf(element, "H");
	if(index == 6) sprintf(element, "C");
	return 0;
}
