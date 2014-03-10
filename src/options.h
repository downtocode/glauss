struct option_struct {
    int width, height;
	unsigned int obj, chosen, dumplevel;
	unsigned short int avail_cores, oglmin, oglmax;
	float boxsize, dt, radius;
	char fontname[200], filename[200];
	long sleepfor;
	long double elcharge, gconst, epsno;
	bool novid, vsync, quiet, stop, enforced, quit;
	bool nowipe, random, flicked, dumped, fullogl, restart;
};
