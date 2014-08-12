/*
 * This file is part of physengine.
 * Copyright (c) 2013 Rostislav Pehlivanov <atomnuker@gmail.com>
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
#include "in_file.h"
#include "options.h"
#include "msg_phys.h"
#include "physics_aux.h"

int in_probe_file(const char *filename)
{
	char str[500];
	int counter = 0;
	FILE *inprep = fopen(filename, "r");
	
	int filetype = 0;
	
	/* Put format specific quirks here. */
	if(strstr(filename, "xyz")!=NULL) {
		filetype = MOL_XYZ;
	} else if(strstr(filename, "pdb")!=NULL) {
		filetype = MOL_PDB;
	} else if(strstr(filename, "obj")!=NULL) {
		filetype = MOL_OBJ;
	} else {
		fprintf(stderr, "[IN] Filetype of %s not recognized!\n", filename);
		exit(1);
	}
	
	/* 
	 * XYZ files have total number of atoms in their first line.
	 * PDB files contain an incrementing index, but I'd like to avoid sscanf.
	 */
	if(filetype == MOL_XYZ) {
		fgets(str, sizeof(str), inprep);
		fclose(inprep);
		sscanf(str, "%i", &counter);
		return counter;
	} else if(filetype == MOL_PDB) {
		while(fgets (str, sizeof(str), inprep)!= NULL) {
			if(strstr(str, "#") == NULL) {
				if(strncmp(str, "ATOM", 4)==0) counter++;
			}
		}
		return counter;
	} else if(filetype == MOL_OBJ) {
		while(fgets (str, sizeof(str), inprep)!= NULL) {
			if(strstr(str, "#") == NULL) {
				if(strncmp(str, "v ", 2)==0) counter++;
			}
		}
		return counter;
	}
	return 0;
}

int in_read_file(data *object, int *i, in_file file)
{
	char str[500], atom[2], pdbtype[10], pdbatomname[10], pdbresidue[10];
	char pdbreschain;
	int filetype, pdbatomindex, pdbresidueseq;
	float xpos, ypos, zpos, pdboccupy, pdbtemp, pdboffset;
	FILE *inpars = fopen(file.filename, "r");
	
	/* Put format specific quirks here. */
	if(strstr(file.filename, "xyz")!=NULL) {
		filetype = MOL_XYZ;
		/* Skip first two lines of XYZ files. */
		fgets(str, sizeof(str), inpars);
		fgets(str, sizeof(str), inpars);
	} else if(strstr(file.filename, "pdb")!=NULL) {
		filetype = MOL_PDB;
	} else if(strstr(file.filename, "obj")!=NULL) {
		filetype = MOL_OBJ;
	} else {
		fprintf(stderr, "Error! Filetype of %s not recognized!\n", file.filename);
		exit(1);
	}
	
	while(fgets (str, sizeof(str), inpars)!= NULL) {
		if(strstr(str, "#") == NULL) {
			if(filetype == MOL_XYZ) {
				sscanf(str, " %s  %f         %f         %f", atom, &xpos, &ypos,
					   &zpos);
			} else if(filetype == MOL_PDB) {
				if(strncmp(str, "ATOM", 4)==0) {
					sscanf(str, "%s %i %s %s %c %i %f %f %f %f %f %s %f",\
							pdbtype, &pdbatomindex, pdbatomname, pdbresidue,
							&pdbreschain, &pdbresidueseq,\
							&xpos, &ypos, &zpos, &pdboccupy, &pdbtemp,
							atom, &pdboffset);
				} else continue;
			} else if(filetype == MOL_OBJ) {
				if(strncmp(str, "v ", 2)==0) {
					sscanf(str, "v  %f %f %f", &xpos, &ypos, &zpos);
					xpos/=100;
					ypos/=100;
					zpos/=100;
				} else continue;
			}
			//object[*i].atomnumber = return_atom_num(atom);
			object[*i].id = *i;
			/* By specifications XYZ and PDB files default to float */
			object[*i].pos[0] = file.scale*xpos + file.inf->pos[0];
			object[*i].pos[1] = file.scale*ypos + file.inf->pos[1];
			object[*i].pos[2] = file.scale*zpos + file.inf->pos[2];
			object[*i].vel[0] = file.inf->vel[0];
			object[*i].vel[1] = file.inf->vel[1];
			object[*i].vel[2] = file.inf->vel[2];
			if(object[*i].atomnumber == 1) {
				object[*i].charge = 2200*option->elcharge;
				object[*i].ignore = 0;
				object[*i].mass = 1.0;
				object[*i].radius = 0.05;
			} else if(object[*i].atomnumber == 6) {
				object[*i].charge = -200*option->elcharge;
				object[*i].ignore = 0;
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			} else {
				object[*i].atomnumber = 0;
				object[*i].charge = 0;
				object[*i].ignore = 0;
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			}
			pprintf(PRI_SPAM, "%s atom %i here = {%lf, %lf, %lf}\n", 
					file.filename, *i, 
					object[*i].pos[0], object[*i].pos[1], object[*i].pos[2]);
			*i = *i + 1;
		}
	}
	fclose(inpars);
	return 0;
}
