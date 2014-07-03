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
#include "in_molecule.h"
#include "options.h"
#include "msg_phys.h"
#include "physics_aux.h"

int probefile(const char *molfile)
{
	char str[500];
	int counter = 0;
	FILE *inprep = fopen(molfile, "r");
	
	int moltype = 0;
	
	/* Put format specific quirks here. */
	if(strstr(molfile, "xyz")!=NULL) {
		moltype = MOL_XYZ;
	} else if(strstr(molfile, "pdb")!=NULL) {
		moltype = MOL_PDB;
	} else {
		fprintf(stderr, "Error! Filetype of %s not recognized!\n", molfile);
		exit(1);
	}
	
	/* 
	 * XYZ files have total number of atoms in their first line.
	 * PDB files contain an incrementing index, but I'd like to avoid using sscanf here.
	 */
	if(moltype == MOL_XYZ) {
		fgets(str, sizeof(str), inprep);
		fclose(inprep);
		sscanf(str, "%i", &counter);
		return counter;
	} else if(moltype == MOL_PDB) {
		while(fgets (str, sizeof(str), inprep)!= NULL) {
			if(strstr(str, "#") == NULL) {
				if(strncmp(str, "ATOM", 4)==0) counter++;
			}
		}
		return counter;
	}
	return 0;
}

int readmolecule(data *object, data *buffer, const char *molfile, int *i)
{
	char str[500], atom[2], pdbtype[10], pdbatomname[10], pdbresidue[10], pdbreschain;
	int filetype, pdbatomindex, pdbresidueseq;
	float xpos, ypos, zpos, pdboccupy, pdbtemp, pdboffset;
	FILE *inpars = fopen(molfile, "r");
	
	/* Put format specific quirks here. */
	if(strstr(molfile, "xyz")!=NULL) {
		filetype = MOL_XYZ;
		/* Skip first two lines of XYZ files. */
		fgets(str, sizeof(str), inpars);
		fgets(str, sizeof(str), inpars);
	} else if(strstr(molfile, "pdb")!=NULL) {
		filetype = MOL_PDB;
	} else {
		fprintf(stderr, "Error! Filetype of %s not recognized!\n", molfile);
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
			object[*i].id = *i;
			/* By specifications XYZ and PDB files default to float */
			object[*i].pos[0] = (double)xpos + buffer->pos[0];
			object[*i].pos[1] = (double)ypos + buffer->pos[1];
			object[*i].pos[2] = (double)zpos + buffer->pos[2];
			object[*i].vel[0] = buffer->vel[0];
			object[*i].vel[1] = buffer->vel[1];
			object[*i].vel[2] = buffer->vel[2];
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
				object[*i].charge = 0;
				object[*i].ignore = 0;
				object[*i].mass = 12.0;
				object[*i].radius = 0.1;
			}
			pprintf(PRI_SPAM, "%s atom %i here = {%lf, %lf, %lf}\n", molfile, *i, object[*i].pos[0], object[*i].pos[1], object[*i].pos[2]);
			*i = *i + 1;
		}
	}
	fclose(inpars);
	return 0;
}
