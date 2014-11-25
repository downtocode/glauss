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
#ifndef PHYSENGINE_INPUT_THREAD
#define PHYSENGINE_INPUT_THREAD

#include <pthread.h>
#include "graph/graph_sdl.h"
#include "input/parser.h"

#define CMD_CLEAR_REPS 20
#define CMD_MAX_TOKENS 20
#define CMD_PROMPT_BASE "phys"
#define CMD_PROMPT_DOT "•"
#define CMD_PROMPT_SPACE " "

enum intercommunication {
	CMD_SYS_RET_ERR,
	CMD_ALL_FINE,
	CMD_EXIT,
	CMD_NOT_FOUND,
	CMD_INVALID_ASSIGN,
	CMD_TOO_MANY_TOKENS,
};

enum commands {
	VAR_CMD,
	VAR_CMD_SYS,
	VAR_QUIT,
	VAR_LIST,
	VAR_START,
	VAR_STOP,
	VAR_RESTART,
	VAR_HELP,
	VAR_STATUS,
	VAR_STATS,
	VAR_CLEAR,
	VAR_PAUSE,
	VAR_SAVE,
	VAR_LOAD,
	VAR_ENABLE_WINDOW,
	VAR_DISABLE_WINDOW,
	VAR_WIN_DRAW_MODE,
	VAR_LUA_READOPTS,
	VAR_CHECK_COLLISIONS,
};

/* Sent to input thread */
struct input_cfg {
	pthread_t input;
	data **obj;
	graph_window **win;
	char *line;
	struct parser_map *cmd_map;
	volatile bool status, selfquit;
};

int input_thread_init(graph_window **win, data **object);
void input_thread_quit(void);
void input_thread_reset_term(void);
int input_call_system(const char *cmd);
int input_token_setall(char *line, struct input_cfg *t);
void *input_thread(void *thread_setts);

#endif
