#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include "physics.h"
#include "parser.h"

long double elcharge;

static char str[500], word[100], links[100], *linkstr;
static float value;
bool quiet;


int preparser(float *dt, long double *elcharge, int *width, int *height, int *boxsize, char fontname[100]) {
	static int count = 0;
	FILE *inprep = fopen ( "posdata.dat", "r" );
	FILE *inconf = fopen ( "simconf.conf", "r" );
	while(fgets (str, 500, inprep)!=NULL) {
		if (strstr(str, "#") == NULL) {
			count += 1;
		}
	}
	fclose(inprep);
	
	while(fgets (str, 200, inconf)!=NULL) {
		sscanf(str, "%s = \"%f\"", word, &value);
		if(strcmp(word, "dt") == 0) {
			*dt = value;
		}
		if(strcmp(word, "elcharge") == 0) {
			*elcharge = value;
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
	}
	
	fclose(inconf);
	return count;
}

int parser(data** object) {
	FILE *in = fopen ( "posdata.dat", "r" );
	static int i, j, link;
	int dimensions = 3;
	float pos[dimensions], vel[dimensions], bond, radius;
	long double mass, chargetemp;
	char ignflag;
	
	
	while(fgets (str, 500, in)!= NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %f %c \"%s\"", &i, &pos[0], &pos[1], &pos[2], &vel[0], \
			&vel[1], &vel[2], &mass, &chargetemp, &radius, &ignflag, links);
			for(j = 0; j < dimensions; j++) {
				(*object)[i].pos[j] = pos[j];
				(*object)[i].vel[j] = vel[j];
			}
			(*object)[i].mass = mass;
			(*object)[i].charge = (chargetemp*elcharge)/pow(10, 5);
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
	fclose(in);
	return 0;
}
