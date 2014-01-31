#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include "physics.h"

//Global vars
int obj, width, height, objcount;
float dt;
long double gconst, epsno, elcharge;

//Static vars
static int i, j;
static v4sf accprev, vecnorm, vectang, centemp, forceconst = {0, 0};
static float spring = 500;
static long double pi;
static float mag, v1norm, v1tang, v2norm, v2tang;


float dotprod( v4sf a, v4sf b ) {
	float result = a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; 
	return result;
}
float lenght( v4sf a ) {
	float result = a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
	return sqrt(result);
}

int initphys(data** object) {
	*object = malloc(sizeof(**object)*(obj+3));
	for( i = 0; i < obj + 1; i++ ) {
		(*object)[i].linkwith = malloc(sizeof(float)*(obj+400));
	}
	pi = acos(-1);
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
				mag = lenght(vecnorm);
				vecnorm /= mag;
				
				object[i].Fgrv += vecnorm*((float)((gconst*object[i].mass*object[j].mass)/(mag*mag)));
				//future:use whole joints instead of individual stuff
				object[i].Fele += -vecnorm*((float)((object[i].charge*object[j].charge)/(4*pi*epsno*mag*mag)));
				
				if( object[i].linkwith[j] != 0 ) {
					object[i].Flink += vecnorm*((spring)*(mag - object[i].linkwith[j])*(float)0.2);
				}
				if( mag < object[i].radius + object[j].radius ) {
					//fixme
					vectang = (v4sf){-vecnorm[1], vecnorm[0]};

					v1norm = dotprod( vecnorm, object[i].vel );
					v1tang = dotprod( vectang, object[i].vel );
					v2norm = dotprod( vecnorm, object[j].vel );
					v2tang = dotprod( vectang, object[j].vel );

					v1norm = (v1norm*(object[i].mass-object[j].mass) + 2*object[j].mass*v2norm)/(object[i].mass+object[j].mass);
					v2norm = (v2norm*(object[j].mass-object[i].mass) + 2*object[i].mass*v1norm)/(object[j].mass+object[i].mass);
					object[i].vel = (v4sf){v1norm, v1tang};
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
