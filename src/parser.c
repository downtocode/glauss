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
#include <tgmath.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "physics.h"
#include "parser.h"
#include "options.h"
#include "molreader.h"
#include "msg_phys.h"
#include "elements.h"

/*	Static variables	*/
static float velmax, massrand, chargerand, sizerand;

int preparser()
{
	int count = 0;
	char word[200], variable[200], namebuff[200], str[200];
	long double anothervar;
	float value, base, power;
	FILE *inconf = fopen ("simconf.ini", "r");
	while(fgets (str, sizeof(str), inconf)!=NULL) {
		if(strncmp(str, "#", 1)!=0) {
			sscanf(str, "%s = \"%f\"", word, &value);
			if(strcmp(word, "dt") == 0) {
				option->dt = value;
			}
			if(strcmp(word, "threads") == 0) {
				if(!option->avail_cores) option->avail_cores = value;
			}
			if(strcmp(word, "elcharge") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, variable);
				sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
				if(anothervar != 0.0) {
					option->elcharge = anothervar*pow(base, power);
				} else {
					option->noele = 1;
					option->elcharge = 0.0;
					option->epsno = 0.0;
				}
			}
			if(strcmp(word, "gconst") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, variable);
				sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
				if(anothervar != 0.0) {
					option->gconst = anothervar*pow(base, power);
				} else {
					option->nogrv = 1;
					option->gconst = 0.0;
				}
			}
			if(strcmp(word, "epsno") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, variable);
				sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
				if(anothervar != 0.0) {
					option->epsno = anothervar*pow(base, power);
				} else {
					option->noele = 1;
					option->elcharge = 0.0;
					option->epsno = 0.0;
				}
			}
			if(strcmp(word, "width") == 0) {
				option->width = (unsigned int)value;
			}
			if(strcmp(word, "height") == 0) {
				option->height = (unsigned int)value;
			}
			if(strcmp(word, "fontname") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, option->fontname);
			}
			if(strcmp(word, "random") == 0) {
				option->moderandom = (bool)value;
			}
			if(strcmp(word, "randobjs") == 0) {
				count = (int)value;
			}
			if(strcmp(word, "velmax") == 0) {
				velmax = value;
			}
			if(strcmp(word, "massrand") == 0) {
				massrand = value;
			}
			if(strcmp(word, "chargerand") == 0) {
				chargerand = value;
			}
			if(strcmp(word, "sizerand") == 0) {
				sizerand = value;
			}
			if(strcmp(word, "verbosity") == 0) {
				/*	6-Default value	*/
				if(option->verbosity == 6) option->verbosity = (unsigned int)value;
			}
			if(strcmp(word, "posdata") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, namebuff);
			}
		} else {
			memset(str, 0, sizeof(str));
		}
	}
	fclose(inconf);
	
	memset(str, 0, sizeof(str));
	
	if(option->moderandom == 1) {
		srand(time(NULL));
	} else {
		count = 0;
		if(access(option->filename, F_OK) == -1) {
			pprintf(PRI_VERYHIGH, "[\033[033m Warning! \033[0m] Argument/default set filename %s not found! Trying %s from configuration file...", option->filename, namebuff);
			if(access(namebuff, F_OK) == 0) {
				strcpy(option->filename, namebuff);
				fprintf(stderr, " Success!\n");
			} else {
				fprintf(stderr, " Fail!\n");
				return 0;
			}
		}
		FILE *inprep = fopen(option->filename, "r");
		while(fgets(str, sizeof(str), inprep)!=NULL) {
			if(strncmp(str, "#!", 2)==0) {
				char molfile[200], molname[180], moltype[20];
				sscanf(str, "#!%s %s", moltype, molname);
				sprintf(molfile, "./resources/molecules/%s.%s", molname, moltype);
				if(access(molfile, F_OK) == 0) {
					int atoms = probefile(molfile, moltype);
					pprintf(7, "File \"%s\" has %i atoms\n", molfile, atoms);
					count += atoms;
				} else {
					fprintf(stderr, "File \"%s\" not found!\n", molfile);
					exit(1);
				}
			} else if(strncmp(str, "#", 1)!=0) {
				count += 1;
			}
		}
		fclose(inprep);
	}
	memset(str, 0, sizeof(str));
	return count;
}

int parser(data** object, char filename[200])
{
	int i = 1, link;
	char links[200], *linkstr, ignflag;
	char molfile[200], molname[180], moltype[20], str[200];
	float posx, posy, posz, velx, vely, velz, bond, radius;
	double mass, chargetemp;
	
	FILE *in = fopen ( option->filename, "r" );
	
	if(option->moderandom == 0) {
		pprintf(9, "	Position		Velocity   |   Mass   |  Charge  |  Radius  |Ign|   Links:\n");
		while(fgets(str, sizeof(str), in)!= NULL) {
			if(strncmp(str, "#", 1)!=0) {
				sscanf(str, "%f %f %f %f %f %f %lf %lf %f %c \"%s\"", &posx, &posy, &posz, &velx, \
				&vely, &velz, &mass, &chargetemp, &radius, &ignflag, links);
				
				(*object)[i].pos[0] = posx;
				(*object)[i].pos[1] = posy;
				(*object)[i].pos[2] = posz;
				(*object)[i].vel[0] = velx;
				(*object)[i].vel[1] = vely;
				(*object)[i].vel[2] = velz;
				/*
				 * Clang is strongly opposed to this assignment.
				 * (*object)[i].pos = (v4sd){ posx, posy, posz };
				 * (*object)[i].vel = (v4sd){ velx, vely, velz };
				 */
				(*object)[i].mass = mass;
				(*object)[i].charge = chargetemp*option->elcharge;
				(*object)[i].ignore = 0;
				(*object)[i].atomnumber = 0;
				(*object)[i].radius = radius;
				
				pprintf(PRI_SPAM, "(%0.2f, %0.2f, %0.2f)	(%0.2f, %0.2f, %0.2f) | %0.2E | %0.2E | %f | %i | ", \
				(*object)[i].pos[0], (*object)[i].pos[1], (*object)[i].pos[2], (*object)[i].vel[0], (*object)[i].vel[1], \
				(*object)[i].vel[2], (*object)[i].mass, (*object)[i].charge, (*object)[i].radius, (*object)[i].ignore);
				
				if(!option->nosprings && links[0] != 0 ) {
					linkstr = strtok(links,",");
					while(linkstr != NULL) {
						sscanf(linkstr, "%i-%f", &link, &bond);
						pprintf(PRI_SPAM, "%i - %f ", link, bond);
						(*object)[i].links[link] = bond;
						linkstr = strtok(NULL,",");
					}
					bond = link = 0;
					memset(links, 0, sizeof(links));
				}
				pprintf(PRI_SPAM, " \n");
				i++;
			}
			if(strncmp(str, "#!", 2)==0) {
				sscanf(str, "#!%s %s %f %f %f %f %f %f", moltype, molname, &posx, &posy, &posz, &velx, &vely, &velz);
				sprintf(molfile, "./resources/molecules/%s.%s", molname, moltype);
				readmolecule(molfile, moltype, *object, (v4sd){ posx, posy, posz }, (v4sd){ velx, vely, velz }, &i);
			}
		}
	} else {
		for(i = 1; i < option->obj + 1; i++) {
			double alpha = 2*((((double)rand()/(double)RAND_MAX))*acos(-1));
			double beta = ((((double)rand()/(double)RAND_MAX))*acos(-1));
			radius = (((double)rand()/(double)RAND_MAX))*40;
			
			(*object)[i].pos[0] = radius*sin(alpha)*cos(beta);
			(*object)[i].pos[1] = radius*sin(alpha)*sin(beta);
			(*object)[i].pos[2] = radius*cos(alpha);
				
			
			//(*object)[i].vel[0] = (((double)rand()/(double)RAND_MAX) - 0.5)*velmax;
			//(*object)[i].vel[1] = (((double)rand()/(double)RAND_MAX) - 0.5)*velmax;
			//(*object)[i].vel[2] = (((double)rand()/(double)RAND_MAX) - 0.5)*velmax;
			
			(*object)[i].mass = (((double)rand()/(double)RAND_MAX))*massrand;
			(*object)[i].charge = 0;
			(*object)[i].radius = 0.07;
			(*object)[i].ignore = 0;
			(*object)[i].atomnumber = 0;
		}
	}
	fclose(in);
	return 0;
}

char* readshader(const char* filename)
{
	FILE* input = fopen(filename, "r");
	if(input == NULL) return NULL;
	
	if(fseek(input, 0, SEEK_END) == -1) return NULL;
	long size = ftell(input);
	if(size == -1) return NULL;
	if(fseek(input, 0, SEEK_SET) == -1) return NULL;
	
	char *content = malloc((size_t)size +1); 
	if(content == NULL) return NULL;
	
	fread(content, 1, (size_t)size, input);
	
	if(ferror(input)) {
		free(content);
		return NULL;
	}
	
	fclose(input);
	content[size] = '\0';
	return content;
}
