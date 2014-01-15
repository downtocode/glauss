#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include "physics.h"
#include "parser.h"

#define elcharge 1.602176565


int preparser() {
	static int count = 0;
	FILE *inprep = fopen ( "posdata.dat", "r" );
	static char temp[200];
	while(fgets (temp, 200, inprep)!=NULL) {
		if (strstr(temp, "#") == NULL) {
			count += 1;
		}
	}
	printf("Objects: %i\n", count);
	fclose(inprep);
	return count;
}

int parser(data** object) {
	FILE *in = fopen ( "posdata.dat", "r" );
	int i, j, link;
	char str[500], links[500];
	char *linkstr;
	int dimensions = 2;
	float pos[dimensions], vel[dimensions], acc[dimensions], bond;
	long double mass, chargetemp;
	char ignflag;
	
	
	while(fgets (str, 200, in)!= NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %c \"%s\"", &i, &pos[0], &pos[1], &vel[0], \
			&vel[1], &acc[0], &acc[1], &mass, &chargetemp, &ignflag, links);
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
