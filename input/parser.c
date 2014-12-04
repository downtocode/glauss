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
#include <tgmath.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"
#include "physics/physics.h"
#include "physics/physics_aux.h"
#include "main/options.h"
#include "main/msg_phys.h"

struct lua_parser_state {
	int i;
	bool nullswitch;
	bool fileset;
	bool read_id;
	in_file file;
	phys_obj buffer, *object;
	struct atomic_cont buffer_ele;
	struct parser_map *opt_map;
};

static bool lua_loaded = 0;
static lua_State *Lp;
struct parser_map *total_opt_map = NULL;

static void conf_lua_get_vector(lua_State *L, vec3 *vec)
{
	/* Can be used for N dim tables, change 3 to N */
	for (int i = 0; i < 3; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		(*vec)[i] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
}

static void conf_lua_get_color(lua_State *L, float color[4])
{
	for (int i = 0; i < 4; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		color[i] = lua_tointeger(L, -1)/255.0;
		lua_pop(L, 1);
	}
}

void parser_print_generic(struct parser_map *var)
{
	switch(var->type) {
		case VAR_FLOAT:
			pprintf(PRI_ESSENTIAL, "%s = %f\n", var->name, *(float *)var->val);
			break;
		case VAR_DOUBLE:
			pprintf(PRI_ESSENTIAL, "%s = %lf\n", var->name, *(double *)var->val);
			break;
		case VAR_LONG_DOUBLE:
			pprintf(PRI_ESSENTIAL, "%s = %Lf\n", var->name, *(long double *)var->val);
			break;
		case VAR_BOOL:
			pprintf(PRI_ESSENTIAL, "%s = %i\n", var->name, *(bool *)var->val);
			break;
		case VAR_UINT:
			pprintf(PRI_ESSENTIAL, "%s = %u\n", var->name, *(unsigned int *)var->val);
			break;
		case VAR_INT:
			pprintf(PRI_ESSENTIAL, "%s = %i\n", var->name, *(int *)var->val);
			break;
		case VAR_USHORT:
			pprintf(PRI_ESSENTIAL, "%s = %hu\n", var->name, *(unsigned short *)var->val);
			break;
		case VAR_SHORT:
			pprintf(PRI_ESSENTIAL, "%s = %hi\n", var->name, *(short *)var->val);
			break;
		case VAR_LONGINT:
			pprintf(PRI_ESSENTIAL, "%s = %li\n", var->name, *(long *)var->val);
			break;
		case VAR_LONGUINT:
			pprintf(PRI_ESSENTIAL, "%s = %lu\n", var->name, *(long unsigned *)var->val);
			break;
		case VAR_STRING:
			pprintf(PRI_ESSENTIAL, "%s = %s\n", var->name, *(char **)var->val);
			break;
		default:
			break;
	}
}

void parser_set_generic(struct parser_map *var, const char *val)
{
	if(!val)
		return;
	switch(var->type) {
		case VAR_BOOL:
			if (!strcmp("true", val))
				*(bool *)var->val = true;
			else if (!strcmp("false", val))
				*(bool *)var->val = false;
			else
				*(bool *)var->val = (bool)strtol(val, NULL, 10);
			pprint("%s = %i\n", var->name, *(bool *)var->val);
			break;
		case VAR_FLOAT:
			*(float *)var->val = strtof(val, NULL);
			pprint("%s = %f\n", var->name, *(float *)var->val);
			break;
		case VAR_DOUBLE:
			*(double *)var->val = strtod(val, NULL);
			pprint("%s = %lf\n", var->name, *(double *)var->val);
			break;
		case VAR_LONG_DOUBLE:
			*(long double *)var->val = strtold(val, NULL);
			pprint("%s = %Lf\n", var->name, *(long double *)var->val);
			break;
		case VAR_SHORT:
			*(short *)var->val = strtol(val, NULL, 10);
			pprint("%s = %hi\n", var->name, *(short *)var->val);
			break;
		case VAR_INT:
			*(int *)var->val = strtol(val, NULL, 10);
			pprint("%s = %i\n", var->name, *(int *)var->val);
			break;
		case VAR_LONGINT:
			*(long *)var->val = strtol(val, NULL, 10);
			pprint("%s = %li\n", var->name, *(long *)var->val);
			break;
		case VAR_USHORT:
			*(unsigned short *)var->val = strtoul(val, NULL, 10);
			pprint("%s = %hu\n", var->name, *(unsigned short *)var->val);
			break;
		case VAR_UINT:
			*(unsigned *)var->val = strtoul(val, NULL, 10);
			pprint("%s = %u\n", var->name, *(unsigned int *)var->val);
			break;
		case VAR_LONGUINT:
			*(long unsigned *)var->val = strtoul(val, NULL, 10);
			pprint("%s = %lu\n", var->name, *(long unsigned *)var->val);
			break;
		case VAR_LONGLONGUINT:
			*(long long unsigned int *)var->val = strtoul(val, NULL, 10);
			pprint("%s = %llu\n", var->name, *(long long unsigned int *)var->val);
			break;
		case VAR_STRING:
			if(!var->val)
				break;
			free(*(char **)var->val);
			*(char **)var->val = strdup(val);
			pprint("%s = %s\n", var->name, *(char **)var->val);
			break;
		default:
			break;
	}
}

int parser_get_value_str(struct parser_map var, char *str, size_t len)
{
	switch(var.type) {
		case VAR_BOOL:
			if (*(bool *)var.val)
				snprintf(str, len, "true");
			else
				snprintf(str, len, "false");
			break;
		case VAR_FLOAT:
			snprintf(str, len, "%f", *(float *)var.val);
			break;
		case VAR_DOUBLE:
			snprintf(str, len, "%lf", *(double *)var.val);
			break;
		case VAR_LONG_DOUBLE:
			snprintf(str, len, "%Lf", *(long double *)var.val);
			break;
		case VAR_SHORT:
			snprintf(str, len, "%hi", *(short int *)var.val);
			break;
		case VAR_INT:
			snprintf(str, len, "%i", *(int *)var.val);
			break;
		case VAR_LONGINT:
			snprintf(str, len, "%li", *(long int *)var.val);
			break;
		case VAR_USHORT:
			snprintf(str, len, "%hu", *(unsigned short int *)var.val);
			break;
		case VAR_UINT:
			snprintf(str, len, "%u", *(unsigned int *)var.val);
			break;
		case VAR_LONGUINT:
			snprintf(str, len, "%0.3lf MiB", *(long unsigned int *)var.val/(double)1048576);
			break;
		case VAR_LONGLONGUINT:
			snprintf(str, len, "%llu", *(long long unsigned int *)var.val);
			break;
		case VAR_STRING:
			snprintf(str, len, "%s", *(char **)var.val);
			break;
		default:
			return 1;
			break;
	}
	return 0;
}

void parser_push_generic(lua_State *L, struct parser_map *var)
{
	if(!var)
		return;
	switch(var->type) {
		case VAR_BOOL:
			lua_pushboolean(L, *(bool *)var->val);
			break;
		case VAR_FLOAT:
			lua_pushnumber(L, *(float *)var->val);
			break;
		case VAR_DOUBLE:
			lua_pushnumber(L, *(double *)var->val);
			break;
		case VAR_LONG_DOUBLE:
			lua_pushnumber(L, *(long double *)var->val);
			break;
		case VAR_SHORT:
			lua_pushinteger(L, *(short *)var->val);
			break;
		case VAR_INT:
			lua_pushinteger(L, *(int *)var->val);
			break;
		case VAR_LONGINT:
			lua_pushinteger(L, *(long *)var->val);
			break;
		case VAR_USHORT:
			lua_pushinteger(L, *(unsigned short *)var->val);
			break;
		case VAR_UINT:
			lua_pushinteger(L, *(unsigned *)var->val);
			break;
		case VAR_LONGUINT:
			lua_pushinteger(L, *(long unsigned *)var->val);
			break;
		case VAR_LONGLONGUINT:
			lua_pushinteger(L, *(long long unsigned int *) var->val);
			break;
		case VAR_STRING:
			lua_pushstring(L, *(char **)var->val);
			break;
		default:
			lua_pushnumber(L, *(int *)var->val);
			break;
	}
	lua_setfield(L, -2, var->name);
}

void parser_read_generic(lua_State *L, struct parser_map *var)
{
	if (!var)
		return;
	switch(var->type) {
		case VAR_BOOL:
			*(bool *)var->val = lua_toboolean(L, -1);
			break;
		case VAR_FLOAT:
			*(float *)var->val = lua_tonumber(L, -1);
			break;
		case VAR_DOUBLE:
			*(double *)var->val = lua_tonumber(L, -1);
			break;
		case VAR_LONG_DOUBLE:
			*(long double *)var->val = lua_tonumber(L, -1);
			break;
		case VAR_SHORT:
			*(short *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_INT:
			*(int *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_LONGINT:
			*(long *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_USHORT:
			*(unsigned short *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_UINT:
			*(unsigned *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_LONGUINT:
			*(long unsigned *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_LONGLONGUINT:
			*(long long unsigned int *)var->val = lua_tointeger(L, -1);
			break;
		case VAR_COLOR:
			conf_lua_get_color(L, (float *)var->val);
		case VAR_STRING:
			free(*(char **)var->val);
			*(char **)var->val = strdup(lua_tostring(L, -1));
			break;
		default:
			break;
	}
}

static int conf_lua_parse_opts(lua_State *L, struct lua_parser_state *parser_state)
{
	if (!parser_state->opt_map) {
		pprint_err("Cannot read empty map.\n");
		return 0;
	}
	if (lua_istable(L, -1)) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			const char *name = lua_tostring(L, -2);
			for (struct parser_map *i = parser_state->opt_map; i->name; i++) {
				if (!strcmp(i->name, name)) {
					conf_lua_get_color(L, (float *)i->val);
					return 0;
				}
			}
		}
		/* Tell conf_traverse_table() to traverse */
		return 1;
	} else {
		if (lua_type(L, -2) != LUA_TSTRING)
			return 0;
		/* Should remain on stack for a few microseconds, right? */
		const char *name = lua_tostring(L, -2);
		for (struct parser_map *i = parser_state->opt_map; i->name; i++) {
			if (!strcmp(i->name, name)) {
				parser_read_generic(L, i);
				break;
			}
		}
	}
	return 0;
}

static int conf_lua_parse_objs(lua_State *L, struct lua_parser_state *parser_state)
{
	/* Variables are read out of order so wait until we see the next object */
	if (lua_istable(L, -1)) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			const char *name = lua_tostring(L, -2);
			if (!strcmp("pos", name)) {
				conf_lua_get_vector(L, &parser_state->buffer.pos);
				return 0;
			} else if (!strcmp("vel", name)) {
				conf_lua_get_vector(L, &parser_state->buffer.vel);
				return 0;
			} else if (!strcmp("acc", name)) {
				conf_lua_get_vector(L, &parser_state->buffer.acc);
				return 0;
			} else if (!strcmp("rot", name)) {
				conf_lua_get_vector(L, &parser_state->file.rot);
				return 0;
			}
		}
		if (parser_state->fileset) {
			/* It's a file */
			if(!access(parser_state->file.filename, R_OK)) {
				pprintf(PRI_OK, "File %s found!\n", parser_state->file.filename);
				parser_state->file.inf = &parser_state->buffer;
				in_read_file(parser_state->object, &parser_state->i, &parser_state->file);
				memset(&parser_state->file, 0,
					   sizeof(in_file));
				parser_state->fileset = 0;
			} else {
				pprintf(PRI_ERR, "File %s not found!\n",
						parser_state->file.filename);
				exit(1);
			}
		} else {
			/* It's just an object. */
			if (!parser_state->nullswitch) {
				/* Cheap hack. Lua's first object is always, always CORRUPTED */
				parser_state->nullswitch = 1;
				return 1;
			}
			
			/* We're reading an object from an entire array(on init) */
			unsigned int arr_num = parser_state->i;
			
			if (parser_state->read_id) {
				arr_num = parser_state->buffer.id;
			} else {
				parser_state->buffer.id = parser_state->i;
			}
			
			if (arr_num > option->obj) {
				pprint_warn("Lua told us to change obj id = %i, but no such exists.\
							\nCheck your Lua exec_function/spawn code. Skipping.\n",
								arr_num);
				return 1;
			}
			
			printf("Old %i = %lf %lf %lf\n", arr_num,
				   (parser_state->object)[arr_num].pos[0],
				   (parser_state->object)[arr_num].pos[1],
				   (parser_state->object)[arr_num].pos[2]);
			printf("New %i = %lf %lf %lf\n", arr_num,
				   parser_state->buffer.pos[0],
				   parser_state->buffer.pos[1],
				   parser_state->buffer.pos[2]);
			
			/* Set object */
			(parser_state->object)[arr_num] = parser_state->buffer;
			parser_state->i++;
		}
		/* Object/file finished, go to next */
		return 1;
	} else if(lua_isnumber(L, -1)) {
		if (!strcmp("charge", lua_tostring(L, -2)))
			parser_state->buffer.charge = lua_tonumber(L, -1)*option->elcharge;
		if (!strcmp("mass", lua_tostring(L, -2)))
			parser_state->buffer.mass = lua_tonumber(L, -1);
		if (!strcmp("radius", lua_tostring(L, -2)))
			parser_state->buffer.radius = lua_tonumber(L, -1);
		if (!strcmp("atomnumber", lua_tostring(L, -2)))
			parser_state->buffer.atomnumber = lua_tointeger(L, -1);
		if (!strcmp("scale", lua_tostring(L, -2)))
			parser_state->file.scale = lua_tonumber(L, -1);
		if (!strcmp("state", lua_tostring(L, -2)))
			parser_state->buffer.state = lua_tointeger(L, -1);
		if (!strcmp("id", lua_tostring(L, -2))) {
			parser_state->buffer.id = lua_tointeger(L, -1);
		}
	} else if (lua_isstring(L, -1)) {
		/* It's a file to import, so we set the flag */
		if (!strcmp("import", lua_tostring(L, -2))) {
			strcpy(parser_state->file.filename, lua_tostring(L, -1));
			parser_state->fileset = 1;
		}
		if (!strcmp("atom", lua_tostring(L, -2))) {
			parser_state->buffer.atomnumber = return_atom_num(lua_tostring(L, -1));
		}
	} else if (lua_isboolean(L, -1)) {
		if (!strcmp("ignore", lua_tostring(L, -2)))
			parser_state->buffer.ignore = lua_toboolean(L, -1);
	}
	return 0;
}

/* We have to know the exact amount of objects we need memory for so we scan. */
static int conf_lua_parse_files(lua_State *L, struct lua_parser_state *parser_state)
{
	if (lua_istable(L, -1)) {
		if(lua_type(L, -2) != LUA_TSTRING) {
			return 1;
		}
	} else if (lua_isstring(L, -1)) {
		if (!strcmp("import", lua_tostring(L, -2))) {
			if (!access(lua_tostring(L, -1), R_OK)) {
				pprintf(PRI_OK, "File %s found!\n", lua_tostring(L, -1));
				/* A molecule is a single object which we get rid of. */
				option->obj += in_probe_file(lua_tostring(L, -1)) - 1;
			} else {
				pprintf(PRI_ERR, "File %s not found!\n",
						lua_tostring(L, -1));
				exit(1);
			}
		}
	}
	return 0;
}

static int conf_lua_parse_elements(lua_State *L, struct lua_parser_state *parser_state)
{
	if (lua_istable(L, -1)) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			if (!strcmp("color", lua_tostring(L, -2))) {
				conf_lua_get_color(L, parser_state->buffer_ele.color);
				return 0;
			}
		}
		if (!parser_state->nullswitch) {
			parser_state->nullswitch = 1;
		}
		parser_state->buffer_ele.number = parser_state->i;
		atom_prop[parser_state->i++] = parser_state->buffer_ele;
		parser_state->buffer_ele = (struct atomic_cont){0};
		return 1;
	} else if (lua_isnumber(L, -1)) {
		if(!strcmp("mass", lua_tostring(L, -2)))
			parser_state->buffer_ele.mass = lua_tonumber(L, -1);
	} else if (lua_isstring(L, -1)) {
		if (!strcmp("name", lua_tostring(L, -2))) 
			parser_state->buffer_ele.name = strdup(lua_tostring(L, -1));
	}
	return 0;
}

static void conf_traverse_table(lua_State *L, int (rec_fn(lua_State *, struct lua_parser_state *)),
								struct lua_parser_state *parser_state)
{
	if (!lua_loaded) {
		pprintf(PRI_ERR, "Lua context not loaded, no file/string to read!\n");
		raise(SIGINT);
	}
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		if (rec_fn(L, parser_state)) {
			/* Funtion pointer will return 1 if we need to go deeper */
			conf_traverse_table(L, rec_fn, parser_state);
		}
		lua_pop(L, 1);
	}
}

static int input_lua_raise(lua_State *L)
{
	int signal = lua_tointeger(L, -1);
	if(!signal)
		signal = SIGINT;
	raise(signal);
	return 0;
}

static int input_lua_stop_physics(lua_State *L)
{
	phys_ctrl(PHYS_PAUSESTART, NULL);
	return 0;
}

static int input_lua_setopt(lua_State *L)
{
	const char *name = lua_tostring(L, -2);
	for (struct parser_map *i = total_opt_map; i->name; i++) {
		if (!strcmp(i->name, name)) {
			parser_read_generic(L, i);
			return 0;
		}
	}
	return 1;
}

static int parse_lua_register_fn(lua_State *L)
{
	/* Register own function to quit */
	lua_register(L, "raise", input_lua_raise);
	lua_register(L, "phys_pause", input_lua_stop_physics);
	lua_register(L, "set_option", input_lua_setopt);
	return 0;
}

int parse_lua_open_file(const char *filename)
{
	if(lua_loaded) {
		pprintf(PRI_WARN, "Closing previous Lua context\n");
		parse_lua_close();
	}
	Lp = luaL_newstate();
	luaL_openlibs(Lp);
	
	parse_lua_register_fn(Lp);
	
	/* Load file */
	if(luaL_loadfile(Lp, filename)) {
		pprintf(PRI_ERR, "Opening Lua file %s failed!\n", filename);
		return 2;
	}
	/* Execute script */
	lua_pcall(Lp, 0, 0, 0);
	
	lua_loaded = 1;
	
	return 0;
}

int parse_lua_open_string(const char *script)
{
	if(lua_loaded) {
		pprintf(PRI_WARN, "Closing previous Lua context\n");
		parse_lua_close();
	} else
		lua_loaded = 1;
	Lp = luaL_newstate();
	luaL_openlibs(Lp);
	
	parse_lua_register_fn(Lp);
	
	/* Load string */
	if(luaL_loadstring(Lp, script)) {
		pprintf(PRI_ERR, "Opening Lua script failed!\n");
		return 2;
	}
	
	/* Execute script */
	lua_pcall(Lp, 0, 0, 0);
	
	return 0;
}

/* Close lua file */
int parse_lua_close(void)
{
	lua_close(Lp);
	lua_loaded = 0;
	return 0;
}

void print_parser_map(struct parser_map *map)
{
	if (!map) {
		if (total_opt_map)
			map = total_opt_map;
		else
			return;
	}
	unsigned int count = 1;
	for (struct parser_map *i = map; i->name; i++) {
		printf("%i. %s\n", count++, i->name);
	}
}

struct parser_map *allocate_parser_map(struct parser_map *map)
{
	if (!map)
		return NULL;
	
	unsigned int map_size = 0;
	for (struct parser_map *i = map; i->name; i++) {
		map_size++;
	}
	map_size++;
	
	struct parser_map *alloc = calloc(map_size, sizeof(struct parser_map));
	
	unsigned int count = 0;
	for (struct parser_map *i = map; i->name; i++) {
		alloc[count++] = *i;
	}
	
	return alloc;
}

unsigned int update_parser_map(struct parser_map *map, struct parser_map **dest)
{
	if (!map)
		return 1;
	
	unsigned int map_size = 0;
	for (struct parser_map *i = *dest; i->name; i++) {
		map_size++;
	}
	
	unsigned int rem_map_size = 0;
	for (struct parser_map *i = map; i->name; i++) {
		rem_map_size++;
	}
	
	unsigned int updated = 0;
	for (int i = 0; i < rem_map_size; i++) {
		for (int j = 0; j < map_size; j++) {
			if (!strcmp((*dest)[j].name, map[i].name)) {
				(*dest)[j] = map[i];
				updated++;
				break;
			}
		}
	}
	
	return updated;
}

int register_parser_map(struct parser_map *map, struct parser_map **dest)
{
	if (!map)
		return 1;
	
	size_t map_size = 0;
	for (struct parser_map *i = map; i->name; i++) {
		map_size++;
	}
	map_size++; /* For the empty option at the end! */
	map_size *= sizeof(struct parser_map);
	
	unsigned int end_index = 0;
	size_t old_map_size = 0;
	if (*dest) {
		for (struct parser_map *i = *dest; i->name; i++) {
			old_map_size++;
		}
	}
	end_index = old_map_size;
	old_map_size *= sizeof(struct parser_map);
	
	*dest = realloc(*dest, old_map_size+map_size);
	
	for (struct parser_map *i = map; i->name; i++) {
		(*dest)[end_index++] = *i;
	}
	(*dest)[end_index] = (struct parser_map){0};
	
	return 0;
}

int unregister_parser_map(struct parser_map *map, struct parser_map **dest)
{
	if (!map)
		return 1;
	
	size_t map_size = 0;
	for (struct parser_map *i = *dest; i->name; i++) {
		map_size++;
	}
	
	size_t rem_map_size = 0;
	for (struct parser_map *i = map; i->name; i++) {
		rem_map_size++;
	}
	
	for (int i = 0; i < rem_map_size; i++) {
		for (int j = 0; j < map_size; j++) {
			if (!strcmp((*dest)[j].name, map[i].name)) {
				for (int k = j; k < map_size; k++) {
					(*dest)[k] = (*dest)[k+1];
				}
				map_size--;
				break;
			}
		}
	}
	map_size++;
	map_size *= sizeof(struct parser_map);
	
	*dest = realloc(*dest, map_size);
	
	return 0;
}

void free_input_parse_opts()
{
	free(total_opt_map);
}

/* Read options */
int parse_lua_simconf_options(struct parser_map *map)
{
	if (!map)
		return 1;
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){
		.i = 0,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 0,
		.file = {0},
		.buffer = {{0}},
		.object = NULL,
		.opt_map = map,
	};
	
	/* Read settings table */
	lua_getglobal(Lp, "settings");
	
	conf_traverse_table(Lp, &conf_lua_parse_opts, parser_state);
	
	if ((option->epsno == 0.0) || (option->elcharge == 0.0)) {
		option->noele = 1;
	} else {
		option->noele = 0;
	}
	if (option->gconst == 0.0) {
		option->nogrv = 1;
	} else {
		option->nogrv = 0;
	}
	
	return 0;
}

/* Read objects */
int parse_lua_simconf_objects(phys_obj **object, const char* sent_to_lua)
{
	lua_getglobal(Lp, option->spawn_funct);
	/* Can send arguments here, currently unused. */
	lua_pushstring(Lp, sent_to_lua);
	
	lua_call(Lp, 1, 1);
	
	/* Lua table "arrays" are indexed from 1, so offset that */
	option->obj = luaL_len(Lp, -1)-1;
	
	if(!lua_istable(Lp, -1)) {
		pprint_err("Lua f-n \"%s\" did not return a table of objects.\n",
				   option->spawn_funct);
		raise(SIGINT);
	}
	
	/* We still need to find the molfiles */
	conf_traverse_table(Lp, &conf_lua_parse_files, NULL);
	phys_init(object);
	/* Finally read the objects */
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){ 
		.i = 0,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 0,
		.file = {0},
		.buffer = {{0}},
		.object = *object,
	};
	
	conf_traverse_table(Lp, &conf_lua_parse_objs, parser_state);
	
	return 0;
}

int parse_lua_simconf_elements(const char *filepath)
{
	const char elements_internal[] =
	// Generated from elements.lua
	#include "resources/elements.h"
	;
	
	atom_prop = calloc(120, sizeof(struct atomic_cont));
	
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	
	/* Load file */
	if (filepath) {
		if(luaL_loadfile(L, filepath))
			pprintf(PRI_ERR, "Opening Lua file %s failed! Using internal DB.\n",
					filepath);
	} else {
		if(luaL_loadstring(L, elements_internal)) {
			pprintf(PRI_ERR, "Failed to open internal DB.\n");
			return 2;
		}
	}
	
	/* Execute script */
	lua_pcall(L, 0, 0, 0);
	/* Read settings table */
	lua_getglobal(L, "elements");
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){ 
		.i = 0,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 0,
		.file = {0},
		.buffer_ele = {0},
	};
	
	conf_traverse_table(L, *conf_lua_parse_elements, parser_state);
	
	lua_close(L);
	
	return 0;
}

static void parser_push_stat_array(lua_State *L, struct global_statistics *stats)
{
	/* Create "array" table. */
	lua_newtable(L);
	
	/* Any single variable go here */
	for (struct parser_map *i = stats->global_stats_map; i->name; i++) {
		parser_push_generic(L, i);
	}
	
	if (!stats->t_stats[0].thread_stats_map) {
		return;
	}
	
	/* Variables for each thread */
	for (unsigned int i = 0; i < phys_stats->threads; i++) {
		/* Create a table inside that to hold everything */
		lua_newtable(L);
		
		for (struct parser_map *j = stats->t_stats[i].thread_stats_map; j->name; j++) {
			parser_push_generic(L, j);
		}
		
		/* Record index */
		lua_rawseti(L, -2, i);
	}
}

static void parser_push_object_array(lua_State *L, phys_obj *obj)
{
	/* Create "array" table. */
	lua_newtable(L);
	for (int i = 0; i < option->obj; i++) {
		/* Create a table inside that to hold everything */
		lua_newtable(L);
		/* Push variables */
		
		/* Position table */
		lua_newtable(L);
		for (int j = 0; j < 3; j++) {
			lua_pushnumber(L, obj[i].pos[j]);
			lua_rawseti(L, -2, j+1);
		}
		lua_setfield(L, -2, "pos");
		
		/* Velocity table */
		lua_newtable(L);
		for (int j = 0; j < 3; j++) {
			lua_pushnumber(L, obj[i].vel[j]);
			lua_rawseti(L, -2, j+1);
		}
		lua_setfield(L, -2, "vel");
		
		/* Accel table */
		lua_newtable(L);
		for (int j = 0; j < 3; j++) {
			lua_pushnumber(L, obj[i].acc[j]);
			lua_rawseti(L, -2, j+1);
		}
		lua_setfield(L, -2, "acc");
		
		/* Everything else */
		lua_pushnumber(L, obj[i].mass);
		lua_setfield(L, -2, "mass");
		lua_pushnumber(L, obj[i].charge);
		lua_setfield(L, -2, "charge");
		lua_pushnumber(L, obj[i].radius);
		lua_setfield(L, -2, "radius");
		lua_pushinteger(L, obj[i].atomnumber);
		lua_setfield(L, -2, "atomnumber");
		lua_pushinteger(L, obj[i].id);
		lua_setfield(L, -2, "id");
		lua_pushinteger(L, obj[i].state);
		lua_setfield(L, -2, "state");
		lua_pushboolean(L, obj[i].ignore);
		lua_setfield(L, -2, "ignore");
		
		lua_rawseti(L, -2, i+1);
	}
}

unsigned int lua_exec_funct(const char *funct, phys_obj *object,
							struct global_statistics *stats)
{
	if (!funct && !lua_loaded)
		return 0;
	
	int num_args = 0;
	
	lua_getglobal(Lp, funct);
	
	if (1) {
		parser_push_stat_array(Lp, stats);
		num_args++;
	}
	
	if (option->lua_expose_obj_array) {
		parser_push_object_array(Lp, object);
		num_args++;
	}
	
	lua_call(Lp, num_args, 1);
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){ 
		.i = 0,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 1,
		.file = {0},
		.buffer = {{0}},
		.object = object,
	};
	
	if (!lua_isnil(Lp, -1) && !lua_isnil(Lp, -2)) {
		if (lua_istable(Lp, -1)) {
			conf_traverse_table(Lp, &conf_lua_parse_objs, parser_state);
			pprint_verb("Updated objs = %i\n", parser_state->i);
		} else {
			pprint_warn("Lua f-n \"%s\" did not return a table as objects. Ignoring.\n",
					   funct);
		}
	}
	
	return parser_state->i;
}

const char *parse_file_to_str(const char* filename)
{
	FILE* input = fopen(filename, "r");
	if (!input)
		return NULL;
	
	if (fseek(input, 0, SEEK_END) == -1)
		return NULL;
	
	size_t size = ftell(input);
	if (size == -1)
		return NULL;
	
	if (fseek(input, 0, SEEK_SET) == -1)
		return NULL;
	
	char *content = malloc(size + 1);
	if (!content)
		return NULL;
	
	fread(content, 1, size, input);
	
	if (ferror(input)) {
		free(content);
		return NULL;
	}
	
	fclose(input);
	content[size] = '\0';
	return content;
}
