#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "physics.h"
#include "parser.h"


static long double anothervar;
long double elcharge;

static char str[100], word[100], links[100], variable[100], *linkstr;
static float value, base, power;
bool quiet, random;
int randobjs;
float velmax, massrand, chargerand, sizerand;


int preparser(float *dt, long double *elcharge, long double *gconst, long double *epsno, int *width, int *height, int *boxsize, char fontname[100]) {
	static int count = 0;
	FILE *inconf = fopen ( "simconf.conf", "r" );
	while(fgets (str, 200, inconf)!=NULL) {
		sscanf(str, "%s = \"%f\"", word, &value);
		if(strcmp(word, "dt") == 0) {
			*dt = value;
		}
		if(strcmp(word, "elcharge") == 0) {
			sscanf(str, "%s = \"%100[^\"]\"", word, variable);
			sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
			*elcharge = anothervar*pow(base, power);
		}
		if(strcmp(word, "gconst") == 0) {
			sscanf(str, "%s = \"%100[^\"]\"", word, variable);
			sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
			*gconst = anothervar*pow(base, power);
		}
		if(strcmp(word, "epsno") == 0) {
			sscanf(str, "%s = \"%100[^\"]\"", word, variable);
			sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
			*epsno = anothervar*pow(base, power);
		}
		if(strcmp(word, "width") == 0) {
			*width = (int)value;
		}
		if(strcmp(word, "height") == 0) {
			*height = (int)value;
		}
		if(strcmp(word, "boxsize") == 0) {
			*boxsize = (int)value;
		}
		if(strcmp(word, "fontname") == 0) {
			sscanf(str, "%s = \"%100[^\"]\"", word, fontname);
		}
		if(strcmp(word, "random") == 0) {
			random = (bool)value;
		}
		if(strcmp(word, "randobjs") == 0) {
			randobjs = (int)value;
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
	}
	fclose(inconf);
	
	if( random == 1 ) {
		srand(time(NULL));
		count = randobjs;
	} else { 
		FILE *inprep = fopen ( "posdata.dat", "r" );
		count = 0;
		while(fgets (str, 500, inprep)!=NULL) {
			if (strstr(str, "#") == NULL) {
				count += 1;
			}
		}
		fclose(inprep);
	}
	
	return count;
}

int parser(data** object) {
	FILE *in = fopen ( "posdata.dat", "r" );
	static int i, j, link;
	int dimensions = 3;
	float pos[dimensions], vel[dimensions], bond, radius;
	long double mass, chargetemp;
	char ignflag;
	
	if( random == 0 ) {
		while(fgets (str, 500, in)!= NULL) {
			if (strstr(str, "#") == NULL) {
				sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %f %c \"%s\"", &i, &pos[0], &pos[1], &pos[2], &vel[0], \
				&vel[1], &vel[2], &mass, &chargetemp, &radius, &ignflag, links);
				for(j = 0; j < dimensions; j++) {
					(*object)[i].pos[j] = pos[j];
					(*object)[i].vel[j] = vel[j];
				}
				(*object)[i].mass = mass;
				(*object)[i].charge = chargetemp*elcharge;
				(*object)[i].ignore = ignflag;
				(*object)[i].radius = radius;
				
				if( quiet == 0 ) printf("Object %i links: ", i);
				linkstr = strtok(links,",");
				while(linkstr != NULL) {
					sscanf(linkstr, "%i-%f", &link, &bond);
					if( quiet == 0 ) printf("%i:%f, ", link, bond);
					(*object)[i].linkwith[link] = bond;
					linkstr = strtok(NULL,",");
				}
				if( quiet == 0 ) printf(" \n");
			}
		}
	} else if ( random == 1 ) {
		for(i = 1; i < randobjs + 1; i++) {
			(*object)[i].pos = (v4sf){((float)rand()/(float)RAND_MAX)*1000, ((float)rand()/(float)RAND_MAX)*600, 1};
			(*object)[i].vel = (v4sf){(((float)rand()/(float)RAND_MAX) - 0.5)*velmax, (((float)rand()/(float)RAND_MAX) - 0.5)*velmax, 0};
			(*object)[i].mass = (((float)rand()/(float)RAND_MAX))*massrand;
			(*object)[i].charge = (((float)rand()/(float)RAND_MAX) - 0.5)*chargerand*elcharge*2;
			(*object)[i].radius = (((float)rand()/(float)RAND_MAX))*sizerand + 1;
		}
	}
	fclose(in);
	return 0;
}
