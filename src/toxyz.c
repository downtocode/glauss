#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "physics.h"
#include "toxyz.h"
#include "elements.h"

char extension[] = "xyz", file[] = "config", filetodump[120];

int toxyz(int obj, data *object, float timestep) {
	char atomchar[2];
	sprintf(filetodump, "%s_%0.2f.%s", file, timestep, extension);
	FILE *out = fopen ( filetodump, "w" );
	fprintf(stderr, "Created %s\n", filetodump);
	fprintf(out, "%i\n", obj);
	fprintf(out, "#Current dump = %0.2f\n", timestep);
	for(int i = 1; i < obj + 1; i++) {
		return_atom_char(atomchar, object[i].atomnumber);
		fprintf(out, "%s %f %f %f\n", atomchar, object[i].pos[0], object[i].pos[1], object[i].pos[2]);
	}
	fclose(out);
	return 0;
}
