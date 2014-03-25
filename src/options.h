#include <stdbool.h>
#define revision "physengine"

struct option_struct* option;

struct option_struct {
	float dt;
	long sleepfor;
	long double elcharge, gconst, epsno;
	unsigned short int avail_cores, oglmin, oglmax, verbosity;
	int width, height;
	bool vsync, novid, fullogl;
	char fontname[200], filename[200];
};
