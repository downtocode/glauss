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
	VAR_LONGLONGUINT,
	VAR_NO_IDEA,
};

#define P_TYPE(x) &(x), _Generic((x),                                            \
    float: VAR_FLOAT,                 double: VAR_DOUBLE,                      \
    long double: VAR_LONG_DOUBLE,     int: VAR_INT,                            \
    unsigned int: VAR_UINT,           unsigned short int: VAR_USHORT,          \
    bool: VAR_BOOL,                   short int: VAR_SHORT,                    \
    char *: VAR_STRING,               long int: VAR_LONGINT,                   \
    unsigned long int: VAR_LONGUINT,  long long unsigned int: VAR_LONGLONGUINT,\
    default: VAR_NO_IDEA                                                         \
), _Generic((x), bool: LUA_TBOOLEAN, char *: LUA_TSTRING, default: LUA_TNUMBER)

struct parser_map {
	const char *name;
	volatile void *val;
	int type;
	int cmd_or_lua_type;
};

extern struct parser_map *total_opt_map;

/* General */
int parse_lua_open_file(const char *filename);
int parse_lua_open_string(const char *script);
int parse_lua_close(void);
void free_input_parse_opts();

/* Maps */
void print_parser_map(struct parser_map *map);
struct parser_map *allocate_parser_map(struct parser_map *map);
unsigned int update_parser_map(struct parser_map *map, struct parser_map **dest);
int register_parser_map(struct parser_map *map, struct parser_map **dest);
int unregister_parser_map(struct parser_map *map, struct parser_map **dest);

/* Maps I/O */
int parser_get_value_str(struct parser_map var, char *str, size_t len);
void parser_print_generic(struct parser_map *var);
void parser_set_generic(struct parser_map *var, const char *val);
void parser_push_generic(lua_State *L, struct parser_map *var);
void parser_read_generic(lua_State *L, struct parser_map *var);

/* Elements */
int parse_lua_simconf_elements(const char *filepath);

/* Parser abstractions */
int parse_lua_simconf_options(struct parser_map *map);
int parse_lua_simconf_objects(phys_obj **object, const char* sent_to_lua);

/* Returns number of changed objects(returned from Lua) */
unsigned int lua_exec_funct(const char *funct, phys_obj *object,
							struct global_statistics *stats);

const char *parse_file_to_str(const char *filename);

#endif
