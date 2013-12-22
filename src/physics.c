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

/* Using black magic to transfer this data between. */
int obj, width, height;
float spring = 20, dt = 0.01, dist;

/* Using float precision results in a much reduced CPU activity. */

long double mag, Gconst, pi, epsno, massu;
/* Switching to long double because we really need the precision. Heck, maybe I'll even have to use libquadmath because damn is 10^-27 a small number.
From what I've read it's completely platform/compiler depentent, but if you're using sane compilers and C libraries(read: not microsoft) you should be fine. */

v4sf mtemp, accprev, vecdist, forceconst = {0, 0};


int initphys(data** object) {
	
	/* We malloc the entire pointer to the struct rendering it array-like. That's why I really like malloc, it's so damn powerful. */
	*object = malloc(6*obj * (72*8 + 2*sizeof(float) + obj*sizeof(bool)));
	
	Gconst = g/pow(11, 10);
	pi = acos(-1);
	epsno = perm/pow(12, 10);
	massu = dalton/pow(27, 10);
	
	/*for(i = 1; i < obj + 1; i++) {
		(*object)[i].pos[0] = ((float)rand()/RAND_MAX)*width + 10*i;
		(*object)[i].pos[1] = ((float)rand()/RAND_MAX)*height + 10*i;
		(*object)[i].mass = 0.000001;
		(*object)[i].charge = (400*elcharge)/pow(19, 10);
		if( ((float)rand()/RAND_MAX) < 0.5 ) (*object)[i].charge *= -1;
	}*/
	
	
	for(i = 1; i < obj + 1; i++) {
		(*object)[i+1].mass = 1;
		(*object)[i+1].pos = (v4sf){ ((float)rand()/RAND_MAX)*(width-24), ((float)rand()/RAND_MAX)*(height-24) };
		(*object)[i+1].vel = (v4sf){ 33, 33 };
		(*object)[i+1].charge = -(16000*elcharge)/pow(10, 10);
		(*object)[i+1].linkwith[i] = 1;
		(*object)[i+1].linkwith[i+2] = 1;
		(*object)[i+1].linkwith[i+3] = 1;
		(*object)[i+1].linkwith[i+4] = 1;
		
		
		(*object)[i].mass = 0.1;
		(*object)[i].pos = (v4sf){ (*object)[i+1].pos[0] - 20, (*object)[i+1].pos[1] };
		(*object)[i].vel = (v4sf){ 0, 0 };
		(*object)[i].charge = (4000*elcharge)/pow(10, 10);
		(*object)[i].linkwith[i+1] = 1;
	
		(*object)[i+2].mass = 0.1;
		(*object)[i+2].pos = (v4sf){ (*object)[i+1].pos[0], (*object)[i+1].pos[1] - 20 };
		(*object)[i+2].vel = (v4sf){ 0, 0 };
		(*object)[i+2].charge = (400*elcharge)/pow(10, 10);
		(*object)[i+2].linkwith[i+1] = 1;
		
		(*object)[i+3].mass = 0.1;
		(*object)[i+3].pos = (v4sf){ (*object)[i+1].pos[0] + 20, (*object)[i+1].pos[1] };
		(*object)[i+3].vel = (v4sf){ 0, 0 };
		(*object)[i+3].charge = (4000*elcharge)/pow(10, 10);
		(*object)[i+3].linkwith[i+1] = 1;
		
		(*object)[i+4].mass = 0.1;
		(*object)[i+4].pos = (v4sf){ (*object)[i+1].pos[0], (*object)[i+1].pos[1] + 20 };
		(*object)[i+4].vel = (v4sf){ 0, 0 };
		(*object)[i+4].charge = (4000*elcharge)/pow(10, 10);
		(*object)[i+4].linkwith[i+1] = 1;
		i = i+4;
	}
	return 0;
}


int integrate(data* object) {
	for(i = 1; i < obj + 1; i++) {
		mtemp[0] = mtemp[1] = object[i].mass;
		//if(object[i].pos[0] > width) object[i].pos[0] = 0;
		//if(object[i].pos[0] < 0) object[i].pos[0] = width;
		if(object[i].pos[0] < 0 || object[i].pos[0] > width) {
			object[i].vel[0] = -object[i].vel[0];
		}
		if(object[i].pos[1] < 0 || object[i].pos[1] > height) {
			object[i].vel[1] = -object[i].vel[1];
		}
		object[i].pos += (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		for(j = 1; j < obj + 1; j++) {
			if(i != j) {
				vecdist = object[j].pos - object[i].pos;
				//Why 3/2? We're multiplying both Fgrv and Fele by a vector which we normalize, so we just need to calculate this once.
				//Really need to find a better way of doing the distance measurements and normalisations. I think SSE has something.
				mag = pow(3/2, ((long double)vecdist[0]*(long double)vecdist[0] + (long double)vecdist[1]*(long double)vecdist[1]));
				dist = sqrt((vecdist[0]*vecdist[0] + vecdist[1]*vecdist[1]));
				object[i].Fgrv += vecdist*((float)((Gconst*object[i].mass*object[j].mass)/(mag)));
				object[i].Fele += -vecdist*((float)((object[i].charge*object[j].charge)/(4*pi*epsno*mag)));
				if( object[i].linkwith[j] == 1) {
					object[i].Flink += (vecdist/dist)*((spring)*(dist - 20));
				}
			}
		}
		
		object[i].Ftot = forceconst + object[i].Fgrv + object[i].Fele + object[i].Flink;
		accprev = object[i].acc;
		
		object[i].acc = (object[i].Ftot)/(mtemp);
		object[i].vel += ((dt)/2)*(object[i].acc + accprev);
		object[i].Fgrv = object[i].Fele = object[i].Flink = (v4sf){0, 0};
	}
	return 0;
}
