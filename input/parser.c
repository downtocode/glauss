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
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include "physics/physics.h"
#include "input/parser.h"
#include "main/options.h"
#include "input/in_file.h"
#include "main/msg_phys.h"

struct lua_parser_state {
	int i;
	bool nullswitch;
	bool fileset;
	in_file file;
	data buffer, *object;
};

static bool lua_loaded = 0;
static lua_State *L;

static int conf_lua_parse_opts(lua_State *L, struct lua_parser_state *parser_state)
{
	if(lua_istable(L, -1)) {
		/* Tell conf_traverse_table() to traverse */
		return 1;
	} else if(lua_isnumber(L, -1)) {
		/* lua_tonumber() returns double, so round them the C way */
		if(!strcmp("threads", lua_tostring(L, -2)))
			option->threads = lua_tonumber(L, -1);
		if(!strcmp("dt", lua_tostring(L, -2)))
			option->dt = lua_tonumber(L, -1);
		if(!strcmp("bh_ratio", lua_tostring(L, -2)))
			option->bh_ratio = lua_tonumber(L, -1);
		if(!strcmp("bh_lifetime", lua_tostring(L, -2)))
			option->bh_lifetime = lua_tonumber(L, -1);
		if(!strcmp("bh_heapsize_max", lua_tostring(L, -2)))
			option->bh_heapsize_max = lua_tonumber(L, -1);
		if(!strcmp("bh_tree_limit", lua_tostring(L, -2)))
			option->bh_tree_limit = lua_tonumber(L, -1);
		if(!strcmp("width", lua_tostring(L, -2)))
			option->width = lua_tonumber(L, -1);
		if(!strcmp("height", lua_tostring(L, -2)))
			option->height = lua_tonumber(L, -1);
		if(!strcmp("elcharge", lua_tostring(L, -2)))
			option->elcharge = lua_tonumber(L, -1);
		if(!strcmp("gconst", lua_tostring(L, -2)))
			option->gconst = lua_tonumber(L, -1);
		if(!strcmp("epsno", lua_tostring(L, -2)))
			option->epsno = lua_tonumber(L, -1);
		if(!strcmp("verbosity", lua_tostring(L, -2)))
			option->verbosity = lua_tonumber(L, -1);
		if(!strcmp("fontsize", lua_tostring(L, -2)))
			option->fontsize = lua_tonumber(L, -1);
		if(!strcmp("dump_xyz", lua_tostring(L, -2)))
			option->dump_xyz = lua_tonumber(L, -1);
		if(!strcmp("dump_sshot", lua_tostring(L, -2)))
			option->dump_sshot = lua_tonumber(L, -1);
		if(!strcmp("exec_funct_freq", lua_tostring(L, -2)))
			option->exec_funct_freq = lua_tonumber(L, -1);
	} else if(lua_isstring(L, -1)) {
		/* The defaults have been assigned using strdup too, so free them */
		if(!strcmp("algorithm", lua_tostring(L, -2))) {
			free(option->algorithm);
			option->algorithm = strdup(lua_tostring(L, -1));
		}
		if(!strcmp("spawn_funct", lua_tostring(L, -2))) {
			free(option->spawn_funct);
			option->spawn_funct = strdup(lua_tostring(L, -1));
		}
		if(!strcmp("timestep_funct", lua_tostring(L, -2))) {
			free(option->timestep_funct);
			option->timestep_funct = strdup(lua_tostring(L, -1));
		}
		if(!strcmp("fontname", lua_tostring(L, -2))) {
			free(option->fontname);
			option->fontname = strdup(lua_tostring(L, -1));
		}
		if(!strcmp("screenshot_template", lua_tostring(L, -2))) {
			free(option->sshot_temp);
			option->sshot_temp = strdup(lua_tostring(L, -1));
		}
		if(!strcmp("file_template", lua_tostring(L, -2))) {
			free(option->xyz_temp);
			option->xyz_temp = strdup(lua_tostring(L, -1));
		}
	} else if(lua_isboolean(L, -1)) {
		if(!strcmp("bh_single_assign", lua_tostring(L, -2)))
			option->bh_single_assign = lua_toboolean(L, -1);
		if(!strcmp("bh_random_assign", lua_tostring(L, -2)))
			option->bh_random_assign = lua_toboolean(L, -1);
	}
	return 0;
}

static int conf_lua_parse_objs(lua_State *L, struct lua_parser_state *parser_state)
{
	/* Variables are read out of order so wait until we see the next object */
	if(lua_istable(L, -1)) {
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
			if(parser_state->nullswitch) {
				parser_state->buffer.id = parser_state->i;
				(parser_state->object)[parser_state->i] = parser_state->buffer;
				parser_state->i++;
			} else parser_state->nullswitch = 1;
		}
		/* Object/file finished, go to next */
		return 1;
	} else if(lua_isnumber(L, -1)) {
		if(!strcmp("posx", lua_tostring(L, -2)))
			parser_state->buffer.pos[0] = lua_tonumber(L, -1);
		if(!strcmp("posy", lua_tostring(L, -2)))
			parser_state->buffer.pos[1] = lua_tonumber(L, -1);
		if(!strcmp("posz", lua_tostring(L, -2)))
			parser_state->buffer.pos[2] = lua_tonumber(L, -1);
		if(!strcmp("velx", lua_tostring(L, -2)))
			parser_state->buffer.vel[0] = lua_tonumber(L, -1);
		if(!strcmp("vely", lua_tostring(L, -2)))
			parser_state->buffer.vel[1] = lua_tonumber(L, -1);
		if(!strcmp("velz", lua_tostring(L, -2)))
			parser_state->buffer.vel[2] = lua_tonumber(L, -1);
		if(!strcmp("charge", lua_tostring(L, -2)))
			parser_state->buffer.charge = lua_tonumber(L, -1)*option->elcharge;
		if(!strcmp("mass", lua_tostring(L, -2)))
			parser_state->buffer.mass = lua_tonumber(L, -1);
		if(!strcmp("radius", lua_tostring(L, -2)))
			parser_state->buffer.radius = lua_tonumber(L, -1);
		if(!strcmp("atom", lua_tostring(L, -2)))
			parser_state->buffer.atomnumber = lua_tonumber(L, -1);
		if(!strcmp("scale", lua_tostring(L, -2)))
			parser_state->file.scale = lua_tonumber(L, -1);
		if(!strcmp("rotx", lua_tostring(L, -2)))
			parser_state->file.rot[0] = lua_tonumber(L, -1);
		if(!strcmp("roty", lua_tostring(L, -2)))
			parser_state->file.rot[1] = lua_tonumber(L, -1);
		if(!strcmp("rotz", lua_tostring(L, -2)))
			parser_state->file.rot[2] = lua_tonumber(L, -1);
	} else if(lua_isstring(L, -1)) {
		/* It's a file to import, so we set the flag */
		if(!strcmp("import", lua_tostring(L, -2))) {
			strcpy(parser_state->file.filename, lua_tostring(L, -1));
			parser_state->fileset = 1;
		}
	} else if(lua_isboolean(L, -1)) {
		if(!strcmp("ignore", lua_tostring(L, -2)))
			parser_state->buffer.ignore = lua_toboolean(L, -1);
	}
	return 0;
}

static void conf_traverse_table(lua_State *L, int (rec_fn(lua_State *, struct lua_parser_state *)),
								struct lua_parser_state *parser_state)
{
	if(!lua_loaded) {
		pprintf(PRI_ERR, "Lua context not loaded, no file/string to read!\n");
		raise(SIGINT);
	}
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(rec_fn(L, parser_state)) {
			/* Funtion pointer will return 1 if we need to go deeper */
			conf_traverse_table(L, rec_fn, parser_state);
		}
		lua_pop(L, 1);
	}
}

/* We have to know the exact amount of objects we need memory for so we scan. */
static void molfiles_traverse_table(lua_State *L)
{
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(lua_istable(L, -1)) {
			molfiles_traverse_table(L);
		} else if(lua_isstring(L, -1)) {
			if(!strcmp("import", lua_tostring(L, -2))) {
				if(!access(lua_tostring(L, -1), R_OK)) {
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
		lua_pop(L, 1);
	}
}

int parse_lua_open_file(const char *filename)
{
	if(lua_loaded) {
		pprintf(PRI_WARN, "Closing previous Lua context\n");
		parse_lua_close();
	} else lua_loaded = 1;
	L = luaL_newstate();
	luaL_openlibs(L);
	/* Load file */
	if(luaL_loadfile(L, filename)) {
		pprintf(PRI_ERR, "Opening Lua file %s failed!\n", filename);
		return 2;
	}
	/* Execute script */
	lua_pcall(L, 0, 0, 0);
	return 0;
}

int parse_lua_open_string(const char *script)
{
	if(lua_loaded) {
		pprintf(PRI_WARN, "Closing previous Lua context\n");
		parse_lua_close();
	} else lua_loaded = 1;
	L = luaL_newstate();
	luaL_openlibs(L);
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
int parse_lua_close()
{
	lua_close(L);
	lua_loaded = 0;
	return 0;
}

/* Read options */
int parse_lua_simconf_options()
{
	/* Read settings table */
	lua_getglobal(L, "settings");
	conf_traverse_table(L, &conf_lua_parse_opts, NULL);
	
	if((option->epsno == 0.0) || (option->elcharge == 0.0)) {
		option->noele = 1;
	} else {
		option->noele = 0;
	}
	if(option->gconst == 0.0) {
		option->nogrv = 1;
	} else {
		option->nogrv = 0;
	}
	return 0;
}

/* Read objects */
int parse_lua_simconf_objects(data **object)
{
	lua_getglobal(L, option->spawn_funct);
	/* Can send arguments here, currently unused. */
	lua_pushnumber(L, 0);
	/* The second returned value is the total number of objects */
	lua_call(L, 1, 2);
	/* Lua lies when reporting how many objects there are. Either that or us. */
	option->obj = lua_tonumber(L, -1)-1;
	lua_pop(L, 1);
	
	/* We still need to find the molfiles */
	molfiles_traverse_table(L);
	phys_init(object);
	/* Finally read the objects */
	
	struct lua_parser_state *parser_state = &(struct lua_parser_state){ 
		.i = 1,
		.nullswitch = 0,
		.fileset = 0,
		.file = {0},
		.buffer = {{0}},
		.object = *object,
	};
	
	conf_traverse_table(L, &conf_lua_parse_objs, parser_state);
	
	return 0;
}

double lua_exec_funct(const char *funct) {
	if(!funct) return 0;
	lua_getglobal(L, funct);
	lua_pushnumber(L, t_stats[1]->progress);
	lua_call(L, 1, 1);
	return lua_tonumber(L, -1);
}

/* Currently unused, parse an external file into a const char string pointer */
const char *parse_file_to_str(const char* filename)
{
	FILE* input = fopen(filename, "r");
	if(input == NULL) return NULL;
	
	if(fseek(input, 0, SEEK_END) == -1) return NULL;
	size_t size = ftell(input);
	if(size == -1) return NULL;
	if(fseek(input, 0, SEEK_SET) == -1) return NULL;
	
	char *content = malloc((size_t)size + 1);
	if(content == NULL) return NULL;
	
	fread(content, 1, (size_t)size, input);
	
	if(ferror(input)) {
		free(content);
		return NULL;
	}
	
	fclose(input);
	content[size] = '\0';
	return content;
}
