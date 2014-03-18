#include <stdio.h>
#include <string.h>
#include <tgmath.h>
#include "molreader.h"


/* 
 * Universal function to read the amount of objects/atoms in a molecule
 * Thankfully every standard I've seen uses an atom-per-line structure,
 * so there's no need to have custom pre-parsers for each.
 */
int probefile(char filename[200]) {
	int counter;
	int code = 1;
	char str[500];
	FILE *inprep = fopen(filename, "r");
	
	/*	XYZ files have total number of atoms in their first line.	*/
	if(code == 1) {
		fgets(str, sizeof(str), inprep);
		fclose(inprep);
		sscanf(str, "%i", &counter);
		return counter;
	}
	return 0;
}

int readmolecule(char filename[200], data *object, int start, int end) {
	char str[500], atom;
	v4sf dist;
	float xpos, ypos, zpos, mag;
	int i = start + 1;
	FILE *inpars = fopen(filename, "r");
	
	fgets(str, sizeof(str), inpars);
	fgets(str, sizeof(str), inpars);
	
	while(fgets (str, sizeof(str), inpars)!= NULL) {
		if(strstr(str, "#") == NULL) {
			sscanf(str, "  %c        %f        %f        %f", &atom, &xpos, &ypos, &zpos);
			
			object[i].pos = (v4sf){ xpos, ypos, zpos };
			object[i].vel = (v4sf){ 0, 0, 0 };
			object[i].mass = 1.0;
			object[i].charge = 0;
			object[i].ignore = '0';
			if(atom == 'H') object[i].charge = 400;
			if(atom == 'C') {
				object[i].charge = -400;
				object[i].ignore = '1';
			}
			object[i].atom = atom;
			object[i].center = 0;
			object[i].index = i;
			object[i].radius = 0.1;
			for(int y = start + 1; y < end + 1; y++) {
				object[i].linkwith[y] = 0;
			}
			i++;
		}
	}
	for(int i = start +1; i < end+1; i++) {
		printf("(%0.2f, %0.2f, %0.2f)	(%0.2f, %0.2f, %0.2f) | %0.2LE | %0.2LE | %f | %c | ", \
		object[i].pos[0], object[i].pos[1], object[i].pos[2], object[i].vel[0], object[i].vel[1], \
		object[i].vel[2], object[i].mass, object[i].charge, object[i].radius, object[i].ignore);
		for(int y = start +1; y < end+1; y++) {
			
			if(atom != '0') {
				dist = object[y].pos - object[i].pos;
				mag = sqrt(dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
				if(object[y].atom == 'C' && mag < 1.5) object[i].linkwith[y] = mag-0.03;
			}
			
			if(object[i].linkwith[y] != 0) {
				printf("%i - %f, ", i, object[i].linkwith[y]);
			}
		}
		printf("\n");
	}
	
	fclose(inpars);
	return 0;
}
