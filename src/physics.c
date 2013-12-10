#include <stdio.h>
#include <stdlib.h>
//#include "parser.h"


typedef float v4sf __attribute__ ((vector_size (16)));

typedef struct {
	v4sf pos, vel, acc, Ftot, Fgrv, Fele;
	double mass;
} data;

v4sf forceconst = {0, -9.81};
v4sf accprev, diff, mtemp;

data object[6];

int i, j, width, height, obj;
float dt = 0.01;

int initphys() {
	object[1].mass = 1;
	object[1].pos[0] = 11;
	object[1].pos[1] = 31;
	object[1].vel[0] = 12;
	object[1].vel[1] = 44;
	
	object[2].mass = 1;
	object[2].pos[0] = 22;
	object[2].pos[1] = 222;
	object[2].vel[0] = 22;
	object[2].vel[1] = 5;
	
	object[3].mass = 1;
	object[3].pos[0] = 41;
	object[3].pos[1] = 112;
	object[3].vel[0] = 12;
	object[3].vel[1] = 23;
	
	object[4].mass = 1;
	object[4].pos[0] = 56;
	object[4].pos[1] = 30;
	object[4].vel[0] = -12;
	object[4].vel[1] = -26;
	
	object[5].mass = 1;
	object[5].pos[0] = 81;
	object[5].pos[1] = 2;
	object[5].vel[0] = -21;
	object[5].vel[1] = 23;
	
	object[6].mass = 1000;
	object[6].pos[0] = 221;
	object[6].pos[1] = 222;
	object[6].vel[0] = 52;
	object[6].vel[1] = 53;
	return 0;
}


float integrate() {
	//printf("Width: %i, Height: %i, Obj: %i", width, height, obj);
	for(i = 1; i < obj + 1; i++) {
		//setting the bounds for a box. Do want a periodic boundary sometime down the road though...
		mtemp[0] = mtemp[1] = object[i].mass;
		if(object[i].pos[0] > width || object[i].pos[0] < 0) {
			object[i].vel[0] = -object[i].vel[0];
		}
		if(object[i].pos[1] < 0 || object[i].pos[1] > height) {
			object[i].vel[1] = -object[i].vel[1];
		}
		object[i].pos = object[i].pos + (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		object[i].Ftot = forceconst;
		//lets just naively assume this for now, k?
		accprev = object[i].acc;
		object[i].acc = (object[i].Ftot)/(mtemp);
		object[i].vel = object[i].vel + ((dt)/2)*(object[i].acc + accprev);
	}
	return object[1].pos[1];
}
