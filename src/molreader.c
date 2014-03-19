#include <stdio.h>
#include <string.h>
#include <tgmath.h>
#include "molreader.h"
#include "options.h"
#include "msg_phys.h"

long double elcharge, gconst, epsno;

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

int readmolecule(char filename[200], data *object, v4sf position, v4sf velocity, int *i) {
	char str[500], atom;
	v4sf dist;
	int start = *i + 1;
	float xpos, ypos, zpos, mag;
	FILE *inpars = fopen(filename, "r");
	
	fgets(str, sizeof(str), inpars);
	fgets(str, sizeof(str), inpars);
	
	while(fgets (str, sizeof(str), inpars)!= NULL) {
		if(strstr(str, "#") == NULL) {
			*i = *i + 1;
			sscanf(str, "  %c        %f        %f        %f", &atom, &xpos, &ypos, &zpos);
			object[*i].index = *i;
			object[*i].pos = (v4sf){ xpos + position[0], ypos + position[1], zpos + position[2] };
			object[*i].vel = (v4sf){ velocity[0], velocity[1], velocity[2] };
			object[*i].charge = 0;
			if(atom == 'H') {
				object[*i].charge = 2200*elcharge;
				object[*i].ignore = '0';
				object[*i].mass = 1.0;
				object[*i].radius = 0.1;
			}
			if(atom == 'C') {
				object[*i].charge = -200*elcharge;
				object[*i].ignore = '0';
				object[*i].mass = 12.0;
				object[*i].radius = 0.3;
			}
			object[*i].atom = atom;
		}
	}
	for(int z = start; z < *i + 1; z++) {
		pprint(9, "(%0.2f, %0.2f, %0.2f)	(%0.2f, %0.2f, %0.2f) | %0.2LE | %0.2LE | %f | %c | ", \
		object[z].pos[0], object[z].pos[1], object[z].pos[2], object[z].vel[0], object[z].vel[1], \
		object[z].vel[2], object[z].mass, object[z].charge, object[z].radius, object[z].ignore);
		for(int y = start; y < *i + 1; y++) {
			
			if(atom != '0') {
				dist = object[y].pos - object[z].pos;
				mag = sqrt(dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
				if(object[y].atom == 'C' && mag < 1.5) object[z].linkwith[y] = mag;
			}
			
			if(object[z].linkwith[y] != 0) {
				pprint(9, "%i - %f, ", z, object[z].linkwith[y]);
			}
		}
		pprint(9, "\n");
	}
	
	fclose(inpars);
	return 0;
}
