#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include "physics.h"


/* Constants */
#define perm 8.854187817
#define g 6.67384

/* Static because you might want to call integrate in a for loop and then you'd be in trouble. */
static int i, j;

int obj, width, height, objcount;
float spring = 500, dt;

long double mag, Gconst, pi, epsno, elcharge;


v4sf accprev, vecnorm, vectang, centemp, forceconst = {0, -9.81};
float v1norm, v1tang, v2norm, v2tang;


int initphys(data** object) {

	*object = malloc((obj+20)*sizeof(**object));
	
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
	object[obj + 1].radius = 12;
	objcount = 0;
	return 0;
}


int integrate(data* object) {
	findstructs(object);
	
	for(i = 1; i < obj + 1; i++) {
		if(object[i].ignore == 1) continue;
		
		
		if(object[i].pos[0] < 0 || object[i].pos[0] > width) {
			object[i].vel[0] = -object[i].vel[0];
		}
		
		if(object[i].pos[1] < 0 || object[i].pos[1] > height) {
			object[i].vel[1] = -object[i].vel[1];
		}
		
		object[i].pos += (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		for(j = 1; j < obj + 1; j++) {
			if(i != j) {
				vecnorm = object[j].pos - object[i].pos;
				mag = sqrt((long double)(vecnorm[0]*vecnorm[0] + vecnorm[1]*vecnorm[1]));
				vecnorm /= (float)mag;
				
				object[i].Fgrv += vecnorm*((float)((Gconst*object[i].mass*object[j].mass)/(mag*mag)));
				//future:use whole joints instead of individual stuff
				object[i].Fele += -vecnorm*((float)((object[i].charge*object[j].charge)/(4*pi*epsno*mag*mag)));
				
				if( object[i].linkwith[j] != 0 ) {
					object[i].Flink += vecnorm*((spring)*((float)mag - object[i].linkwith[j])*(float)0.2);
				}
				if( mag < object[i].radius + object[j].radius ) {
					vectang[0] = -vecnorm[1];
					vectang[1] = vecnorm[0];
					v1norm = object[i].vel[0]*vecnorm[0] + object[i].vel[1]*vecnorm[1];
					v1tang = object[i].vel[0]*vectang[0] + object[i].vel[1]*vectang[1];
					v2norm = object[j].vel[0]*vecnorm[0] + object[j].vel[1]*vecnorm[1];
					v2tang = object[j].vel[0]*vectang[0] + object[j].vel[1]*vectang[1];
					
					v1norm = (v1norm*(object[i].mass-object[j].mass) + 2*object[j].mass*v2norm)/(object[i].mass+object[j].mass);
					v2norm = (v2norm*(object[j].mass-object[i].mass) + 2*object[i].mass*v1norm)/(object[j].mass+object[i].mass);
					
					object[i].vel[0] = v1norm;
					object[i].vel[1] = v1tang;
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
