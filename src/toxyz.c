#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "physics.h"
#include "toxyz.h"

char extension[20] = "xyz", file[80] = "config", filetodump[120];
static int dumpcount = 0;


int toxyz(int obj, data *object) {
	dumpcount++;
	sprintf(filetodump, "%s_%05i.%s", file, dumpcount, extension);
	FILE *out = fopen ( filetodump, "w" );
	fprintf(stderr, "Created %s\n", filetodump);
	fprintf(out, "%i\n", obj);
	fprintf(out, "#Current dump = %i\n", dumpcount);
	for(int i = 1; i < obj + 1; i++) {
		fprintf(out, "N %f %f %f\n", object[i].pos[0], object[i].pos[1], object[i].pos[2]);
	}
	fclose(out);
	return 0;
}
