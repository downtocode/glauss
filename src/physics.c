#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include "physics.h"


/* Constants */
#define perm 8.854187817
#define g 6.67384
#define elcharge 1.602176565
#define dalton 1.660538921

/* Static because you might want to call integrate in a for loop and then you'd be in trouble. */
static int i, j;

int obj, width, height, objcount;
float spring = 500, dt;

long double mag, Gconst, pi, epsno;


v4sf accprev, vecdist, centemp, forceconst = {0, -9.81};


int initphys(data** object) {

	*object = malloc(600*sizeof(**object));
	
	Gconst = g/pow(10, 10);
	pi = acos(-1);
	epsno = perm/pow(10, 12);
	return 0;
}

int findstructs(data* object) {
	//Determine links to get the approximate centers
	
	for(i = 1; i < obj + 1; i++) {
		for( j = 1; j < obj + 1; j++ ) {
			objcount++;
			centemp += object[i].pos;
			if( object[i].linkwith[j] == 0 ) break;
		}
	}
	centemp /= (float)objcount;
	object[obj + 1].pos = centemp;
	object[obj + 1].center = 1;
	objcount = 0;
	return 0;
}


int integrate(data* object) {
	
	findstructs(object);
	
	for(i = 1; i < obj + 1; i++) {
		if(object[i].ignore == 1) continue;
		
		
		if(object[i].pos[0] < 0) {
			object[i].pos[0] = width;
		}
		
		if(object[i].pos[0] > width) {
			object[i].pos[0] = 0;
		}
		
		
		if(object[i].pos[1] < 0 || object[i].pos[1] > height) {
			object[i].vel[1] = -object[i].vel[1];
		}
		
		object[i].pos += (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		for(j = 1; j < obj + 1; j++) {
			if(i != j) {
				vecdist = object[j].pos - object[i].pos;
				mag = sqrt((long double)(vecdist[0]*vecdist[0] + vecdist[1]*vecdist[1]));
				vecdist /= (float)mag;
				
				object[i].Fgrv += vecdist*((float)((Gconst*object[i].mass*object[j].mass)/(mag*mag)));
				//future:use whole joints instead of individual stuff
				object[i].Fele += -vecdist*((float)((object[i].charge*object[j].charge)/(4*pi*epsno*mag*mag)));
				
				if( object[i].linkwith[j] != 0 ) {
					object[i].Flink += vecdist*((spring)*((float)mag - object[i].linkwith[j])*(float)0.2);
				}
			}
		}
		
		object[i].Ftot = forceconst + object[i].Fgrv + object[i].Fele + object[i].Flink;
		accprev = object[i].acc;
		
		object[i].acc = (object[i].Ftot)/(float)object[i].mass;
		object[i].vel += ((dt)/2)*(object[i].acc + accprev);
		object[i].Fgrv = object[i].Fele = centemp = object[i].Flink = (v4sf){0, 0};
	}
	return 0;
}
