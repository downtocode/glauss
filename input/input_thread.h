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

#define CMD_CLEAR_REPS 20
#define CMD_MAX_TOKENS 20
#define CMD_PROMPT_BASE "phys"
#define CMD_PROMPT_DOT "•"
#define CMD_PROMPT_SPACE " "

enum codepaths {
	T_VAR,
	T_FLOAT,
	T_INT,
	T_UINT,
	T_BOOL,
	T_USHORT,
	T_SHORT,
	T_STRING, /* WARNING: WILL TRY TO FREE, ONLY USE STRDUP'D STRINGS! */
	T_LONGINT,
	T_LONGUINT,
	/* Never mix enums, commands go here */
	T_CMD,
	T_QUIT,
	T_LIST,
	T_START,
	T_STOP,
	T_HELP,
	T_STATUS,
	T_STATS,
	T_CLEAR,
	T_PAUSE,
};

enum intercommunication {
	CMD_ALL_FINE,
	CMD_EXIT,
	CMD_NOT_FOUND,
	CMD_INVALID_ASSIGN,
	CMD_TOO_MANY_TOKENS,
};

/* string as a name, void* to a value, type and T_CMD/T_VAR as cmd */
struct interp_opt {
	const char *name;
	void *val;
	int type;
	int cmd;
};

/* Sent to input thread */
struct input_cfg {
	pthread_t input;
	data *obj;
	graph_window *win;
	char *line;
	volatile bool status, selfquit;
};

int input_thread_ctrl(struct input_cfg *cfg, int ctrl);
int input_thread_init(graph_window *win, data *object);
void input_thread_handle_char(char *line);
void input_thread_quit();
int input_token_setall(char *line, struct input_cfg *t, struct interp_opt *cmd_map);
void *input_thread(void *thread_setts);

#endif
