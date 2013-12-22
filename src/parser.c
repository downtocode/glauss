#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include "physics.h"
#include "parser.h"

#define linkmax 100
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
	int i, links;
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
	fclose(in);
	return 0;
}
