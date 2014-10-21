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
#include "main/out_xyz.h"
#include "physics/physics.h"
#include "graph/graph.h"
#include "graph/graph_thread.h"
#include "input_thread.h"
#include "sighandle.h"

static const char pointgreen[] = "\033[032m"CMD_PROMPT_DOT"\033[0m";
static const char pointyellow[] = "\033[033m"CMD_PROMPT_DOT"\033[0m";
static const char pointred[] = "\033[031m"CMD_PROMPT_DOT"\033[0m";

static struct input_cfg *global_cfg = NULL;
static struct interp_opt *global_cmd_map = NULL;

int input_thread_init(void **win, unsigned int *frames, float *fps, data *object)
{
	struct input_cfg *cfg = calloc(1, sizeof(struct input_cfg));
	
	cfg->obj = object;
	cfg->win = (graph_window **)win;
	cfg->fps = fps;
	cfg->frames = frames;
	cfg->status = true;
	cfg->selfquit = false;
	cfg->line = NULL;
	
	pthread_create(&cfg->input, NULL, input_thread, cfg);
	
	global_cfg = cfg;
	
	return 0;
}

void input_thread_quit()
{
	if(!global_cfg) return;
	
	global_cfg->status = false;
	
	void *val = NULL, *res = NULL;
	
	if(!global_cfg->selfquit) {
		pthread_cancel(global_cfg->input);
		val = PTHREAD_CANCELED;
	}
	
	pthread_join(global_cfg->input, &res);
	
	if(res != val) {
		pprint_err("Error joining with input thread!\n");
	}
	
	/* Free resources */
	free(global_cfg);
	
	/* Reset terminal */
	rl_free_line_state();
	rl_cleanup_after_signal();
	
	return;
}

void input_print_typed(struct interp_opt *var)
{
	switch(var->type) {
		case T_FLOAT:
			pprintf(PRI_ESSENTIAL, "%s = %f\n", var->name, *(float *)var->val);
			break;
		case T_BOOL:
			pprintf(PRI_ESSENTIAL, "%s = %i\n", var->name, *(bool *)var->val);
			break;
		case T_UINT:
			pprintf(PRI_ESSENTIAL, "%s = %u\n", var->name, *(unsigned int *)var->val);
			break;
		case T_INT:
			pprintf(PRI_ESSENTIAL, "%s = %i\n", var->name, *(int *)var->val);
			break;
		case T_USHORT:
			pprintf(PRI_ESSENTIAL, "%s = %hu\n", var->name, *(unsigned short *)var->val);
			break;
		case T_SHORT:
			pprintf(PRI_ESSENTIAL, "%s = %hi\n", var->name, *(short *)var->val);
			break;
		case T_LONGINT:
			pprintf(PRI_ESSENTIAL, "%s = %li\n", var->name, *(long *)var->val);
			break;
		case T_LONGUINT:
			pprintf(PRI_ESSENTIAL, "%s = %lu\n", var->name, *(long unsigned *)var->val);
			break;
		case T_STRING:
			pprintf(PRI_ESSENTIAL, "%s = %s\n", var->name, *(char **)var->val);
			break;
		default:
			break;
	}
}

void input_set_typed(struct interp_opt *var, char *val)
{
	switch(var->type) {
		case T_FLOAT:
			*(float *)var->val = strtof(val, NULL);
			pprintf(PRI_ESSENTIAL, "%s = %f\n", var->name, *(float *)var->val);
			break;
		case T_BOOL:
			*(bool *)var->val = (bool)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %i\n", var->name, *(bool *)var->val);
			break;
		case T_UINT:
			*(unsigned int *)var->val = (unsigned int)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %u\n", var->name, *(unsigned int *)var->val);
			break;
		case T_INT:
			*(int *)var->val = (int)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %i\n", var->name, *(int *)var->val);
			break;
		case T_USHORT:
			*(unsigned short *)var->val = (unsigned short)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %hu\n", var->name, *(unsigned short *)var->val);
			break;
		case T_SHORT:
			*(short *)var->val = (short)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %hi\n", var->name, *(short *)var->val);
			break;
		case T_LONGINT:
			*(long *)var->val = (long)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %li\n", var->name, *(long *)var->val);
			break;
		case T_LONGUINT:
			*(long unsigned *)var->val = (long unsigned)strtol(val, NULL, 10);
			pprintf(PRI_ESSENTIAL, "%s = %lu\n", var->name, *(long unsigned *)var->val);
			break;
		case T_STRING:
			free(*(char **)var->val);
			*(char **)var->val = strdup(val);
			pprintf(PRI_ESSENTIAL, "%s = %s\n", var->name, *(char **)var->val);
			break;
		default:
			break;
	}
}

void input_cmd_printall(struct interp_opt *cmd_map)
{
	pprintf(PRI_ESSENTIAL, "Implemented commands(current value):\n");
	for(struct interp_opt *i = cmd_map; i->name; i++) {
		if(i->cmd == T_CMD) {
			pprintf(PRI_ESSENTIAL, "%s\n", i->name);
		} else if(i->cmd == T_VAR) {
			input_print_typed(i);
		} else {
			pprintf(PRI_ESSENTIAL, "Unrecognized type: %s\n", i->name);
		}
	}
}

int input_token_setall(char *line, struct input_cfg *t, struct interp_opt *cmd_map)
{
	int num_tok = 0, retval = CMD_ALL_FINE;
	bool match = 0;
	char *tokstr = strdup(line);
	char *token[CMD_MAX_TOKENS] = {NULL}, *freestr = tokstr;
	
	tokstr = strtok(tokstr, " ");
	while(tokstr) {
		if(num_tok > CMD_MAX_TOKENS) {
			retval = CMD_TOO_MANY_TOKENS;
			goto cleanall;
		}
		if(*tokstr == '=') {
			retval = CMD_INVALID_ASSIGN;
			goto cleanall;
		}
		token[num_tok++] = strdup(tokstr);
		tokstr = strtok (NULL, " ");
	}
	
	for(struct interp_opt *i = cmd_map; i->name; i++) {
		if(!strcmp(i->name, token[0])) {
			match = 1;
			if(i->cmd == T_CMD) {
				switch(i->type) {
					case T_HELP:
						input_cmd_printall(cmd_map);
						break;
					case T_STOP:
						phys_ctrl(PHYS_SHUTDOWN, NULL);
						break;
					case T_LIST:
						phys_list_algo();
						break;
					case T_START:
						phys_ctrl(PHYS_START, &t->obj);
						break;
					case T_PAUSE:
						phys_ctrl(PHYS_PAUSESTART, NULL);
						break;
					case T_QUIT:
						retval = CMD_EXIT;
						break;
					case T_STATS:
						raise(SIGUSR1);
						break;
					case T_STATUS:
						raise(SIGUSR1);
						break;
					case T_CLEAR:
						for(int c = 0; c < CMD_CLEAR_REPS; c++) pprintf(PRI_ESSENTIAL, "\n");
						break;
					case T_ENABLE_WINDOW:
						if(*t->win) break;
						*t->win = *graph_thread_init(t->obj, t->frames, t->fps);
						option->novid = false;
						break;
					case T_DISABLE_WINDOW:
						if(!*t->win) break;
						graph_thread_quit();
						*t->win = NULL;
						option->novid = true;
						break;
					default:
						break;
				}
			} else {
				if(num_tok < 2)
					input_print_typed(i);
				else
					input_set_typed(i, token[1]);
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
	static int list_index = 0, len = 0;
	char *name = NULL;
	if(!state) {
		list_index = 0;
		len = strlen(line);
	}
	while((name = global_cmd_map[list_index++].name)) {
		if(!strncmp(name, line, len)) {
			return strdup(name);
		}
	}
	return NULL;
}

char **input_completion(const char *line, int start, int end)
{
	char **matches = NULL;
	
	if(start == 0)
		matches = rl_completion_matches(line, input_cmd_generator);
	
	return matches;
}

void *input_thread(void *thread_setts)
{
	struct input_cfg *t = thread_setts;
	char prompt[sizeof(CMD_PROMPT_BASE)+sizeof(pointyellow)+sizeof(CMD_PROMPT_SPACE)+1];
	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	/* Should remain on stack until thread exits */
	struct interp_opt cmd_map[] = {
		{"dt", &option->dt, T_FLOAT, T_VAR},
		{"threads", &option->threads, T_USHORT, T_VAR},
		{"bh_ratio", &option->bh_ratio, T_FLOAT, T_VAR},
		{"bh_tree_limit", &option->bh_tree_limit, T_USHORT, T_VAR},
		{"bh_lifetime", &option->bh_lifetime, T_USHORT, T_VAR},
		{"bh_heapsize_max", &option->bh_heapsize_max, T_LONGUINT, T_VAR},
		{"bh_single_assign", &option->bh_single_assign, T_BOOL, T_VAR},
		{"bh_random_assign", &option->bh_random_assign, T_BOOL, T_VAR},
		{"algorithm", &option->algorithm, T_STRING, T_VAR},
		{"filename", &option->filename, T_STRING, T_VAR},
		{"def_radius", &option->def_radius, T_FLOAT, T_VAR},
		{"exec_funct_freq", &option->exec_funct_freq, T_UINT, T_VAR},
		{"list", NULL, T_LIST, T_CMD},
		{"quit", NULL, T_QUIT, T_CMD},
		{"exit", NULL, T_QUIT, T_CMD},
		{"start", NULL, T_START, T_CMD},
		{"stop", NULL, T_STOP, T_CMD},
		{"help", NULL, T_HELP, T_CMD},
		{"status", NULL, T_STATUS, T_CMD},
		{"stats", NULL, T_STATS, T_CMD},
		{"clear", NULL, T_CLEAR, T_CMD},
		{"pause", NULL, T_PAUSE, T_CMD},
		{"unpause", NULL, T_PAUSE, T_CMD},
		{"win_create", NULL, T_ENABLE_WINDOW, T_CMD},
		{"win_destroy", NULL, T_DISABLE_WINDOW, T_CMD},
		{0}
	};
	global_cmd_map = cmd_map;
	
	rl_catch_signals = false;
	rl_attempted_completion_function = input_completion;
	
	while(t->status) {
		/* Refresh prompt */
		if(option->paused) {
			sprintf(prompt, "%s %s ", CMD_PROMPT_BASE, pointyellow);
		} else if(option->status) {
			sprintf(prompt, "%s %s ", CMD_PROMPT_BASE, pointgreen);
		} else {
			sprintf(prompt, "%s %s ", CMD_PROMPT_BASE, pointred);
		}
		
		t->line = readline(prompt);
		
		if(!t->line) {
			t->selfquit = 1;
			raise(SIGINT);
		} else if(*t->line) {
			add_history(t->line);
			switch(input_token_setall(t->line, t, cmd_map)) {
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
