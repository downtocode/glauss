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

#include "physics/physics.h"

enum vartype {
	VAR_VAR,
	VAR_FLOAT,
	VAR_DOUBLE,
	VAR_INT,
	VAR_UINT,
	VAR_BOOL,
	VAR_USHORT,
	VAR_SHORT,
	VAR_STRING,
	VAR_LONGINT,
	VAR_LONGUINT,
};

struct parser_opt {
	const char *name;
	void *val;
	int type;
	int lua_type;
};

int parse_lua_open_file(const char *filename);
int parse_lua_open_string(const char *script);
int parse_lua_close(void);
int parse_lua_simconf_options(void);
int parse_lua_simconf_objects(data **object, const char* sent_to_lua);

/* Returns number of changed objects(returned from Lua) */
unsigned int lua_exec_funct(const char *funct, data *object);

const char *parse_file_to_str(const char *filename);

#endif
