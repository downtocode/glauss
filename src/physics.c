#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <tgmath.h>
#include "physics.h"

//Definitions
#define threads 2

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

//Sticking to 2 threads for now. Probably will use arrays in the future (is it even possible with pthreads?).
pthread_t thread1, thread2;
int iret1, iret2;

data *objalt;
int looplimit1[8], looplimit2[8];

float dotprod( v4sf a, v4sf b )
{
	float result = a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; 
	return result;
}
float lenght( v4sf a )
{
	float result = a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
	return sqrt(result);
}

int initphys(data** object)
{
	*object = calloc(obj+1,sizeof(**object));
	for( i = 0; i < obj + 1; i++ ) {
		(*object)[i].linkwith = calloc(obj+1,sizeof(float));
	}
	pi = acos(-1);
	
	objalt = *object;
	
	
	looplimit1[0] = 1;
	looplimit2[0] = (int)((float)obj/threads);
	looplimit1[1] = looplimit2[1-1] + 1;
	looplimit2[1] = obj;
	
	return 0;
}

void *resolveforces(void *arg) {
	for(j = looplimit1[(long)arg]; j < looplimit2[(long)arg] + 1; j++) {
		if(i != j) {
			vecnorm = objalt[j].pos - objalt[i].pos;
			mag = lenght(vecnorm);
			vecnorm /= mag;
			
			objalt[i].Fgrv += vecnorm*((float)((gconst*objalt[i].mass*objalt[j].mass)/(mag*mag)));
			//future:use whole joints instead of individual stuff
			objalt[i].Fele += -vecnorm*((float)((objalt[i].charge*objalt[j].charge)/(4*pi*epsno*mag*mag)));
			
			if( objalt[i].linkwith[j] != 0 ) {
				objalt[i].Flink += vecnorm*((spring)*(mag - objalt[i].linkwith[j])*(float)0.2);
			}
			if( mag < objalt[i].radius + objalt[j].radius ) {
				//fixme
				vectang = (v4sf){-vecnorm[1], vecnorm[0]};

				v1norm = dotprod( vecnorm, objalt[i].vel );
				v1tang = dotprod( vectang, objalt[i].vel );
				v2norm = dotprod( vecnorm, objalt[j].vel );
				v2tang = dotprod( vectang, objalt[j].vel );

				v1norm = (v1norm*(objalt[i].mass-objalt[j].mass) + 2*objalt[j].mass*v2norm)/(objalt[i].mass+objalt[j].mass);
				v2norm = (v2norm*(objalt[j].mass-objalt[i].mass) + 2*objalt[i].mass*v1norm)/(objalt[j].mass+objalt[i].mass);
				objalt[i].vel = (v4sf){v1norm, v1tang};
			}
		}
	}
	return 0;
}

int integrate(data* object)
{
	//findstructs(object);
	
	for(i = 1; i < obj + 1; i++) {
		if(object[i].ignore == 1) continue;
		if(object[i].pos[0] < -1 || object[i].pos[0] > 1) {
			object[i].vel[0] = -object[i].vel[0];
		}
		
		if(object[i].pos[1] < -1 || object[i].pos[1] > 1) {
			object[i].vel[1] = -object[i].vel[1];
		}
		
		if(object[i].pos[2] < -1 || object[i].pos[2] > 1) {
			object[i].vel[2] = -object[i].vel[2];
		}
		
		object[i].pos += (object[i].vel*dt) + (object[i].acc)*((dt*dt)/2);
		
		iret1 = pthread_create(&thread1, NULL, resolveforces, (void*)0);
		iret1 = pthread_create(&thread2, NULL, resolveforces, (void*)1);
		
		pthread_join( thread1, NULL);
		pthread_join( thread2, NULL);
		
		object[i].Ftot = forceconst + object[i].Fgrv + object[i].Fele + object[i].Flink;
		accprev = object[i].acc;
		
		object[i].acc = (object[i].Ftot)/(float)object[i].mass;
		object[i].vel += ((dt)/2)*(object[i].acc + accprev);
		object[i].Fgrv = object[i].Fele = centemp = object[i].Flink = (v4sf){0, 0};
	}
	return 0;
}
