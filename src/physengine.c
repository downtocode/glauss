/*
 * This file is part of physengine.
 * Copyright (c) 2013 Rostislav Pehlivanov <atomnuker@gmail.com>
 * 
 * physengine is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * physengine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with physengine.  If not, see <http://www.gnu.org/licenses/>.
 */

/*	Standard header files	*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <tgmath.h>
#include <getopt.h>
#include <sys/time.h>
#include <unistd.h>

/*	Dependencies	*/
#include <SDL2/SDL.h>

/*	Functions	*/
#include "graph.h"
#include "config.h"
#include "physics.h"
#include "physics_barnes_hut.h"
#include "parser.h"
#include "toxyz.h"
#include "options.h"
#include "msg_phys.h"
#include "physics_aux.h"

static const char *ARGSTRING =
"Usage: physengine -f (file) (arguments)\n"
"		--novid			Disable video output.\n"
"		--nosync		Disable waiting for vblank.\n"
"		--bench			Benchmark mode(30 seconds, threads=1, novid\n"
"		--dump			Dump an xyz file of the system every second.\n"
"	-a	--algorithm (string)	Select an algorithm to use. To list all: \"help\".\n"
"	-l	--log (file)		Log everything to a file.\n"
"	-t	--threads (int)		Use this amount of threads.\n"
"	-r	--timer (int)		OSD update rate/benchmark duration.\n"
"	-v	--verb (int)		STDOUT spam level.\n"
"	-V	--version		Outputs the version of the program(+short git checksum).\n"
"	-h	--help			What you're reading.\n";

int main(int argc, char *argv[])
{
	/*	Default settings.	*/
		option = calloc(1, sizeof(*option));
		*option = (struct option_struct){
			.width = 1200, .height = 600,
			.avail_cores = 0,
			.dt = 0.008, .verbosity = 5,
			.noflj = 1,
			.gconst = 0, .epsno = 0, .elcharge = 0,
			.noele = 1, .nogrv = 1,
		};
	/*	Default settings.	*/
	
	/*	Main function vars	*/
		SDL_Event event;
		int mousex, mousey, initmousex, initmousey;
		struct timeval t1, t2;
		struct numbers_selection numbers;
		numbers.final_digit = 0;
		float deltatime = 0.0, totaltime = 0.0f, timestep = 0.0, fps = 0.0;
		unsigned int frames = 0, chosen = 0, currentnum;
		char currentsel[100] = "Select object:";
		bool flicked = 0, translate = 0, drawobj = 0, drawlinks = 0;
		bool start_selection = 0;
		int novid = 0, dumplevel = 0, vsync = 1, bench = 0;
		float timer = 1.0f;
		GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0, scalefactor = 0.1;
		GLfloat tr_x = 0.0, tr_y = 0.0, tr_z = 0.0;
	/*	Main function vars	*/
	
	/*	Arguments	*/
		int c;
		
		while(1) {
			struct option long_options[] =
			{
				{"novid",		no_argument,			&novid, 1},
				{"nosync",		no_argument,			&vsync, 0},
				{"bench",		no_argument,			&bench, 1},
				{"dump",		no_argument,			&dumplevel, 1},
				{"log",			required_argument,		0, 'l'},
				{"algorithm",	required_argument,		0, 'a'},
				{"threads",		required_argument,		0, 't'},
				{"timer",		required_argument,		0, 'r'},
				{"verb",		required_argument,		0, 'v'},
				{"version",		no_argument,			0, 'V'},
				{"file",		required_argument,		0, 'f'},
				{"help",		no_argument,			0, 'h'},
				{NULL,			0,						0, 0}
			};
			/* getopt_long stores the option index here. */
			int option_index = 0;
			
			c = getopt_long(argc, argv, "a:f:l:t:r:v:Vh", long_options, &option_index);
			
			/* Detect the end of the options. */
			if(c == -1)
				break;
			switch(c) {
				case 0:
					/* If this option set a flag, do nothing else now. */
					if(long_options[option_index].flag != 0)
						break;
					printf("option %s", long_options[option_index].name);
					if(optarg)
						printf(" with arg %s", optarg);
					printf("\n");
					break;
				case 'a':
					if(strcmp(optarg, "help") == 0) {
						printf("Implemented algorithms:\n");
						for(int n = 0; phys_algorithms[n].name; n++) {
							printf("    %s\n", phys_algorithms[n].name);
						}
						exit(0);
					}
					strcpy(option->algorithm, optarg);
					break;
				case 'l':
					option->logfile = fopen(optarg, "w");
					pprintf(PRI_ESSENTIAL, "Writing log to file %s.\n", optarg);
					option->logenable = 1;
					break;
				case 't':
					sscanf(optarg, "%hu", &option->avail_cores);
					break;
				case 'r':
					sscanf(optarg, "%f", &timer);
					pprintf(PRI_ESSENTIAL, "Timer set to %f seconds.\n", timer);
					break;
				case 'v':
					sscanf(optarg, "%hu", &option->verbosity);
					break;
				case 'V':
					printf("%s\nCompiled on %s, %s\n", PACKAGE_STRING, __DATE__, __TIME__);
					exit(0);
					break;
				case 'f':
					option->filename = strdup(optarg);
					if(parse_lua_simconf_options(option->filename)) {
						pprintf(PRI_ERR, "Could not parse configuration from %s!\n", option->filename);
						return 0;
					}
					break;
				case 'h':
					printf("%s\nCompiled on %s, %s\n", PACKAGE_STRING, __DATE__, __TIME__);
					printf("%s", ARGSTRING);
					exit(0);
					break;
				case '?':
					exit(1);
					break;
				default:
					abort();
			}
		}
		if(optind < argc) {
			pprintf(PRI_ERR, "Arguments not recognized: ");
			while(optind < argc)
				printf("%s ", argv[optind++]);
			printf("\n");
			exit(1);
		}
		
		if(option->filename == NULL) {
			pprintf(PRI_ERR, "No file specified! Use -f (filename) to specify one.\n");
			exit(1);
		}
		
		if(bench) {
			pprintf(PRI_WARN, "Benchmark mode active.\n");
			novid = 1;
			option->avail_cores = 1;
			option->verbosity = 9;
			if(timer==1.0f) timer=30.0f;
		}
	/*	Arguments	*/
	
	/*	Error handling.	*/
	if(dumplevel) printf("Outputting XYZ file every %f seconds.\n", timer);
	/*	Error handling.	*/
	
	/*	Physics.	*/
		data* object;
		if(!init_elements()) pprintf(PRI_OK, "Successfully read ./resources/elements.conf!\n");
		if(parse_lua_simconf_objects(option->filename, &object)) {
			pprintf(PRI_ERR, "Could not parse objects from %s!\n", option->filename);
			return 0;
		}
		
		pprintf(PRI_ESSENTIAL, "Objects: %i\n", option->obj);
		pprintf(PRI_ESSENTIAL, "Settings: dt=%f\n", option->dt);
		pprintf(PRI_ESSENTIAL, "Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n" \
		, option->elcharge, option->gconst, option->epsno);
	/*	Physics.	*/
	
	/*	OpenGL ES 2.0 + SDL2	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		if(!novid) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			window = SDL_CreateWindow(PACKAGE_STRING, \
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, option->width, option->height, \
				SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
			SDL_GL_CreateContext(window);
			SDL_GL_SetSwapInterval(vsync);
			/* We deal with any and all graphical vizualization through this function. */
			graph_init();
		}
	/*	OpenGL ES 2.0 + SDL2	*/
	
	gettimeofday (&t1 , NULL);
	
	threadcontrol(PHYS_START, &object);
	
	struct phys_barnes_hut_octree *octree = bh_init_tree(object);
	//bh_build_octree(object, octree);
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_WINDOWEVENT:
					if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
						SDL_GetWindowSize(window, &option->width, &option->height);
						graph_resize_wind();
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if(event.button.button == SDL_BUTTON_LEFT) flicked = 1;
					if(event.button.button == SDL_BUTTON_MIDDLE) {
						chosen = 0;
						translate = 1;
					}
					SDL_ShowCursor(0);
					SDL_GetMouseState(&initmousex, &initmousey);
					SDL_SetRelativeMouseMode(1);
					SDL_GetRelativeMouseState(NULL, NULL);
					break;
				case SDL_MOUSEBUTTONUP:
					if(event.button.button == SDL_BUTTON_LEFT) flicked = 0;
					if(event.button.button == SDL_BUTTON_MIDDLE) translate = 0;
					if(!translate && !flicked) {
						SDL_SetRelativeMouseMode(0);
						SDL_WarpMouseInWindow(window, initmousex, initmousey);
						SDL_ShowCursor(1);
					}
					break;
				case SDL_MOUSEWHEEL:
					if(event.wheel.y == 1) scalefactor *= 1.11;
					if(event.wheel.y == -1) scalefactor /= 1.11;
					if(scalefactor < 0.005) scalefactor = 0.005;
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_ESCAPE) {
						goto quit;
					}
					if(start_selection) {
						if(event.key.keysym.sym!=SDLK_RETURN && numbers.final_digit < 19) {
							if(event.key.keysym.sym==SDLK_BACKSPACE && numbers.final_digit > 0) {
								getnumber(&numbers, 0, NUM_REMOVE);
								currentsel[strlen(currentsel)-1] = '\0';
								break;
							}
							/*	sscanf will return 0 if nothing was done	*/
							if(!sscanf(SDL_GetKeyName(event.key.keysym.sym), "%u", &currentnum)) {
								break;
							} else {
								strcat(currentsel, SDL_GetKeyName(event.key.keysym.sym));
								getnumber(&numbers, currentnum, NUM_ANOTHER);
							}
							break;
						} else {
							strcpy(currentsel, "Select object:");
							start_selection = 0;
							chosen = getnumber(&numbers, 0, NUM_GIVEME);
							if(chosen > option->obj) chosen = 0;
							break;
						}
					}
					if(event.key.keysym.sym==SDLK_RETURN) {
						start_selection = 1;
						break;
					}
					if(event.key.keysym.sym==SDLK_RIGHTBRACKET) {
						option->dt *= 2;
						printf("dt = %f\n", option->dt);
					}
					if(event.key.keysym.sym==SDLK_LEFTBRACKET) {
						option->dt /= 2;
						printf("dt = %f\n", option->dt);
					}
					if(event.key.keysym.sym==SDLK_SPACE) {
						if(threadcontrol(PHYS_STATUS, &object)) threadcontrol(PHYS_PAUSE, &object);
						else threadcontrol(PHYS_UNPAUSE, &object);
					}
					if(event.key.keysym.sym==SDLK_r) {
						view_roty = view_rotx = view_rotz = 0.0;
						tr_x = tr_y = tr_z = 0.0;
						scalefactor = 0.1;
						chosen = 0;
					}
					if(event.key.keysym.sym==SDLK_o) {
						bh_build_octree(object, octree);
					}
					if(event.key.keysym.sym==SDLK_z) {
						toxyz(option->obj, object, timestep);
					}
					if(event.key.keysym.sym==SDLK_PERIOD) {
						if(chosen < option->obj) chosen++;
					}
					if(event.key.keysym.sym==SDLK_COMMA) {
						if(chosen > 0) chosen--;
					}
					if(event.key.keysym.sym==SDLK_1) {
						if(drawobj) drawobj = 0;
						else drawobj = 1;
					}
					if(event.key.keysym.sym==SDLK_2) {
						if(drawlinks) drawlinks = 0;
						else drawlinks = 1;
					}
					break;
				case SDL_QUIT:
					goto quit;
					break;
			}
		}
		
		{
			timestep = option->processed*option->dt;
			gettimeofday(&t2, NULL);
			deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
			t1 = t2;
			totaltime += deltatime;
			frames++;
		}
		if (totaltime >  timer) {
			fps = frames/totaltime;
			
			if(dumplevel) toxyz(option->obj, object, timestep);
			pprintf(PRI_VERYLOW, "Progressed %f timeunits.\n", timestep);
			
			if(bench) {
				pprintf(PRI_ESSENTIAL, "Progressed %f timeunits over %f seconds.\n", timestep, totaltime);
				pprintf(PRI_ESSENTIAL, "Average = %f timeunits per second.\n", timestep/totaltime);
				goto quit;
			}
			totaltime = frames = 0;
		}
		
		if(novid) {
			/* Prevents wasting CPU time by waking up once every 50 msec.
			 * Using SDL_Delay because usleep is depricated and nanosleep is overkill. */
			SDL_Delay(50);
			continue;
		}
		
		if(flicked || translate) {
			SDL_GetRelativeMouseState(&mousex, &mousey);
			if(flicked) {
				view_roty += (float)mousex/4;
				view_rotx += (float)mousey/4;
			}
			if(translate) {
				tr_x += -(float)mousex/100;
				tr_y += (float)mousey/100;
			}
		}
		
		adjust_rot(view_rotx, view_roty, view_rotz, scalefactor, tr_x, tr_y, tr_z);
		graph_draw_scene(&object, fps);
		
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		printf("Quitting...\n");
		if(!novid) {
			SDL_DestroyWindow(window);
			SDL_Quit();
		}
		if(threadcontrol(PHYS_STATUS, &object))
			threadcontrol(PHYS_SHUTDOWN, &object);
		if(option->logenable)
			fclose(option->logfile);
		free(option);
		free(atom_prop);
		free(object);
		return 0;
}
