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
	float bond;
	long double chargetemp;
	char str[500], links[500], bonds[500];
	char *linkstr;
	char *bondstr;
	while(fgets (str, 200, in)!= NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %i \"%s\"", &i, &(*object)[i+1].pos[0], &(*object)[i+1].pos[1], \
				&(*object)[i+1].vel[0], &(*object)[i+1].vel[1], &(*object)[i+1].acc[0], &(*object)[i+1].acc[1], \
				&(*object)[i+1].mass, &chargetemp, &(*object)[i+1].ignore, links);
				(*object)[i].charge = (chargetemp*elcharge)/pow(10, 7);
				
				printf("Object %i links: ", i);
				
				
				//Remove this line. It's not related to anything. Compile. Segfault. Scratch head. Segfault. Segfault. Lose hair. Segfault. Why?
				//gdb fails me. Really, why? Removing the variables changes NOTHING. WHAT?
				bondstr = strtok(bonds,",");
				
				
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
