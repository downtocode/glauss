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
#include "config.h"
#include "physics.h"
#include "graph.h"
#include "parser.h"
#include "out_xyz.h"
#include "options.h"
#include "msg_phys.h"
#include "physics_aux.h"

static const char *ARGSTRING =
"Usage: physengine -f (file) (arguments)\n"
"		--novid			Disable video output.\n"
"		--bench			Benchmark mode(30 seconds, threads=1, novid\n"
"	-a	--algorithm (string)	Select an algorithm to use. To list all: \"help\".\n"
"	-l	--log (file)		Log everything to a file.\n"
"	-t	--threads (int)		Use this amount of threads.\n"
"	-r	--timer (int)		OSD update rate/benchmark duration.\n"
"	-v	--verb (int)		STDOUT spam level.\n"
"	-V	--version		Outputs the version of the program(+short git checksum).\n"
"	-h	--help			What you're reading.\n";

struct option_struct *option;

int main(int argc, char *argv[])
{
	/*	Default settings.	*/
		option = calloc(1, sizeof(*option));
		*option = (struct option_struct) {
			/* Visuals */
			.width = 1200, .height = 600,
			.fontsize = 38,
			.sshot_temp = strdup("sshot_%3.3Lf.png"),
			.fontname = strdup("Sans"),
			
			/* Physics */
			.threads = 0,
			.dt = 0.008, .verbosity = 5,
			.gconst = 0, .epsno = 0, .elcharge = 0,
			.noele = 1, .nogrv = 1,
			.algorithm = strdup("n-body"),
			
			/* Physics: Barnes-Hut */
			.bh_ratio = 0.5, .bh_lifetime = 24,
			.bh_tree_limit = 8,
			.bh_heapsize_max = 536870912,
			.bh_single_assign = true,
			
			/* Physics - misc */
			.dump_xyz = 0, .dump_sshot = 0,
		};
	/*	Default settings.	*/
	
	/*	Main function vars	*/
		SDL_Event event;
		int mousex, mousey, initmousex, initmousey;
		struct timeval t1, t2;
		struct numbers_selection numbers;
		struct graph_cam_view camera = { 32.0, 315.0, 0, 0, 0, 0, 0.1 };
		camera.scalefactor = 0.005;
		numbers.final_digit = 0;
		float deltatime = 0.0, totaltime = 0.0, fps = 0.0;
		unsigned int frames = 0, chosen = 0, currentnum;
		char currentsel[100] = "Select object:";
		bool flicked = 0, translate = 0, fullscreen = 0;
		bool start_selection = 0;
		int novid = 0, bench = 0;
		float timer = 1.0f;
	/*	Main function vars	*/
	
	/*	Arguments	*/
		int c;
		
		while(1) {
			struct option long_options[] =
			{
				{"novid",		no_argument,			&novid, 1},
				{"bench",		no_argument,			&bench, 1},
				{"log",			required_argument,		0, 'l'},
				{"algorithm",	required_argument,		0, 'a'},
				{"threads",		required_argument,		0, 't'},
				{"timer",		required_argument,		0, 'r'},
				{"verb",		required_argument,		0, 'v'},
				{"version",		no_argument,			0, 'V'},
				{"file",		required_argument,		0, 'f'},
				{"help",		no_argument,			0, 'h'},
				{0}
			};
			/* getopt_long stores the option index here. */
			int option_index = 0;
			
			c = getopt_long(argc, argv, "a:f:l:t:r:v:Vh", long_options,
							&option_index);
			
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
					sscanf(optarg, "%hu", &option->threads);
					break;
				case 'r':
					sscanf(optarg, "%f", &timer);
					pprintf(PRI_ESSENTIAL, "Timer set to %f seconds.\n", timer);
					break;
				case 'v':
					sscanf(optarg, "%hu", &option->verbosity);
					break;
				case 'V':
					printf("%s\nCompiled on %s, %s\n", PACKAGE_STRING,
						   __DATE__, __TIME__);
					exit(0);
					break;
				case 'f':
					option->filename = strdup(optarg);
					if(parse_lua_open(option->filename)) {
						pprintf(PRI_ERR,
								"Could not parse configuration from %s!\n",
								option->filename);
						return 0;
					} else parse_lua_simconf_options();
					break;
				case 'h':
					printf("%s\nCompiled on %s, %s\n", PACKAGE_STRING,
						   __DATE__, __TIME__);
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
			pprintf(PRI_ERR,
					"No file specified! Use -f (filename) to specify one.\n");
			exit(2);
		}
		
		if(bench) {
			pprintf(PRI_WARN, "Benchmark mode active.\n");
			novid = 1;
			//option->verbosity = 9;
			if(timer==1.0f) timer=30.0f;
		}
	/*	Arguments	*/
	
	/*	Physics.	*/
		data* object;
		
		if(init_elements(NULL)) {
			pprintf(PRI_OK, "Failed to init elements db!\n");
		}
		
		if(parse_lua_simconf_objects(&object)) {
			pprintf(PRI_ERR, "Could not parse objects from %s!\n",
					option->filename);
			return 2;
		}
		
		pprintf(PRI_ESSENTIAL, "Objects: %i\n", option->obj+1);
		pprintf(PRI_ESSENTIAL, "Settings: dt=%f\n", option->dt);
		pprintf(PRI_ESSENTIAL,
		"Constants: elcharge=%E C, gconst=%E m^3 kg^-1 s^-2, epsno=%E F m^-1\n", 
							   option->elcharge, option->gconst, option->epsno);
	/*	Physics.	*/
	
	/*	SDL2	*/
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = NULL;
		SDL_GLContext context = NULL;
		if(!novid) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			window = SDL_CreateWindow(PACKAGE_STRING, \
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, option->width,
				option->height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|\
				SDL_WINDOW_ALLOW_HIGHDPI);
			context = SDL_GL_CreateContext(window);
			/* We deal with any and all graphical vizualization here */
			graph_init();
		}
	/*	SDL2	*/
	
	threadcontrol(PHYS_START, &object);
	
	gettimeofday (&t1 , NULL);
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_WINDOWEVENT:
					if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
						SDL_GL_GetDrawableSize(window, &option->width, &option->height);
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
					if(event.wheel.y == 1) camera.scalefactor *= 1.11;
					if(event.wheel.y == -1) camera.scalefactor /= 1.11;
					if(camera.scalefactor < 0.005) camera.scalefactor = 0.005;
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_ESCAPE) {
						goto quit;
					}
					if(start_selection) {
						if(event.key.keysym.sym!=SDLK_RETURN &&\
							numbers.final_digit < 19) {
							if(event.key.keysym.sym==SDLK_BACKSPACE &&\
								numbers.final_digit > 0) {
								getnumber(&numbers, 0, NUM_REMOVE);
								currentsel[strlen(currentsel)-1] = '\0';
								break;
							}
							/*	sscanf will return 0 if nothing was done	*/
							if(!sscanf(SDL_GetKeyName(event.key.keysym.sym),
								"%u", &currentnum)) {
								break;
							} else {
								strcat(currentsel,
									   SDL_GetKeyName(event.key.keysym.sym));
								getnumber(&numbers, currentnum, NUM_ANOTHER);
							}
							break;
						} else {
							strcpy(currentsel, "Select object:");
							start_selection = 0;
							chosen = getnumber(&numbers, 0, NUM_GIVEME);
							if(chosen > option->obj)
								chosen = 0;
							else pprintf(PRI_HIGH, "Object %u selected.\n",
										 chosen);
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
						threadcontrol(PHYS_PAUSESTART, &object);
					}
					if(event.key.keysym.sym==SDLK_r) {
						camera = (struct graph_cam_view)\
								 { 32.0, 315.0, 0, 0, 0, 0, 0.1 };
						chosen = 0;
					}
					if(event.key.keysym.sym==SDLK_z) {
						toxyz(object);
					}
					if(event.key.keysym.sym==SDLK_q) {
						goto quit;
					}
					if(event.key.keysym.sym==SDLK_PERIOD) {
						if(chosen < option->obj) chosen++;
					}
					if(event.key.keysym.sym==SDLK_COMMA) {
						if(chosen > 0) chosen--;
					}
					if(event.key.keysym.sym==SDLK_f) {
						if(fullscreen) {
							SDL_SetWindowFullscreen(window, 0);
							fullscreen = 0;
						} else {
							/* Because real fullscreen sucks */
							SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
							fullscreen = 1;
						}
						SDL_GL_GetDrawableSize(window, &option->width, &option->height);
						graph_resize_wind();
					}
					if(event.key.keysym.sym==SDLK_s) {
						graph_sshot(t_stats[1]->progress);
					}
					break;
				case SDL_QUIT:
					goto quit;
					break;
			}
		}
		
		{
			gettimeofday(&t2, NULL);
			deltatime = (float)(t2.tv_sec - t1.tv_sec +\
										(t2.tv_usec - t1.tv_usec) * 1e-6);
			t1 = t2;
			totaltime += deltatime;
			frames++;
		}
		if (totaltime >  timer) {
			fps = frames/totaltime;
			
			if(bench) {
				pprintf(PRI_ESSENTIAL,
						"Progressed %Lf timeunits over %f seconds.\n",
						t_stats[1]->progress, totaltime);
				pprintf(PRI_ESSENTIAL,
						"Average = %Lf timeunits per second.\n",
						t_stats[1]->progress/totaltime);
				goto quit;
			}
			totaltime = frames = 0;
		}
		
		if(novid) {
			/* Prevents wasting CPU time by waking up once every 50 msec. */
			SDL_Delay(50);
			continue;
		}
		
		if(flicked || translate) {
			SDL_GetRelativeMouseState(&mousex, &mousey);
			if(flicked) {
				camera.view_roty += (float)mousex/4;
				camera.view_rotx += (float)mousey/4;
			}
			if(translate) {
				camera.tr_x += -(float)mousex/100;
				camera.tr_y += (float)mousey/100;
			}
		}
		
		if(chosen != 0) {
			camera.tr_x = object[chosen].pos[0];
			camera.tr_y = object[chosen].pos[1];
			camera.tr_z = object[chosen].pos[2];
		}
		graph_view(&camera);
		
		graph_draw_scene(&object, fps, chosen);
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		printf("Quitting...\n");
		if(!novid) {
			SDL_GL_DeleteContext(context);
			SDL_DestroyWindow(window);
			SDL_Quit();
		}
		threadcontrol(PHYS_SHUTDOWN, &object);
		if(option->logenable)
			fclose(option->logfile);
		free(option);
		free(atom_prop);
		free(object);
		return 0;
}
