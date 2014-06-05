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
#include "parser.h"
#include "glfun.h"
#include "toxyz.h"
#include "options.h"
#include "msg_phys.h"
#include "elements.h"

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
"	-h	--help			What you're reading.\n";

/* Function to input numbers in an array and later extract a single number out.
 * Used in selecting objects.*/
int getnumber(struct numbers_selection *numbers, int currentdigit, unsigned int status) {
	if(status == NUM_ANOTHER) {
		numbers->digits[numbers->final_digit] = currentdigit;
		numbers->final_digit+=1;
		return 0;
	} else if(status == NUM_GIVEME) {
		unsigned int result = 0;
		for(int i=0; i < numbers->final_digit; i++) {
			result*=10;
			result+=numbers->digits[i];
		}
		numbers->final_digit = 0;
		return result;
	} else if(status == NUM_REMOVE) {
		numbers->final_digit-=1;
		return 0;
	}
	return 0;
}

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
		unsigned int frames = 0, chosen = 0, currentnum, fpscolor = GL_WHITE;
		char osdfps[100] = "FPS = n/a", osdobj[100] = "Objects = n/a";
		char osdtime[100] = "Timestep = 0.0", currentsel[100] = "Select object:";
		bool flicked = 0, translate = 0, drawobj = 0, drawlinks = 0;
		bool start_selection = 0;
		int novid = 0, dumplevel = 0, vsync = 1, bench = 0;
		float timer = 1.0f;
		GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0, scalefactor = 0.01;
		GLfloat tr_x = 0.0, tr_y = 0.0, tr_z = 0.0;
		GLuint* shaderprogs;
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
				{"file",		required_argument,		0, 'f'},
				{"help",		no_argument,			0, 'h'},
				{NULL,			0,						0, 0}
			};
			/* getopt_long stores the option index here. */
			int option_index = 0;
			
			c = getopt_long(argc, argv, "a:f:l:t:r:v:h", long_options, &option_index);
			
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
				case 'f':
					option->filename = strdup(optarg);
					if(parse_lua_simconf_options(option->filename)) {
						pprintf(PRI_ERR, "Could not parse configuration from %s!\n", option->filename);
						return 0;
					}
					break;
				case 'h':
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
		sprintf(osdobj, "Objects = %i", option->obj);
		pprintf(PRI_ESSENTIAL, "Settings: dt=%f\n", option->dt);
		pprintf(PRI_ESSENTIAL, "Constants: elcharge=%LE C, gconst=%LE m^3 kg^-1 s^-2, epsno=%LE F m^-1\n" \
		, option->elcharge, option->gconst, option->epsno);
	/*	Physics.	*/
	
	/*	OpenGL ES 2.0 + SDL2	*/
		GLuint textvbo, pointvbo;
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
			resize_wind();
			SDL_GL_CreateContext(window);
			SDL_GL_SetSwapInterval(vsync);
			glViewport(0, 0, option->width, option->height);
			create_shaders(&shaderprogs);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glActiveTexture(GL_TEXTURE0);
			glGenBuffers(1, &textvbo);
			glGenBuffers(1, &pointvbo);
			glClearColor(0.12, 0.12, 0.12, 1);
			pprintf(4, "OpenGL Version %s\n", glGetString(GL_VERSION));
		}
	/*	OpenGL ES 2.0 + SDL2	*/
	
	/*	Freetype.	*/
		if(!novid) {
			if(FT_Init_FreeType(&library)) {
				pprintf(PRI_ERR, "Could not init freetype library.\n");
				return 1;
			}
			if(FT_New_Face(library, option->fontname, 0, &face)) {
				pprintf(PRI_ERR, "Could not open font.\n");
				return 1;
			}
			FT_Set_Pixel_Sizes(face, 0, 34);
			g = face->glyph;
		}
	/*	Freetype.	*/
	
	/*	Links.	*/
		GLfloat (*links)[3] = calloc(10000, sizeof(*links));
		unsigned int linkcounter;
		if(!novid) linkcounter = createlinks(object, &links);
	/*	Links.	*/
	
	gettimeofday (&t1 , NULL);
	
	threadcontrol(PHYS_START, &object);
	
	while( 1 ) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_WINDOWEVENT:
					if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
						SDL_GetWindowSize(window, &option->width, &option->height);
						resize_wind();
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
			
			if(!novid || drawlinks) linkcounter = createlinks(object, &links);
			
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
			/* Prevents wasting CPU time by waking up once every 100 msec.
			 * Using SDL_Delay because usleep is depricated and nanosleep is overkill. */
			SDL_Delay(100);
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
		
		glUseProgram(shaderprogs[0]);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		adjust_rot(view_rotx, view_roty, view_rotz, scalefactor, tr_x, tr_y, tr_z);
		
		/*	Dynamic drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, pointvbo);
		drawaxis();
		for(int i = 1; i < option->obj+1; i++) {
			if(drawobj) draw_obj_sphere(object[i]);
		}
		if(!drawobj) draw_obj_points(object);
		if(drawlinks) draw_obj_links(&links, linkcounter);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Dynamic drawing	*/
		
		
		glUseProgram(shaderprogs[1]);
		
		/*	Static drawing	*/
		glBindBuffer(GL_ARRAY_BUFFER, textvbo);
		if(chosen != 0) {
			selected_box_text(object[chosen]);
			tr_x = -object[chosen].pos[0];
			tr_y = -object[chosen].pos[1];
			tr_z = -object[chosen].pos[2];
		}
		
		{
			if(fps < 25) fpscolor = GL_RED;
			else if(fps < 48) fpscolor = GL_BLUE;
			else fpscolor = GL_GREEN;
			sprintf(osdfps, "FPS = %3.2f", fps);
			render_text(osdfps, -0.95, 0.85, 1.0/option->width, 1.0/option->height, fpscolor);
		}
		
		render_text(osdobj, -0.95, 0.75, 1.0/option->width, 1.0/option->height, GL_WHITE);
		
		sprintf(osdtime, "Timestep = %f", timestep);
		render_text(osdtime, -0.95, 0.65, 1.0/option->width, 1.0/option->height, GL_WHITE);
		
		if(start_selection)
			render_text(currentsel, 0.67, -0.95, 1.0/option->width, 1.0/option->height, GL_WHITE);
		
		if(!threadcontrol(PHYS_STATUS, &object))
			render_text("Simulation stopped", -0.95, -0.95, 1.0/option->width, 1.0/option->height, GL_RED);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/*	Static drawing	*/
		
		SDL_GL_SwapWindow(window);
	}
	
	quit:
		printf("Quitting...\n");
		if(!novid) {
			FT_Done_Face(face);
			FT_Done_FreeType(library);
			SDL_DestroyWindow(window);
			SDL_Quit();
			free(links);
			free(shaderprogs);
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
