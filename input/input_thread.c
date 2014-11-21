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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "input/parser.h"
#include "main/options.h"
#include "main/msg_phys.h"
#include "main/output.h"
#include "physics/physics.h"
#include "graph/graph.h"
#include "graph/graph_thread.h"
#include "input_thread.h"
#include "sighandle.h"

static const char pointgreen[] = "\033[032m"CMD_PROMPT_DOT"\033[0m";
static const char pointyellow[] = "\033[033m"CMD_PROMPT_DOT"\033[0m";
static const char pointred[] = "\033[031m"CMD_PROMPT_DOT"\033[0m";

/* Needed for the generator and completions */
static int list_index_cmd = 0, len_cmd = 0;
static int list_index_opt = 0, len_opt = 0;
static struct input_cfg *global_cfg = NULL;

int input_thread_init(void **win, data **object)
{
	struct input_cfg *cfg = calloc(1, sizeof(struct input_cfg));
	
	cfg->obj = object;
	cfg->win = (graph_window **)win;
	cfg->status = true;
	cfg->selfquit = false;
	cfg->line = NULL;
	cfg->opt_map = total_opt_map;
	cfg->cmd_map = allocate_parser_map((struct parser_map[]){
		{"phys_check_collisions", NULL, VAR_CHECK_COLLISIONS, VAR_CMD},
		{"save", NULL, VAR_SAVE, VAR_CMD},
		{"load", NULL, VAR_LOAD, VAR_CMD},
		{"list", NULL, VAR_LIST, VAR_CMD},
		{"quit", NULL, VAR_QUIT, VAR_CMD},
		{"exit", NULL, VAR_QUIT, VAR_CMD},
		{"start", NULL, VAR_START, VAR_CMD},
		{"stop", NULL, VAR_STOP, VAR_CMD},
		{"restart", NULL, VAR_RESTART, VAR_CMD},
		{"help", NULL, VAR_HELP, VAR_CMD},
		{"status", NULL, VAR_STATUS, VAR_CMD},
		{"stats", NULL, VAR_STATS, VAR_CMD},
		{"clear", NULL, VAR_CLEAR, VAR_CMD},
		{"pause", NULL, VAR_PAUSE, VAR_CMD},
		{"unpause", NULL, VAR_PAUSE, VAR_CMD},
		{"win_create", NULL, VAR_ENABLE_WINDOW, VAR_CMD},
		{"win_destroy", NULL, VAR_DISABLE_WINDOW, VAR_CMD},
		{"lua_readopts", NULL, VAR_LUA_READOPTS, VAR_CMD},
		{"#command (runs a system command)", NULL, VAR_CMD_SYS, VAR_CMD},
		{0}
	});
	
	global_cfg = cfg;
	
	pthread_create(&cfg->input, NULL, input_thread, cfg);
	
	return 0;
}

void input_thread_quit(void)
{
	if (!global_cfg)
		return;
	
	global_cfg->status = false;
	
	void *val = NULL, *res = NULL;
	
	if (!global_cfg->selfquit) {
		pthread_cancel(global_cfg->input);
		val = PTHREAD_CANCELED;
		free(global_cfg->line);
	}
	
	pthread_join(global_cfg->input, &res);
	
	if (res != val) {
		pprint_err("Error joining with input thread!\n");
	}
	
	/* Reset terminal */
	rl_free_line_state();
	rl_cleanup_after_signal();
	
	/* Free resources */
	free(global_cfg->cmd_map);
	free(global_cfg);
	
	return;
}

static void input_cmd_printall(struct parser_map *cmd_map, struct parser_map *opt_map)
{
	pprintf(PRI_ESSENTIAL, "Implemented variables:\n");
	for (struct parser_map *i = opt_map; i->name; i++) {
		parser_print_generic(i);
	}
	
	pprintf(PRI_ESSENTIAL, "Implemented commands:\n");
	for (struct parser_map *i = cmd_map; i->name; i++) {
		if (i->cmd_or_lua_type == VAR_CMD) {
			pprintf(PRI_ESSENTIAL, "%s\n", i->name);
		} else {
			parser_print_generic(i);
		}
	}
}

int input_call_system(const char *cmd)
{
	pprint_disable();
	int ret = system(cmd);
	pprint_enable();
	return ret;
}

int input_token_setall(char *line, struct input_cfg *t)
{
	int num_tok = 0, retval = CMD_ALL_FINE;
	bool match = 0;
	char *tokstr = strdup(line);
	char *token[CMD_MAX_TOKENS] = {NULL}, *freestr = tokstr;
	
	tokstr = strtok(tokstr, " ");
	while (tokstr) {
		if (num_tok > CMD_MAX_TOKENS) {
			retval = CMD_TOO_MANY_TOKENS;
			goto cleanall;
		}
		if (*tokstr == '=') {
			retval = CMD_INVALID_ASSIGN;
			goto cleanall;
		}
		token[num_tok++] = strdup(tokstr);
		tokstr = strtok (NULL, " ");
	}
	
	if (token[0][0] == '#') {
		size_t len = strlen(line)-1;
		char *command = calloc(1, len);
		strncpy(command, line+1, len);
		int cmd_ret = input_call_system(command);
		free(command);
		retval = cmd_ret ? CMD_SYS_RET_ERR : CMD_ALL_FINE;
		goto cleanall;
	}
	
	for (struct parser_map *i = t->opt_map; i->name; i++) {
		if (!strcmp(i->name, token[0])) {
			match = 1;
			if (num_tok < 2)
				parser_print_generic(i);
			else
				parser_set_generic(i, token[1]);
		}
	}
	
	for (struct parser_map *i = t->cmd_map; i->name; i++) {
		if (!strcmp(i->name, token[0])) {
			match = 1;
			if (i->cmd_or_lua_type == VAR_CMD) {
				switch(i->type) {
					case VAR_CHECK_COLLISIONS:
						pprint_warn("Checking for collisions...\n");
						unsigned int coll = phys_check_collisions(*t->obj, 0, option->obj);
						if (coll) {
							pprint_err("%i objects share coordinates,\
									   reconsider starting anything at all and\
									   check your input script\n");
						} else {
							pprint_ok("No object share coordinates\n");
						}
						break;
					case VAR_HELP:
						input_cmd_printall(t->cmd_map, t->opt_map);
						break;
					case VAR_STOP:
						phys_ctrl(PHYS_SHUTDOWN, NULL);
						break;
					case VAR_RESTART:
						phys_ctrl(PHYS_SHUTDOWN, NULL);
						phys_ctrl(PHYS_START, t->obj);
						break;
					case VAR_LIST:
						phys_list_algo();
						break;
					case VAR_START:
						phys_ctrl(PHYS_START, t->obj);
						break;
					case VAR_SAVE:
						out_write_array(*t->obj, "array_%0.2Lf.bin", halt_objects);
						break;
					case VAR_LOAD:
						if (num_tok < 2) {
							printf("Usage: \"load <filename>\"\n");
						} else {
							in_write_array(t->obj, token[1], halt_objects);
						}
						break;
					case VAR_PAUSE:
						phys_ctrl(PHYS_PAUSESTART, NULL);
						break;
					case VAR_QUIT:
						retval = CMD_EXIT;
						break;
					case VAR_STATS:
						raise(SIGUSR1);
						break;
					case VAR_STATUS:
						raise(SIGUSR1);
						break;
					case VAR_CLEAR:
						for (int c = 0; c < CMD_CLEAR_REPS; c++) pprintf(PRI_ESSENTIAL, "\n");
						break;
					case VAR_LUA_READOPTS:
						if (num_tok < 2) {
							parse_lua_simconf_options(t->opt_map);
						} else {
							if (strcmp(option->filename, token[1])) {
								if (parse_lua_open_file(token[1])) {
									break;
								}
							}
							parse_lua_simconf_options(t->opt_map);
						}
						break;
					case VAR_ENABLE_WINDOW:
						if (*t->win) break;
						*t->win = *graph_thread_init(*t->obj);
						option->novid = false;
						break;
					case VAR_DISABLE_WINDOW:
						if (!*t->win) break;
						graph_thread_quit();
						*t->win = NULL;
						option->novid = true;
						break;
					default:
						break;
				}
			}
			break;
		}
	}
	if(!match) retval = CMD_NOT_FOUND;
	
	cleanall:
		for(int i = 0; i < num_tok; i++) {
			free(token[i]);
		}
		free(freestr);
	
	return retval;
}

char *input_cmd_generator(const char *line, int state)
{
	/* Readline is fussy as fuck about every single word. Here be dragons */
	char *name = NULL;
	if (!state) {
		list_index_cmd = 0;
		len_cmd = strlen(line);
	}
	while ((name = (char *)global_cfg->cmd_map[list_index_cmd++].name)) {
		if (!strncmp(name, line, len_cmd)) {
			return strdup(name);
		}
	}
	return NULL;
}

char *input_opt_generator(const char *line, int state)
{
	/* Readline is fussy as fuck about every single... we did this already*/
	char *name = NULL;
	if (!state) {
		list_index_opt = 0;
		len_opt = strlen(line);
	}
	while ((name = (char *)global_cfg->opt_map[list_index_opt++].name)) {
		if (!strncmp(name, line, len_opt)) {
			return strdup(name);
		}
	}
	return NULL;
}

char **input_completion(const char *line, int start, int end)
{
	char **matches = NULL;
	
	if (start == 0) {
		matches = rl_completion_matches(line, input_cmd_generator);
		if (!matches) {
			matches = rl_completion_matches(line, input_opt_generator);
		}
	}
	
	return matches;
}

void *input_thread(void *thread_setts)
{
	struct input_cfg *t = thread_setts;
	char prompt[sizeof(CMD_PROMPT_BASE)+sizeof(pointyellow)+sizeof(CMD_PROMPT_SPACE)+1];
	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	/* Rant: why the shit does every library in existance want to install its
	 * own signal handler which THEN passes the signal to us? Even damn SDL
	 * tries to do that, DESPITE having to make us call SDL_Quit(). It's retarded */
	rl_catch_signals = false;
	rl_attempted_completion_function = input_completion;
	
	while (t->status) {
		/* Refresh prompt */
		if (phys_ctrl(PHYS_STATUS, NULL) == PHYS_STATUS_PAUSED) {
			sprintf(prompt, "%s %s ", CMD_PROMPT_BASE, pointyellow);
		} else if (phys_ctrl(PHYS_STATUS, NULL) == PHYS_STATUS_RUNNING) {
			sprintf(prompt, "%s %s ", CMD_PROMPT_BASE, pointgreen);
		} else {
			sprintf(prompt, "%s %s ", CMD_PROMPT_BASE, pointred);
		}
		
		t->line = readline(prompt);
		
		if (!t->line) {
			t->selfquit = 1;
			raise(SIGINT);
		} else if (*t->line) {
			add_history(t->line);
			switch(input_token_setall(t->line, t)) {
				case CMD_SYS_RET_ERR:
					pprint_err("System command error\n");
					break;
				case CMD_ALL_FINE:
					break;
				case CMD_EXIT:
					t->selfquit = 1;
					raise(SIGINT);
					break;
				case CMD_NOT_FOUND:
					pprintf(PRI_ERR, "Command not found\n");
					break;
				case CMD_INVALID_ASSIGN:
					pprintf(PRI_ERR, "Correct assignment syntax is \"variable value\"\n");
					break;
				default:
					break;
			}
		}
		free(t->line);
		t->line = NULL;
	}
	
	t->selfquit = 1;
	
	return 0;
}
