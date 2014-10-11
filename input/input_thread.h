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

enum codepaths {
	T_VAR,
	T_FLOAT,
	T_INT,
	T_UINT,
	T_BOOL,
	T_USHORT,
	T_SHORT,
	T_LONGINT,
	T_LONGUINT,
	T_STRING,
	/* Never mix enums */
	T_CMD,
	T_QUIT,
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
	CMD_TOO_MANY_TOKENS,
};

struct interp_opt {
	const char *name;
	void *val;
	int type;
	int cmd;
};

struct input_cfg {
	pthread_t input;
	graph_window *win;
	char *prompt;
	bool status;
};

int input_thread_ctrl(struct input_cfg *cfg, int ctrl);
int input_thread_init(graph_window *win);
void input_thread_handle_char(char *line);
void input_thread_quit();
int input_token_setall(char *line, graph_window *win, struct interp_opt *cmd_map);
void *input_thread(void *thread_setts);

#endif
