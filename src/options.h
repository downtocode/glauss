#define revision "physengine"

struct option_struct* option;

struct option_struct {
	float dt;
	long sleepfor;
	unsigned short int avail_cores, oglmin, oglmax;
	int width, height;
	bool vsync, novid, fullogl;
	char fontname[200], filename[200];
};
