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
#include "input/in_file.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "physics/physics_aux.h"

int in_probe_file(const char *filename)
{
	char str[500];
	int counter = 0;
	FILE *inprep = fopen(filename, "r");
	
	int filetype = 0;
	
	/* Put format specific quirks here. */
	if (strstr(filename, "xyz")!=NULL) {
		filetype = MOL_XYZ;
	} else if (strstr(filename, "pdb")!=NULL) {
		filetype = MOL_PDB;
	} else if (strstr(filename, "obj")!=NULL) {
		filetype = MOL_OBJ;
	} else {
		fprintf(stderr, "[IN] Filetype of %s not recognized!\n", filename);
		exit(1);
	}
	
	/* 
	 * XYZ files have total number of atoms in their first line.
	 * PDB files contain an incrementing index, but I'd like to avoid sscanf.
	 */
	if (filetype == MOL_XYZ) {
		fgets(str, sizeof(str), inprep);
		fclose(inprep);
		sscanf(str, "%i", &counter);
	} else if (filetype == MOL_PDB) {
		while (fgets(str, sizeof(str), inprep)) {
			if (!strstr(str, "#")) {
				if (!strncmp(str, "ATOM", 4))
					counter++;
			}
		}
	} else if (filetype == MOL_OBJ) {
		while (fgets (str, sizeof(str), inprep)!= NULL) {
			if (!strstr(str, "#")) {
				if (!strncmp(str, "v ", 2))
					counter++;
			}
		}
	}
	if (option->skip_model_vec) {
		counter/=option->skip_model_vec;
	}
	return counter;
}

int in_read_file(phys_obj *object, int *i, in_file *file)
{
	char str[500] = {0}, atom[2] = {0}, pdbtype[10], pdbatomname[10], pdbresidue[10];
	char pdbreschain;
	int filetype, pdbatomindex, pdbresidueseq, vec_counter = 0;
	float xpos, ypos, zpos, pdboccupy, pdbtemp, pdboffset;
	vec3 pos;
	FILE *inpars = fopen(file->filename, "r");
	
	/* Put format specific quirks here. */
	if (strstr(file->filename, "xyz")) {
		filetype = MOL_XYZ;
		/* Skip first two lines of XYZ files. */
		fgets(str, sizeof(str), inpars);
		fgets(str, sizeof(str), inpars);
	} else if (strstr(file->filename, "pdb")) {
		filetype = MOL_PDB;
	} else if (strstr(file->filename, "obj")) {
		filetype = MOL_OBJ;
	} else {
		fprintf(stderr, "Error! Filetype of %s not recognized!\n", file->filename);
		exit(1);
	}
	
	/* TODO: use something less primitive   */
	/* TODO: figure out what I mean by that */
	while (fgets (str, sizeof(str), inpars)) {
		/* Skip if needed */
		if (option->skip_model_vec && ++vec_counter < option->skip_model_vec) {
			continue;
		} else {
			vec_counter = 0;
		}
		if (!strstr(str, "#")) {
			if (filetype == MOL_XYZ) {
				sscanf(str, " %s  %f         %f         %f", atom, &xpos, &ypos,
					   &zpos);
			} else if (filetype == MOL_PDB) {
				if (!strncmp(str, "ATOM", 4)) {
					sscanf(str, "%s %i %s %s %c %i %f %f %f %f %f %s %f",\
							pdbtype, &pdbatomindex, pdbatomname, pdbresidue,
							&pdbreschain, &pdbresidueseq,\
							&xpos, &ypos, &zpos, &pdboccupy, &pdbtemp,
							atom, &pdboffset);
				} else continue;
			} else if (filetype == MOL_OBJ) {
				if (!strncmp(str, "v ", 2)) {
					sscanf(str, "v  %f %f %f", &xpos, &ypos, &zpos);
				} else continue;
			}
			object[*i].atomnumber = return_atom_num(atom);
			object[*i].id = *i;
			pos = (vec3){xpos, ypos, zpos};
			rotate_vec(&pos, &file->rot);
			object[*i].pos = file->scale*pos + file->inf->pos;
			object[*i].vel = file->inf->vel;
			object[*i].ignore = file->inf->ignore;
			object[*i].charge = file->inf->charge*option->elcharge;
			if (object[*i].atomnumber == 1) {
				object[*i].mass = 1.0;
				object[*i].radius = 0.05;
			} else if (object[*i].atomnumber == 6) {
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			} else {
				object[*i].atomnumber = 1;
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			}
			pprintf(PRI_SPAM, "%s atom %i here = {%lf, %lf, %lf}\n", 
					file->filename, *i, 
					object[*i].pos[0], object[*i].pos[1], object[*i].pos[2]);
			*i = *i + 1;
		}
	}
	fclose(inpars);
	return 0;
}
