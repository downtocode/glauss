#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include "physics.h"
#include "parser.h"

#define elcharge 1.602176565

static char str[500],links[100], *linkstr;


int preparser() {
	static int count = 0;
	FILE *inprep = fopen ( "posdata.dat", "r" );
	while(fgets (str, 500, inprep)!=NULL) {
		if (strstr(str, "#") == NULL) {
			count += 1;
		}
	}
	printf("Objects: %i\n", count);
	fclose(inprep);
	return count;
}

int parser(data** object) {
	FILE *in = fopen ( "posdata.dat", "r" );
	static int i, j, link;
	int dimensions = 3;
	float pos[dimensions], vel[dimensions], acc[dimensions], bond;
	long double mass, chargetemp;
	char ignflag;
	
	
	while(fgets (str, 500, in)!= NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %f %f %f %Lf %Lf %c \"%s\"", &i, &pos[0], &pos[1], &pos[2], &vel[0], \
			&vel[1], &vel[2], &acc[0], &acc[1], &acc[2], &mass, &chargetemp, &ignflag, links);
			for(j = 0; j < dimensions; j++) {
				(*object)[i].pos[j] = pos[j];
				(*object)[i].vel[j] = vel[j];
				(*object)[i].acc[j] = acc[j];
			}
			(*object)[i].mass = mass;
			(*object)[i].charge = (chargetemp*elcharge)/pow(10, 6);
			(*object)[i].ignore = ignflag;
			
			printf("Object %i links: ", i);
			linkstr = strtok(links,",");
				while(linkstr != NULL) {
					sscanf(linkstr, "%i-%f", &link, &bond);
					printf("%i:%f, ", link, bond);
					(*object)[i].linkwith[link] = bond;
					linkstr = strtok(NULL,",");
				}
			printf(" \n");
		}
	}
	fclose(in);
	return 0;
}
