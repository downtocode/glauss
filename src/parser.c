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
	int i, link;
	long double chargetemp;
	char str[200], links[500];
	char *linkstr;
	while(fgets (str, 200, in)!= NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %s", &i, &(*object)[i+1].pos[0], &(*object)[i+1].pos[1], \
				&(*object)[i+1].vel[0], &(*object)[i+1].vel[1], &(*object)[i+1].acc[0], &(*object)[i+1].acc[1], \
				&(*object)[i+1].mass, &chargetemp, links);
				(*object)[i].charge = (chargetemp*elcharge)/pow(10, 7);
				
				linkstr = strtok(links,",");
				while(linkstr != NULL) {
					sscanf(linkstr, "%i", &link);
					(*object)[i].linkwith[link] = 1;
					linkstr = strtok(NULL,",");
				}
		}
	}
	fclose(in);
	return 0;
}
