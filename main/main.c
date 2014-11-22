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
#include "options.h"
#include "physics/physics.h"
#include "graph/graph_thread.h"
#include "input/input_thread.h"
#include "input/sighandle.h"
#include "input/parser.h"
#include "msg_phys.h"

#define WATCHDOG_OFFSET_SEC 20

static const char ARGSTRING[] =
// Generated from helpstring.txt
#include "main/resources/helpstring.h"
;

struct option_struct *option;

int main(int argc, char *argv[])
{
	/*	Default settings.	*/
		option = calloc(1, sizeof(*option));
		*option = (struct option_struct){
			/* Visuals */
			.def_radius = 1.0,
			.width = 1280, .height = 720,
			.fontsize = 38,
			.fontname = strdup("Sans"),
			.spawn_funct = strdup("spawn_objects"),
			.timestep_funct = NULL,
			.exec_funct_freq = 0,
			.lua_expose_obj_array = 0,
			.quit_main_now = 0,
			.skip_model_vec = 250,
			.novid = 0,
			.rng_seed = 0,
			
			/* Input */
			.input_thread_enable = 1,
			
			/* Physics */
			.threads = 0,
			.dt = 0.008, .verbosity = 5,
			.gconst = 0, .epsno = 0, .elcharge = 0,
			.noele = 1, .nogrv = 1,
			.algorithm = strdup("none"),
			.thread_schedule_mode = strdup("SCHED_RR"),
			
			/* Physics - ctrl */
			.dump_xyz = 0,
			.xyz_temp = strdup("system_%0.2Lf.xyz"),
			.dump_sshot = 0,
			.sshot_temp = strdup("sshot_%3.3Lf.png"),
		};
	/*	Default settings.	*/
	
	/*	Register parser options */
		struct parser_map opts_map[] = {
			{"threads",                P_TYPE(option->threads)                },
			{"dt",                     P_TYPE(option->dt)                     },
			{"rng_seed",               P_TYPE(option->rng_seed)               },
			{"width",                  P_TYPE(option->width)                  },
			{"height",                 P_TYPE(option->height)                 },
			{"elcharge",               P_TYPE(option->elcharge)               },
			{"epsno",                  P_TYPE(option->epsno)                  },
			{"elcharge",               P_TYPE(option->elcharge)               },
			{"gconst",                 P_TYPE(option->gconst)                 },
			{"verbosity",              P_TYPE(option->verbosity)              },
			{"fontsize",               P_TYPE(option->fontsize)               },
			{"dump_xyz",               P_TYPE(option->dump_xyz)               },
			{"dump_sshot",             P_TYPE(option->dump_sshot)             },
			{"exec_funct_freq",        P_TYPE(option->exec_funct_freq)        },
			{"skip_model_vec",         P_TYPE(option->skip_model_vec)         },
			{"algorithm",              P_TYPE(option->algorithm)              },
			{"spawn_funct",            P_TYPE(option->spawn_funct)            },
			{"timestep_funct",         P_TYPE(option->timestep_funct)         },
			{"file_template",          P_TYPE(option->xyz_temp)               },
			{"screenshot_template",    P_TYPE(option->sshot_temp)             },
			{"fontname",               P_TYPE(option->fontname)               },
			{"lua_expose_obj_array",   P_TYPE(option->lua_expose_obj_array)   },
			{"input_thread_enable",    P_TYPE(option->input_thread_enable)    },
			{"thread_schedule_mode",   P_TYPE(option->thread_schedule_mode)   },
			{0},
		};
		register_parser_map(opts_map, &total_opt_map);
	/*	Register parser options */
	
	/*	Main function vars	*/
		unsigned long long int t1 = phys_gettime_us(), t2 = 0;
		long double time = 0.0, deltatime = 0.0;
		int novid = 0, bench = 0;
		char *sent_to_lua = NULL;
		float timer = 1;
	/*	Main function vars	*/
	
	/*	Arguments	*/
		int c;
		
		while(1) {
			struct option long_options[] = {
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
				{"lua_val",		required_argument,		0, 'u'},
				{0}
			};
			/* getopt_long stores the option index here. */
			int option_index = 0;
			
			c = getopt_long(argc, argv, "a:f:l:t:r:u:v:Vh", long_options,
							&option_index);
			
			/* Detect the end of the options. */
			if (c == -1)
				break;
			switch(c) {
				case 0:
					/* If this option set a flag, do nothing else now. */
					if(long_options[option_index].flag != 0)
						break;
					pprintf(PRI_ESSENTIAL, "option %s", long_options[option_index].name);
					if(optarg)
						pprintf(PRI_ESSENTIAL, " with arg %s", optarg);
					pprintf(PRI_ESSENTIAL, "\n");
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
					pprintf(PRI_ESSENTIAL, "%s\nCompiled on %s, %s\n", PACKAGE_STRING,
						   __DATE__, __TIME__);
					exit(0);
					break;
				case 'u':
					sent_to_lua = strdup(optarg);
					break;
				case 'f':
					option->filename = strdup(optarg);
					if (parse_lua_open_file(option->filename)) {
						pprintf(PRI_ERR,
								"Could not parse configuration from %s!\n",
								option->filename);
						return 0;
					} else {
						parse_lua_simconf_options(opts_map);
					}
					break;
				case 'h':
					pprintf(PRI_ESSENTIAL, "%s\nCompiled on %s, %s\n", PACKAGE_STRING,
						   __DATE__, __TIME__);
					pprintf(PRI_ESSENTIAL, "%s", ARGSTRING);
					exit(0);
					break;
				case '?':
					exit(1);
					break;
				default:
					abort();
			}
		}
		
		if (optind < argc) {
			pprintf(PRI_ERR, "Arguments not recognized: ");
			while (optind < argc)
				pprintf(PRI_ESSENTIAL, "%s ", argv[optind++]);
			pprintf(PRI_ESSENTIAL, "\n");
			exit(1);
		}
		
		if (option->filename == NULL) {
			pprintf(PRI_ERR,
					"No file specified! Use -f (filename) to specify one.\n");
			exit(2);
		}
		
		option->novid = novid;
		
		if (bench) {
			pprintf(PRI_WARN, "Benchmark mode active.\n");
			option->novid = 1;
			option->verbosity = 8;
			if (timer==1.0f)
				timer = 30.0f;
		}
	/*	Arguments	*/
	
	/* Signal handling */
		if (signal(SIGINT, on_quit_signal) == SIG_ERR) {
			fputs("An error occurred while setting SIGINT signal handler.\n", stderr);
			return EXIT_FAILURE;
		}
		if (signal(SIGUSR1, on_usr1_signal) == SIG_ERR) {
			fputs("An error occurred while setting USR1 signal handler.\n", stderr);
			return EXIT_FAILURE;
		}
		if (signal(SIGALRM, on_alrm_signal) == SIG_ERR) {
			fputs("An error occurred while setting SIGALRM signal handler.\n", stderr);
			return EXIT_FAILURE;
		} else {
			/* Setup watchdog timer */
			alarm(timer+WATCHDOG_OFFSET_SEC);
		}
	/* Signal handling */
	
	/*	Physics.	*/
		data* object = NULL;
		
		/* Init elements subsystem */
		if (init_elements(NULL)) {
			pprintf(PRI_OK, "Failed to init elements db!\n");
		}
		
		/* Read objects AND initialize physics(we need to know the count) */
		if (parse_lua_simconf_objects(&object, sent_to_lua)) {
			pprintf(PRI_ERR, "Could not parse objects from %s!\n",
					option->filename);
			return 2;
		}
		
		pprintf(PRI_ESSENTIAL, "Objects: %i\n", option->obj);
		pprintf(PRI_ESSENTIAL,
		"Constants: elcharge=%E C, gconst=%E m^3 kg^-1 s^-2, epsno=%E F m^-1\n", 
							   option->elcharge, option->gconst, option->epsno);
		
		phys_ctrl(PHYS_START, &object);
	/*	Physics.	*/
	
	/*	Graphics	*/
		graph_window **window = option->novid ? NULL : graph_thread_init(object);
	/*	Graphics	*/
	
	/*	Input	*/
		if(option->input_thread_enable) {
			input_thread_init(window, &object);
		}
	/*	Input	*/
	
	while (!option->quit_main_now) {
		/* Update timer */
		t2 = phys_gettime_us();
		deltatime = (t2 - t1)*(long double)1.0e-6;
		time += deltatime;
		phys_stats->time_running += deltatime;
		t1 = t2;
		
		/* Timer trigg'd events */
		if (time > timer) {
			/* Kick the watchdog timer */
			alarm(timer+WATCHDOG_OFFSET_SEC);
			
			if (bench) {
				pprintf(PRI_ESSENTIAL,
						"Progressed %Lf timeunits over %Lf seconds.\n",
						phys_stats->progress, time);
				pprintf(PRI_ESSENTIAL,
						"Average = %Lf timeunits per second.\n",
						phys_stats->progress/time);
				raise(SIGINT);
			}
			
			time = 0.0;
		}
		
		/* Prevents wasting CPU time. */
		phys_sleep_msec(37);
	}
	
	phys_sleep_msec(37);
	
	/* free physics */
	phys_quit(&object);
	free(option);
	
	pprint("Done!\n");
	
	return 0;
}
