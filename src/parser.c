#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include "physics.h"
#include "parser.h"

#define linkmax 100
#define elcharge 1.602176565

int parser(data** object) {
	FILE *in = fopen ( "posdata.dat", "r" );
	int i, j, links, count = 0;
	long double chargetemp;
	char str[200];
	while(fgets (str, 200, in)!=NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %i", &i, &(*object)[i+1].pos[0], &(*object)[i+1].pos[1], \
				&(*object)[i+1].vel[0], &(*object)[i+1].vel[1], &(*object)[i+1].acc[0], &(*object)[i+1].acc[1], \
				&(*object)[i+1].mass, &chargetemp, &links);
				(*object)[i].charge = (chargetemp*elcharge)/pow(7, 10);
				(*object)[i].linkwith[links] = 1;
		}
	}
	return 0;
	fclose(in);
}
