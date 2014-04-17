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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tgmath.h>
#include "molreader.h"
#include "options.h"
#include "msg_phys.h"
#include "elements.h"

int probefile(char filename[200], char moltype[20])
{
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
			if(strstr(str, "#") == NULL) {
				if(strncmp(str, "ATOM", 4)==0) counter++;
			}
		}
		return counter;
	}
	return 0;
}

int readmolecule(char filename[200], char moltype[20], data *object, v4sd position, v4sd velocity, int *i)
{
	char str[500], atom[2], pdbtype[10], pdbatomname[10], pdbresidue[10], pdbreschain;
	v4sd dist;
	int filetype, start = *i+1, pdbatomindex, pdbresidueseq;
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
	} else {
		fprintf(stderr, "Error! Filetype %s not recognized!\n", moltype);
		exit(1);
	}
	
	while(fgets (str, sizeof(str), inpars)!= NULL) {
		if(strstr(str, "#") == NULL) {
			if(filetype == MOL_XYZ) {
				sscanf(str, " %s  %f         %f         %f", atom, &xpos, &ypos, &zpos);
			} else if(filetype == MOL_PDB) {
				if(strncmp(str, "ATOM", 4)==0) {
					sscanf(str, "%s %i %s %s %c %i %f %f %f %f %f %s %f",\
							pdbtype, &pdbatomindex, pdbatomname, pdbresidue, &pdbreschain, &pdbresidueseq,\
							&xpos, &ypos, &zpos, &pdboccupy, &pdbtemp, atom, &pdboffset);
				} else continue;
			}
			object[*i].atomnumber = return_atom_num(atom);
			object[*i].index = *i;
			/* Look in parser.c for info on why do this. */
			object[*i].pos[0] = (double)xpos + position[0];
			object[*i].pos[1] = (double)ypos + position[1];
			object[*i].pos[2] = (double)zpos + position[2];
			object[*i].vel[0] = velocity[0];
			object[*i].vel[1] = velocity[1];
			object[*i].vel[2] = velocity[2];
			if(object[*i].atomnumber == 1) {
				object[*i].charge = 2200*option->elcharge;
				object[*i].ignore = '0';
				object[*i].mass = 1.0;
				object[*i].radius = 0.05;
			} else if(object[*i].atomnumber == 6) {
				object[*i].charge = -200*option->elcharge;
				object[*i].ignore = '0';
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			} else {
				object[*i].charge = 0;
				object[*i].ignore = '0';
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			}
			*i = *i + 1;
		}
	}
	/* Build initial links here. */
	for(int z = start-1; z < *i + 1; z++) {
		pprintf(9, "(%0.2f, %0.2f, %0.2f)	(%0.2f, %0.2f, %0.2f) | %0.2LE | %0.2LE | %f | %c\n", \
		object[z].pos[0], object[z].pos[1], object[z].pos[2], object[z].vel[0], object[z].vel[1], \
		object[z].vel[2], object[z].mass, object[z].charge, object[z].radius, object[z].ignore);
		for(int y = start-1; y < *i + 1; y++) {
			if(z==y) continue;
			dist = object[y].pos - object[z].pos;
			mag = sqrt(dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
			if(object[z].atomnumber == 6) {
				if(object[y].atomnumber == 7 && mag < 2.4) {
					object[z].links[object[z].totlinks] = y;
					object[z].totlinks += 1;
				}
			}
			if(object[z].atomnumber == 1) {
				if(object[y].atomnumber == 7 && mag < 2.4) {
					object[z].links[object[z].totlinks] = y;
					object[z].totlinks += 1;
				}
			}
		}
	}
	
	fclose(inpars);
	return 0;
}
