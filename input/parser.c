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
#include "input_thread.h"
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
	data buffer, *object;
	struct parser_opt *opt_map;
};

static bool lua_loaded = 0;
static lua_State *L;
struct parser_opt *total_opt_map = NULL;

static int conf_lua_parse_opts(lua_State *L, struct lua_parser_state *parser_state)
{
	if (lua_istable(L, -1)) {
		/* Tell conf_traverse_table() to traverse */
		return 1;
	} else {
		int lua_vartype = lua_type(L, -1);
		const char *lua_varvalue = lua_tostring(L, -1);
		const char *lua_varname = lua_tostring(L, -2);
		for (struct parser_opt *i = parser_state->opt_map; i->name; i++) {
			if (lua_vartype == i->cmd_or_lua_type) {
				if (!strcmp(i->name, lua_varname)) {
					if(lua_vartype == LUA_TBOOLEAN) {
						/* lua_tostring() of LUA_TBOOLEAN returns NULL */
						lua_varvalue = lua_toboolean(L, -1) ? "true" : "false";
					}
					input_set_typed(i, lua_varvalue);
					break;
				}
			}
		}
	}
	return 0;
}

static void conf_lua_get_vector(lua_State *L, vec3 *vec)
{
	/* Can be used for N dim tables, change 3 to N */
	for(int i = 0; i < 3; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		(*vec)[i] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
}

static int conf_lua_parse_objs(lua_State *L, struct lua_parser_state *parser_state)
{
	/* Variables are read out of order so wait until we see the next object */
	if(lua_istable(L, -1)) {
		if(lua_type(L, -2) == LUA_TSTRING) {
			if(!strcmp("pos", lua_tostring(L, -2))) {
				conf_lua_get_vector(L, &parser_state->buffer.pos);
				return 0;
			} else if(!strcmp("vel", lua_tostring(L, -2))) {
				conf_lua_get_vector(L, &parser_state->buffer.vel);
				return 0;
			} else if(!strcmp("rot", lua_tostring(L, -2))) {
				conf_lua_get_vector(L, &parser_state->file.rot);
				return 0;
			}
		}
		if(parser_state->fileset) {
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
			if(parser_state->read_id) {
				/* Read changed object from Lua */
				if(parser_state->buffer.id > option->obj) {
					pprint_warn("Lua told us to change obj id = %i, but no such exists.\
								\nCheck your Lua exec_funct code. Skipping.\n",
								parser_state->buffer.id);
				} else {
					/*printf("Old = %lf\n", (parser_state->object)[parser_state->buffer.id].pos[0]);
					printf("New = %lf\n", parser_state->buffer.pos[0]);*/
					(parser_state->object)[parser_state->buffer.id] = parser_state->buffer;
					parser_state->i++;
				}
			} else if(parser_state->nullswitch) {
				/* We're reading an object from an entire array(on init) */
				parser_state->buffer.id = parser_state->i;
				(parser_state->object)[parser_state->i] = parser_state->buffer;
				parser_state->i++;
			} else parser_state->nullswitch = 1;
			/* Cheap hack. Lua's first object is always, always CORRUPTED */
		}
		/* Object/file finished, go to next */
		return 1;
	} else if(lua_isnumber(L, -1)) {
		if(!strcmp("charge", lua_tostring(L, -2)))
			parser_state->buffer.charge = lua_tonumber(L, -1)*option->elcharge;
		if(!strcmp("mass", lua_tostring(L, -2)))
			parser_state->buffer.mass = lua_tonumber(L, -1);
		if(!strcmp("radius", lua_tostring(L, -2)))
			parser_state->buffer.radius = lua_tonumber(L, -1);
		if(!strcmp("atomnumber", lua_tostring(L, -2)))
			parser_state->buffer.atomnumber = lua_tonumber(L, -1);
		if(!strcmp("scale", lua_tostring(L, -2)))
			parser_state->file.scale = lua_tonumber(L, -1);
		if(parser_state->read_id) {
			if(!strcmp("id", lua_tostring(L, -2))) {
				parser_state->buffer.id = lua_tonumber(L, -1);
			}
		}
	} else if(lua_isstring(L, -1)) {
		/* It's a file to import, so we set the flag */
		if(!strcmp("import", lua_tostring(L, -2))) {
			strcpy(parser_state->file.filename, lua_tostring(L, -1));
			parser_state->fileset = 1;
		}
		if(!strcmp("atom", lua_tostring(L, -2))) {
			parser_state->buffer.atomnumber = return_atom_num(lua_tostring(L, -1));
		}
	} else if(lua_isboolean(L, -1)) {
		if(!strcmp("ignore", lua_tostring(L, -2)))
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

static void conf_traverse_table(lua_State *L, int (rec_fn(lua_State *, struct lua_parser_state *)),
								struct lua_parser_state *parser_state)
{
	if (!lua_loaded) {
		pprintf(PRI_ERR, "Lua context not loaded, no file/string to read!\n");
		raise(SIGINT);
	}
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
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

int parse_lua_open_file(const char *filename)
{
	if(lua_loaded) {
		pprintf(PRI_WARN, "Closing previous Lua context\n");
		parse_lua_close();
	}
	L = luaL_newstate();
	luaL_openlibs(L);
	
	/* Register own function to quit */
	lua_register(L, "raise", input_lua_raise);
	
	/* Load file */
	if(luaL_loadfile(L, filename)) {
		pprintf(PRI_ERR, "Opening Lua file %s failed!\n", filename);
		return 2;
	}
	/* Execute script */
	lua_pcall(L, 0, 0, 0);
	
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
	L = luaL_newstate();
	luaL_openlibs(L);
	
	/* Register own function to quit */
	lua_register(L, "raise", input_lua_raise);
	
	/* Load file */
	if(luaL_loadstring(L, script)) {
		pprintf(PRI_ERR, "Opening Lua script failed!\n");
		return 2;
	}
	
	/* Execute script */
	lua_pcall(L, 0, 0, 0);
	
	return 0;
}

/* Close lua file */
int parse_lua_close(void)
{
	lua_close(L);
	lua_loaded = 0;
	return 0;
}

void print_input_parse_opts(struct parser_opt *map)
{
	if (!map) {
		map = total_opt_map;
	}
	unsigned int count = 1;
	for (struct parser_opt *i = total_opt_map; i->name; i++) {
		printf("%i. %s\n", count++, i->name);
	}
}

struct parser_opt *allocate_input_parse_opts(struct parser_opt *map)
{
	if (!map)
		return NULL;
	
	size_t map_size = 0;
	unsigned int count = 0;
	struct parser_opt *alloc = NULL;
	for (struct parser_opt *i = map; i->name; i++) {
		map_size++;
	}
	map_size *= sizeof(struct parser_opt);
	
	alloc = malloc(map_size);
	
	for (struct parser_opt *i = map; i->name; i++) {
		alloc[count++] = *i;
	}
	
	return alloc;
}

unsigned int update_input_parse_opts(struct parser_opt *map)
{
	if (!map)
		return 1;
	
	unsigned int map_size = 0;
	for (struct parser_opt *i = total_opt_map; i->name; i++) {
		map_size++;
	}
	
	unsigned int rem_map_size = 0;
	for (struct parser_opt *i = map; i->name; i++) {
		rem_map_size++;
	}
	
	unsigned int updated = 0;
	for (int i = 0; i < rem_map_size; i++) {
		for (int j = 0; j < map_size; j++) {
			if (!strcmp(total_opt_map[j].name, map[i].name)) {
				total_opt_map[j] = map[i];
				updated++;
				break;
			}
		}
	}
	
	return updated;
}

int register_input_parse_opts(struct parser_opt *map)
{
	if (!map)
		return 1;
	
	size_t map_size = 0;
	for (struct parser_opt *i = map; i->name; i++) {
		map_size++;
	}
	
	map_size *= sizeof(struct parser_opt);
	
	unsigned int end_index = 0;
	size_t old_map_size = 0;
	if (total_opt_map) {
		for (struct parser_opt *i = total_opt_map; i->name; i++) {
			old_map_size++;
		}
	}
	end_index = old_map_size;
	old_map_size *= sizeof(struct parser_opt);
	
	total_opt_map = realloc(total_opt_map, old_map_size+map_size+1);
	
	for (struct parser_opt *i = map; i->name; i++) {
		total_opt_map[end_index++] = *i;
	}
	
	return 0;
}

int unregister_input_parse_opts(struct parser_opt *map)
{
	if (!map)
		return 1;
	
	size_t map_size = 0;
	for (struct parser_opt *i = total_opt_map; i->name; i++) {
		map_size++;
	}
	
	size_t rem_map_size = 0;
	for (struct parser_opt *i = map; i->name; i++) {
		rem_map_size++;
	}
	
	for (int i = 0; i < rem_map_size; i++) {
		for (int j = 0; j < map_size; j++) {
			if (!strcmp(total_opt_map[j].name, map[i].name)) {
				for (int k = j; k < map_size; k++) {
					total_opt_map[k] = total_opt_map[k+1];
				}
				map_size--;
				break;
			}
		}
	}
	map_size++;
	map_size *= sizeof(struct parser_opt);
	
	total_opt_map = realloc(total_opt_map, map_size);
	
	return 0;
}

void free_input_parse_opts()
{
	free(total_opt_map);
}

/* Read options */
int parse_lua_simconf_options()
{
	struct lua_parser_state *parser_state = &(struct lua_parser_state){
		.i = 1,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 0,
		.file = {0},
		.buffer = {{0}},
		.object = NULL,
		.opt_map = total_opt_map,
	};
	
	/* Read settings table */
	lua_getglobal(L, "settings");
	
	pprint_disable();
	conf_traverse_table(L, &conf_lua_parse_opts, parser_state);
	pprint_enable();
	
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
int parse_lua_simconf_objects(data **object, const char* sent_to_lua)
{
	lua_getglobal(L, option->spawn_funct);
	/* Can send arguments here, currently unused. */
	lua_pushstring(L, sent_to_lua);
	/* The second returned value is the total number of objects */
	lua_call(L, 1, 2);
	/* Lua lies when reporting how many objects there are. Either that or us. */
	if(lua_isnumber(L, -1)) {
		option->obj = lua_tonumber(L, -1)-1;
	} else {
		pprint_err("Lua f-n \"%s\" did not return an integer to tell us the number of objects.\n",
				   option->spawn_funct);
		raise(SIGINT);
	}
	lua_pop(L, 1);
	
	if(!lua_istable(L, -1)) {
		pprint_err("Lua f-n \"%s\" did not return a table of objects.\n",
				   option->spawn_funct);
		raise(SIGINT);
	}
	
	/* We still need to find the molfiles */
	conf_traverse_table(L, &conf_lua_parse_files, NULL);
	phys_init(object);
	/* Finally read the objects */
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){ 
		.i = 1,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 0,
		.file = {0},
		.buffer = {{0}},
		.object = *object,
	};
	
	conf_traverse_table(L, &conf_lua_parse_objs, parser_state);
	
	return 0;
}

static void lua_push_stat_array()
{
	/* Create "array" table. */
	lua_newtable(L);
	
	/* Any single variable go here */
	lua_pushnumber(L, phys_stats->progress);
	lua_setfield(L, -2, "progress");
	lua_pushnumber(L, phys_stats->time_running);
	lua_setfield(L, -2, "time_running");
	lua_pushnumber(L, phys_stats->rng_seed);
	lua_setfield(L, -2, "rng_seed");
	lua_pushnumber(L, phys_stats->time_per_step);
	lua_setfield(L, -2, "time_per_step");
	lua_pushnumber(L, phys_stats->steps);
	lua_setfield(L, -2, "steps");
	
	/* Null */
	lua_pushnumber(L, phys_stats->null_avg_dist);
	lua_setfield(L, -2, "null_avg_dist");
	lua_pushnumber(L, phys_stats->null_max_dist);
	lua_setfield(L, -2, "null_max_dist");
	
	/* Barnes-Hut */
	lua_pushnumber(L, phys_stats->bh_total_alloc);
	lua_setfield(L, -2, "bh_total_alloc");
	lua_pushnumber(L, phys_stats->bh_new_alloc);
	lua_setfield(L, -2, "bh_new_alloc");
	lua_pushnumber(L, phys_stats->bh_new_cleaned);
	lua_setfield(L, -2, "bh_new_cleaned");
	lua_pushnumber(L, phys_stats->bh_heapsize);
	lua_setfield(L, -2, "bh_heapsize");
	
	/* Variables for each thread */
	for(short i = 1; i < option->threads + 1; i++) {
		/* Create a table inside that to hold everything */
		lua_newtable(L);
		
		/* Shared */
		lua_pushnumber(L, phys_stats->t_stats[i].clockid);
		lua_setfield(L, -2, "clockid");
		
		/* Barnes-Hut */
		lua_pushnumber(L, phys_stats->t_stats[i].bh_total_alloc);
		lua_setfield(L, -2, "bh_total_alloc");
		lua_pushnumber(L, phys_stats->t_stats[i].bh_new_alloc);
		lua_setfield(L, -2, "bh_new_alloc");
		lua_pushnumber(L, phys_stats->t_stats[i].bh_new_cleaned);
		lua_setfield(L, -2, "bh_new_cleaned");
		lua_pushnumber(L, phys_stats->t_stats[i].bh_heapsize);
		lua_setfield(L, -2, "bh_heapsize");
		
		/* Null */
		lua_pushnumber(L, phys_stats->t_stats[i].null_avg_dist);
		lua_setfield(L, -2, "null_avg_dist");
		lua_pushnumber(L, phys_stats->t_stats[i].null_max_dist);
		lua_setfield(L, -2, "null_max_dist");
		
		/* Record index */
		lua_rawseti(L, -2, i);
	}
}

static void lua_push_object_array(data *obj)
{
	/* Create "array" table. */
	lua_newtable(L);
	for (int i = 1; i < option->obj + 1; i++) {
		/* Create a table inside that to hold everything */
		lua_newtable(L);
		/* Push variables */
		
		/* Position table */
		lua_newtable(L);
		for (int j = 0; j < 3; j++) {
			lua_pushnumber(L, obj[i].pos[j]);
			lua_rawseti(L, -2, j);
		}
		lua_setfield(L, -2, "pos");
		
		/* Velocity table */
		lua_newtable(L);
		for (int j = 0; j < 3; j++) {
			lua_pushnumber(L, obj[i].vel[j]);
			lua_rawseti(L, -2, j);
		}
		lua_setfield(L, -2, "vel");
		
		/* Everything else */
		lua_pushnumber(L, obj[i].mass);
		lua_setfield(L, -2, "mass");
		lua_pushnumber(L, obj[i].charge);
		lua_setfield(L, -2, "charge");
		lua_pushnumber(L, obj[i].radius);
		lua_setfield(L, -2, "radius");
		lua_pushnumber(L, obj[i].atomnumber);
		lua_setfield(L, -2, "atomnumber");
		lua_pushnumber(L, obj[i].id);
		lua_setfield(L, -2, "id");
		lua_pushboolean(L, obj[i].ignore);
		lua_setfield(L, -2, "ignore");
		
		lua_rawseti(L, -2, i);
	}
}

unsigned int lua_exec_funct(const char *funct, data *object)
{
	if (!funct && !lua_loaded)
		return 0;
	lua_getglobal(L, funct);
	
	lua_push_stat_array();
	
	int num_args = 1;
	if (option->lua_expose_obj_array) {
		lua_push_object_array(object);
		num_args++;
	}
	
	lua_call(L, num_args, 1);
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){ 
		.i = 0,
		.nullswitch = 0,
		.fileset = 0,
		.read_id = 1,
		.file = {0},
		.buffer = {{0}},
		.object = object,
	};
	
	if (!lua_isnil(L, -2)) {
		if (lua_istable(L, -2)) {
			conf_traverse_table(L, &conf_lua_parse_objs, parser_state);
			pprint_verb("Updated objs = %i\n", parser_state->i);
		} else {
			pprint_warn("Lua f-n \"%s\" did not return a table as objects. Ignoring.\n",
					   funct);
		}
	}
	
	return parser_state->i;
}

/* Currently unused, parse an external file into a const char string pointer */
const char *parse_file_to_str(const char* filename)
{
	FILE* input = fopen(filename, "r");
	if (!input) return NULL;
	
	if (fseek(input, 0, SEEK_END) == -1)
		return NULL;
	
	size_t size = ftell(input);
	if (size == -1)
		return NULL;
	
	if (fseek(input, 0, SEEK_SET) == -1)
		return NULL;
	
	char *content = malloc((size_t)size + 1);
	if (!content) return NULL;
	
	fread(content, 1, (size_t)size, input);
	
	if (ferror(input)) {
		free(content);
		return NULL;
	}
	
	fclose(input);
	content[size] = '\0';
	return content;
}
