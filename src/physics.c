#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include "physics.h"


//Constants
#define perm 8.854187817
#define g 6.67384
#define elcharge 1.602176565

//Static because you might want to call integrate in a for loop and then you'd be in trouble.
static int i, j;

//Using black magic to transfer this data between.
int obj, width, height;
float spring = 3;

//Using float precision results in a much reduced CPU activity. For a 100 objects it goes down from 15(double) to barely 4(float).
float mag, dist, dt = 0.01, Gconst, pi, epsno;
v4sf mtemp, accprev, vecdist, forceconst = {0, -9.81};


int initphys(data** object) {
	*object = malloc((1000*obj) * sizeof(object));
	
	Gconst = g/pow(12, 10);
	pi = acos(-1);
	epsno = perm/pow(12, 10);
	
	/*for(i = 1; i < obj + 1; i++) {
		(*object)[i].pos[0] = ((float)rand()/RAND_MAX)*width + 10*i;
		(*object)[i].pos[1] = ((float)rand()/RAND_MAX)*height + 10*i;
		(*object)[i].mass = 0.000001;
		(*object)[i].charge = (400*elcharge)/pow(19, 10);
		if( ((float)rand()/RAND_MAX) < 0.5 ) (*object)[i].charge *= -1;
	}*/
	
	
	(*object)[1].mass = 1;
	(*object)[1].pos[0] = 20 + width/2;
	(*object)[1].pos[1] = height/2;
	(*object)[1].vel[0] = 12;
	(*object)[1].vel[1] = 6;
	(*object)[1].charge = 0;
	(*object)[1].link = 2;
	
	(*object)[2].mass = 1;
	(*object)[2].pos[0] = width/2;
	(*object)[2].pos[1] = height/2;
	(*object)[2].vel[0] = 0;
	(*object)[2].vel[1] = 0;
	(*object)[2].charge = 0;
	(*object)[2].link = 0;
	
	(*object)[3].mass = 1;
	(*object)[3].pos[0] = width/2;
	(*object)[3].pos[1] = 20 + height/2;
	(*object)[3].vel[0] = 6;
	(*object)[3].vel[1] = 12;
	(*object)[3].charge = 0;
	(*object)[3].link = 2;
	
	/*(*object)[4].mass = 0.001;
	(*object)[4].pos[0] = width/2;
	(*object)[4].pos[1] = 300;
	(*object)[4].vel[0] = 0;
	(*object)[4].vel[1] = 0;
	(*object)[4].charge = (400*elcharge)/pow(19, 10);
	
	(*object)[5].mass = 0.001;
	(*object)[5].pos[0] = width/2;
	(*object)[5].pos[1] = 400;
	(*object)[5].vel[0] = 0;
	(*object)[5].vel[1] = 0;
	(*object)[5].charge = (400*elcharge)/pow(19, 10);
	
	(*object)[6].mass = 0.001;
	(*object)[6].pos[0] = width/2;
	(*object)[6].pos[1] = 500;
	(*object)[6].vel[0] = 0;
	(*object)[6].vel[1] = 0;
	(*object)[6].charge = (400*elcharge)/pow(19, 10);
	
	(*object)[7].mass = 0.001;
	(*object)[7].pos[0] = width/2;
	(*object)[7].pos[1] = 600;
	(*object)[7].vel[0] = 0;
	(*object)[7].vel[1] = 0;
	(*object)[7].charge = (400*elcharge)/pow(19, 10);
	
	(*object)[8].mass = 1000;
	(*object)[8].pos[0] = width/1.5;
	(*object)[8].pos[1] = height/2;
	(*object)[8].vel[0] = 0;
	(*object)[8].vel[1] = 0;
	(*object)[8].charge = -(40000*elcharge)/pow(19, 10);
	
	(*object)[9].mass = 1000;
	(*object)[9].pos[0] = width/3;
	(*object)[9].pos[1] = height/2;
	(*object)[9].vel[0] = 14;
	(*object)[9].vel[1] = 10;
	(*object)[9].charge = -(40000*elcharge)/pow(19, 10);*/
	return 0;
}


int integrate(data* object) {
	for(i = 1; i < obj + 1; i++) {
		mtemp[0] = mtemp[1] = object[i].mass;
		if(object[i].pos[0] > width) object[i].pos[0] = 0;
		if(object[i].pos[0] < 0) object[i].pos[0] = width;
		if(object[i].pos[1] < 0 || object[i].pos[1] > height) {
			object[i].vel[1] = -object[i].vel[1];
		}
		object[i].pos += (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		for(j = 1; j < obj + 1; j++) {
			if(i != j) {
				vecdist = object[j].pos - object[i].pos;
				//Why 3/2? We're multiplying both Fgrv and Fele by a vector which we normalize, so we just need to calculate this once.
				mag = pow(3/2, (vecdist[0]*vecdist[0] + vecdist[1]*vecdist[1]));
				dist = sqrt((vecdist[0]*vecdist[0] + vecdist[1]*vecdist[1]));
				object[i].Fgrv += vecdist*((float)(Gconst*object[i].mass*object[j].mass)/(mag));
				object[i].Fele += -vecdist*((object[i].charge*object[j].charge)/(4*pi*epsno*mag));
				if(object[i].link == j) {
					//Super secret link feature disabled.
					object[i].Flink += (vecdist/dist)*((spring)*(dist - 20));
				}
			}
		}
		
		object[i].Ftot = forceconst + object[i].Fgrv + object[i].Fele + object[i].Flink;
		accprev = object[i].acc;
		
		object[i].acc = (object[i].Ftot)/(mtemp);
		object[i].vel += ((dt)/2)*(object[i].acc + accprev);
		object[i].Fgrv[0] = object[i].Fgrv[1] = object[i].Fele[0] = object[i].Fele[1] = object[i].Flink[0] = object[i].Flink[1] = 0;
	}
	return 0;
}
