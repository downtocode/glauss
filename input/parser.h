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
#ifndef PHYSENGINE_PARS
#define PHYSENGINE_PARS

#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include "physics/physics.h"
#include "in_file.h"

enum vars_and_cmds {
	/* Variables */
	VAR_FLOAT,
	VAR_DOUBLE,
	VAR_LONG_DOUBLE,
	VAR_INT,
	VAR_UINT,
	VAR_BOOL,
	VAR_USHORT,
	VAR_SHORT,
	VAR_STRING,
	VAR_LONGINT,
	VAR_LONGUINT,
	VAR_SIZE_T,
	/* Commands */
	VAR_CMD,
	VAR_CMD_SYS,
	VAR_QUIT,
	VAR_LIST,
	VAR_START,
	VAR_STOP,
	VAR_HELP,
	VAR_STATUS,
	VAR_STATS,
	VAR_CLEAR,
	VAR_PAUSE,
	VAR_SAVE,
	VAR_LOAD,
	VAR_ENABLE_WINDOW,
	VAR_DISABLE_WINDOW,
	VAR_LUA_READOPTS,
	VAR_CHECK_COLLISIONS,
};

struct parser_opt {
	const char *name;
	void *val;
	int type;
	int cmd_or_lua_type;
};

extern struct parser_opt *total_opt_map;

int parse_lua_open_file(const char *filename);
int parse_lua_open_string(const char *script);
int parse_lua_close(void);
struct parser_opt *allocate_input_parse_opts(struct parser_opt *map);
void print_input_parse_opts(struct parser_opt *map);
int register_input_parse_opts(struct parser_opt *map);
int unregister_input_parse_opts(struct parser_opt *map);
unsigned int update_input_parse_opts(struct parser_opt *map);
void free_input_parse_opts();
int parse_lua_simconf_options();
int parse_lua_simconf_objects(data **object, const char* sent_to_lua);

/* Returns number of changed objects(returned from Lua) */
unsigned int lua_exec_funct(const char *funct, data *object);

const char *parse_file_to_str(const char *filename);

#endif
