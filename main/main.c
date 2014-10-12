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
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

/*	Functions	*/
#include "config.h"
#include "physics/physics.h"
#include "graph/graph.h"
#include "input/graph_input.h"
#include "input/parser.h"
#include "out_xyz.h"
#include "options.h"
#include "msg_phys.h"
#include "physics/physics_aux.h"
#include "input/sighandle.h"
#include "input/input_thread.h"

static const char ARGSTRING[] =
// Generated from helpstring.txt
#include "main/resources/helpstring.h"
;

struct option_struct *option;

int main(int argc, char *argv[])
{
	/*	Default settings.	*/
		option = calloc(1, sizeof(*option));
		add_to_free_queue(option);
		*option = (struct option_struct){
			/* Visuals */
			.width = 1280, .height = 720,
			.fontsize = 38,
			.fontname = strdup("Sans"),
			.spawn_funct = strdup("spawn_objects"),
			.timestep_funct = NULL,
			.exec_funct_freq = 0,
			.lua_expose_obj_array = 0,
			
			/* Physics */
			.threads = 0,
			.dt = 0.008, .verbosity = 5,
			.gconst = 0, .epsno = 0, .elcharge = 0,
			.noele = 1, .nogrv = 1,
			.algorithm = strdup("none"),
			
			/* Physics: Barnes-Hut */
			.bh_ratio = 0.5, .bh_lifetime = 24,
			.bh_tree_limit = 8,
			.bh_heapsize_max = 336870912,
			.bh_single_assign = true, .bh_random_assign = true,
			
			/* Physics - ctrl */
			.dump_xyz = 0,
			.xyz_temp = strdup("system_%0.2Lf.xyz"),
			.dump_sshot = 0,
			.sshot_temp = strdup("sshot_%3.3Lf.png"),
		};
	/*	Default settings.	*/
	
	/*	Main function vars	*/
		struct timeval t1 = {0}, t2 = {0};
		float deltatime = 0.0, totaltime = 0.0;
		unsigned int frames = 0;
		int novid = 0, bench = 0;
		float timer = 1;
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
						phys_list_algo();
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
					if(parse_lua_open_file(option->filename)) {
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
			option->verbosity = 8;
			if(timer==1.0f) timer = 30.0f;
		}
	/*	Arguments	*/
	
	/* Signal handling */
		if(signal(SIGINT, on_quit_signal) == SIG_ERR) {
			fputs("An error occurred while setting SIGINT signal handler.\n", stderr);
			return EXIT_FAILURE;
		}
		if(signal(SIGUSR1, on_usr1_signal) == SIG_ERR) {
			fputs("An error occurred while setting USR1 signal handler.\n", stderr);
			return EXIT_FAILURE;
		}
	/* Signal handling */
	
	/*	Physics.	*/
		data* object;
		
		if(init_elements(NULL)) {
			pprintf(PRI_OK, "Failed to init elements db!\n");
		} else {
			add_to_free_queue(atom_prop);
		}
		
		if(parse_lua_simconf_objects(&object)) {
			pprintf(PRI_ERR, "Could not parse objects from %s!\n",
					option->filename);
			return 2;
		}
		
		add_to_free_queue(object);
		
		pprintf(PRI_ESSENTIAL, "Objects: %i\n", option->obj);
		pprintf(PRI_ESSENTIAL, "Settings: dt=%f\n", option->dt);
		pprintf(PRI_ESSENTIAL,
		"Constants: elcharge=%E C, gconst=%E m^3 kg^-1 s^-2, epsno=%E F m^-1\n", 
							   option->elcharge, option->gconst, option->epsno);
		
		phys_ctrl(PHYS_START, &object);
	/*	Physics.	*/
	
	/*	Graphics	*/
		graph_window *win = NULL;
		if(!novid) {
			win = graph_sdl_init(object);
			/* OpenGL */
			graph_init(win);
		} else {
			input_thread_init(win, object);
		}
	/*	Graphics	*/
	
	gettimeofday(&t1 , NULL);
	
	while(1) {
		/* Get input from SDL */
		graph_sdl_input_main(win);
		
		/* Update timer */
		gettimeofday(&t2, NULL);
		deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
		option->time_running += deltatime;
		t1 = t2;
		totaltime += deltatime;
		frames++;
		
		/* Timer trigg'd events */
		if(totaltime > timer) {
			if(!novid)
				win->fps = frames/totaltime;
			
			if(bench) {
				pprintf(PRI_ESSENTIAL,
						"Progressed %Lf timeunits over %f seconds.\n",
						t_stats[1]->progress, totaltime);
				pprintf(PRI_ESSENTIAL,
						"Average = %Lf timeunits per second.\n",
						t_stats[1]->progress/totaltime);
				on_quit_signal(2);
			}
			totaltime = frames = 0;
		}
		
		/* Prevents wasting CPU time by waking up once every 50 msec. */
		if(novid) {
			/* TODO: Wakeup a bit before physics_ctrl thread wakes up */
			SDL_Delay(50);
		}
		
		/* Draw scene */
		graph_draw_scene(win);
	}
}
