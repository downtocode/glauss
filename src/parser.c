#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physics.h"
#include "parser.h"

#define linkmax 100


int parser(data** object) {
	FILE *in = fopen ( "posdata.dat", "r" );
	int i, j, links, count = 0;
	char str[200], linktemp[linkmax];
	while(fgets (str, 200, in)!=NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %i", &i, &(*object)[i].pos[0], &(*object)[i].pos[1], \
				&(*object)[i].vel[0], &(*object)[i].vel[1], &(*object)[i].acc[0], &(*object)[i].acc[1], \
				&(*object)[i].mass, &(*object)[i].charge, &links);
				sprintf(linktemp, "%i", links);
				for(j = 1; j < linkmax + 1; j++) {
					if( linktemp[i] != NULL ) count += 1;
				}
				for(j = 1; j < count + 1; j++) {
					(*object)[i].linkwith[linktemp[i]] = 1;
				}
		}
	}
	return 0;
	fclose(in);
}
