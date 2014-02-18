#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "physics.h"
#include "parser.h"


//Global variables
int obj;
long double elcharge;
bool quiet, random;

//Static variables
static char str[200];
static float velmax, massrand, chargerand, sizerand;

int preparser(float *dt, long double *elcharge, long double *gconst, long double *epsno, int *width, \
int *height, float *boxsize, char fontname[200], char filename[200])
{
	int count = 0;
	char word[200], variable[200], namebuff[200];
	long double anothervar;
	float value, base, power;
	FILE *inconf = fopen ( "simconf.conf", "r" );
	while(fgets (str, sizeof(str), inconf)!=NULL) {
		if (strstr(str, "#") == NULL) {
			sscanf(str, "%s = \"%f\"", word, &value);
			if(strcmp(word, "dt") == 0) {
				*dt = value;
			}
			if(strcmp(word, "elcharge") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, variable);
				sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
				*elcharge = anothervar*pow(base, power);
			}
			if(strcmp(word, "gconst") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, variable);
				sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
				*gconst = anothervar*pow(base, power);
			}
			if(strcmp(word, "epsno") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, variable);
				sscanf(variable, "%Lfx%f^(%f)", &anothervar, &base, &power);
				*epsno = anothervar*pow(base, power);
			}
			if(strcmp(word, "width") == 0) {
				*width = (int)value;
			}
			if(strcmp(word, "height") == 0) {
				*height = (int)value;
			}
			if(strcmp(word, "boxsize") == 0) {
				*boxsize = value;
			}
			if(strcmp(word, "fontname") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, fontname);
			}
			if(strcmp(word, "random") == 0) {
				random = (bool)value;
			}
			if(strcmp(word, "randobjs") == 0) {
				count = (int)value;
			}
			if(strcmp(word, "velmax") == 0) {
				velmax = value;
			}
			if(strcmp(word, "massrand") == 0) {
				massrand = value;
			}
			if(strcmp(word, "chargerand") == 0) {
				chargerand = value;
			}
			if(strcmp(word, "sizerand") == 0) {
				sizerand = value;
			}
			if(strcmp(word, "posdata") == 0) {
				sscanf(str, "%s = \"%100[^\"]\"", word, namebuff);
			}
		} else {
			memset(str, 0, sizeof(str));
		}
	}
	fclose(inconf);
	
	memset(str, 0, sizeof(str));
	
	if( random == 1 ) {
		srand(time(NULL));
	} else { 
		count = 0;
		if( access( filename, F_OK ) == -1 ) {
			fprintf( stderr, "Argument/default set filename %s not found! Trying %s from configuration file...", filename, namebuff );
			if( access( namebuff, F_OK ) == 0 ) {
				strcpy( filename, namebuff );
				fprintf( stderr, " Success!\n");
			} else {
				fprintf( stderr, " Fail!\n");
				return 0;
			}
		}
		FILE *inprep = fopen ( filename, "r" );
		while(fgets (str, sizeof(str), inprep)!=NULL) {
			if (strstr(str, "#") == NULL) {
				count += 1;
			}
		}
		fclose(inprep);
	}
	memset(str, 0, sizeof(str));
	return count;
}

int parser(data** object, char filename[200])
{
	int i, link;
	char links[200], *linkstr, ignflag;
	float posx, posy, posz, velx, vely, velz, bond, radius;
	long double mass, chargetemp;
	
	FILE *in = fopen ( filename, "r" );
	
	if( random == 0 ) {
		if( quiet == 0 ) {
			printf("	Position		Velocity   |   Mass   |  Charge  |  Radius  |Ign|   Links:\n");
		}
		while(fgets (str, sizeof(str), in)!= NULL) {
			if (strstr(str, "#") == NULL) {
				sscanf(str, "%i %f %f %f %f %f %f %Lf %Lf %f %c \"%s\"", &i, &posx, &posy, &posz, &velx, \
				&vely, &velz, &mass, &chargetemp, &radius, &ignflag, links);
				
				(*object)[i].pos = (v4sf){ posx, posy, posz };
				(*object)[i].vel = (v4sf){ velx, vely, velz };
				(*object)[i].mass = mass;
				(*object)[i].charge = chargetemp*elcharge;
				(*object)[i].ignore = ignflag;
				(*object)[i].center = 0;
				(*object)[i].radius = radius;
				
				if( quiet == 0 ) {
					printf("(%0.2f, %0.2f, %0.2f)	(%0.2f, %0.2f, %0.2f) | %0.2LE | %0.2LE | %f | %c | ", \
					(*object)[i].pos[0], (*object)[i].pos[1], (*object)[i].pos[2], (*object)[i].vel[0], (*object)[i].vel[1], \
					(*object)[i].vel[2], (*object)[i].mass, (*object)[i].charge, (*object)[i].radius, (*object)[i].ignore);
				}
				
				if( links[0] != 0 ) {
					linkstr = strtok(links,",");
					while(linkstr != NULL) {
						sscanf(linkstr, "%i-%f", &link, &bond);
						if( quiet == 0 ) printf("%i - %f ", link, bond);
						(*object)[i].linkwith[link] = bond;
						linkstr = strtok(NULL,",");
					}
					bond = link = 0;
					memset(links, 0, sizeof(links));
				}
				
				if( quiet == 0 ) printf(" \n");
			} else {
				/*	Wipe string in case last line didn't have a newline char.	*/
				memset(str, 0, sizeof(str));
			}
		}
	} else if ( random == 1 ) {
		for(i = 1; i < obj + 1; i++) {
			(*object)[i].pos = (v4sf){((float)rand()/(float)RAND_MAX), ((float)rand()/(float)RAND_MAX), ((float)rand()/(float)RAND_MAX)};
			(*object)[i].vel = (v4sf){(((float)rand()/(float)RAND_MAX) - 0.5)*velmax, \
			(((float)rand()/(float)RAND_MAX) - 0.5)*velmax, (((float)rand()/(float)RAND_MAX) - 0.5)*velmax};
			(*object)[i].mass = (((float)rand()/(float)RAND_MAX))*massrand;
			(*object)[i].charge = (((float)rand()/(float)RAND_MAX) - 0.5)*chargerand*elcharge*2;
			(*object)[i].radius = (((float)rand()/(float)RAND_MAX))*sizerand + 0.07;
			(*object)[i].center = 0;
			(*object)[i].ignore = 0;
		}
	}
	fclose(in);
	return 0;
}

char* readshader(const char* filename)
{
	FILE* input = fopen(filename, "r");
	if(input == NULL) return NULL;
	
	if(fseek(input, 0, SEEK_END) == -1) return NULL;
	long size = ftell(input);
	if(size == -1) return NULL;
	if(fseek(input, 0, SEEK_SET) == -1) return NULL;
	
	/*if using c-compiler: dont cast malloc's return value*/
	char *content = (char*) malloc( (size_t) size +1  ); 
	if(content == NULL) return NULL;
	
	fread(content, 1, (size_t)size, input);
	if(ferror(input)) {
		free(content);
		return NULL;
	}
	
	fclose(input);
	content[size] = '\0';
	return content;
}
