#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tgmath.h>
#include "molreader.h"
#include "options.h"
#include "msg_phys.h"

int probefile(char filename[200], char moltype[20]) {
	char str[500];
	int counter = 0;
	FILE *inprep = fopen(filename, "r");
	
	/* 
	 * XYZ files have total number of atoms in their first line.
	 * PDB files contain an incrementing index, but I'd like to avoid using sscanf here.
	 */
	if(strstr(moltype, "xyz")!=NULL) {
		fgets(str, sizeof(str), inprep);
		fclose(inprep);
		sscanf(str, "%i", &counter);
		return counter;
	} else if(strstr(moltype, "pdb")!=NULL) {
		while(fgets (str, sizeof(str), inprep)!= NULL) {
			if(strstr(str, "ATOM")!=NULL) {
				counter++;
			}
		}
		return counter;
	}
	return 0;
}

int readmolecule(char filename[200], char moltype[20], data *object, v4sd position, v4sd velocity, int *i) {
	char str[500], atom, pdbtype[10], pdbatomname[10], pdbresidue[10], pdbreschain;
	v4sd dist;
	int filetype, start = *i + 1, pdbatomindex, pdbresidueseq;
	float xpos, ypos, zpos, mag, pdboccupy, pdbtemp, pdboffset;
	FILE *inpars = fopen(filename, "r");
	
	/* Put format specific quirks here. */
	if(strstr(moltype, "xyz")!=NULL) {
		filetype = MOL_XYZ;
		/* Skip first two lines of XYZ files. */
		fgets(str, sizeof(str), inpars);
		fgets(str, sizeof(str), inpars);
	} else if(strstr(moltype, "pdb")!=NULL) {
		filetype = MOL_PDB;
	}
	
	while(fgets (str, sizeof(str), inpars)!= NULL) {
		if(strstr(str, "#") == NULL) {
			*i = *i + 1;
			if(filetype == MOL_XYZ) {
				sscanf(str, " %c  %f         %f         %f", &atom, &xpos, &ypos, &zpos);
			} else if(filetype == MOL_PDB) {
				if(strstr(str, "ATOM")!=NULL) {
					sscanf(str, "%s %i %s %s %c %i %f %f %f %f %f %c %f",\
							pdbtype, &pdbatomindex, pdbatomname, pdbresidue, &pdbreschain, &pdbresidueseq,\
							&xpos, &ypos, &zpos, &pdboccupy, &pdbtemp, &atom, &pdboffset);
				}
			}
			object[*i].index = *i;
			object[*i].pos = (v4sd){ xpos + position[0], ypos + position[1], zpos + position[2] };
			object[*i].vel = (v4sd){ velocity[0], velocity[1], velocity[2] };
			if(atom == 'H') {
				object[*i].charge = 2200*option->elcharge;
				object[*i].ignore = '0';
				object[*i].mass = 1.0;
				object[*i].radius = 0.1;
			} else if(atom == 'C') {
				object[*i].charge = -200*option->elcharge;
				object[*i].ignore = '0';
				object[*i].mass = 12.0;
				object[*i].radius = 0.3;
			} else {
				object[*i].charge = 0;
				object[*i].ignore = '0';
				object[*i].mass = 12.0;
				object[*i].radius = 0.3;
			}
			object[*i].atom = atom;
		}
	}
	/*
	 * Build initial links here.
	 */
	for(int z = start; z < *i + 1; z++) {
		pprintf(9, "(%0.2f, %0.2f, %0.2f)	(%0.2f, %0.2f, %0.2f) | %0.2LE | %0.2LE | %f | %c | ", \
		object[z].pos[0], object[z].pos[1], object[z].pos[2], object[z].vel[0], object[z].vel[1], \
		object[z].vel[2], object[z].mass, object[z].charge, object[z].radius, object[z].ignore);
		for(int y = start; y < *i + 1; y++) {
			if(atom != '0') {
				dist = object[y].pos - object[z].pos;
				mag = sqrt(dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
				if(object[y].atom == 'C' && mag < 1.45) object[z].linkwith[y] = mag;
			}
			if(object[z].linkwith[y] != 0) {
				pprintf(PRI_SPAM, "%i - %f, ", z, object[z].linkwith[y]);
			}
		}
		pprintf(9, "\n");
	}
	
	fclose(inpars);
	return 0;
}
