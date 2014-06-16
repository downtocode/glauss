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
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include "physics.h"
#include "parser.h"
#include "options.h"
#include "molreader.h"
#include "msg_phys.h"

static void conf_traverse_table(lua_State *L)
{
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(lua_istable(L, -1)) {
			conf_traverse_table(L);
		} else if(lua_isnumber(L, -1)) {
			if(!strcmp("threads", lua_tostring(L, -2)))
				option->avail_cores = (unsigned short int)lua_tonumber(L, -1);
			if(!strcmp("dt", lua_tostring(L, -2)))
				option->dt = lua_tonumber(L, -1);
			if(!strcmp("width", lua_tostring(L, -2)))
				option->width = (int)lua_tonumber(L, -1);
			if(!strcmp("height", lua_tostring(L, -2)))
				option->height = (int)lua_tonumber(L, -1);
			if(!strcmp("elcharge", lua_tostring(L, -2)))
				option->elcharge = lua_tonumber(L, -1);
			if(!strcmp("gconst", lua_tostring(L, -2)))
				option->gconst = lua_tonumber(L, -1);
			if(!strcmp("epsno", lua_tostring(L, -2)))
				option->epsno = lua_tonumber(L, -1);
			if(!strcmp("verbosity", lua_tostring(L, -2)))
				option->verbosity = (unsigned short int)lua_tonumber(L, -1);
		} else if(lua_isstring(L, -1)) {
			if(!strcmp("algorithm", lua_tostring(L, -2)))
				option->algorithm = strdup(lua_tostring(L, -1));
			if(!strcmp("fontname", lua_tostring(L, -2)))
				option->fontname = strdup(lua_tostring(L, -1));
		}
		lua_pop(L, 1);
	}
}

static int i = 1;
/* Lua's tables are weird since upon the very first traversal we're given
 * valid strings for variables in the later layers of the table(posx, radius, etc)
 * however they are 0. This throws the count off and causes explosions.
 * Using a one-time switch seems to do the trick. Temorary solution. */
static bool nullswitch = 0;
static bool molset = 0;
static char molfile[100];

static void obj_traverse_table(lua_State *L, data** object, data *buffer) {
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(lua_istable(L, -1)) {
			if(molset) {
				if(!access(molfile, R_OK)) {
					pprintf(PRI_OK, "File %s found!\n", molfile);
					readmolecule(*object, buffer, molfile, &i);
					memset(molfile, 0, sizeof(molfile));
					molset = 0;
				} else {
					pprintf(PRI_ERR, "File %s not found!\n", molfile);
					exit(1);
				}
			} else {
				/* It's just an object. */
				if(nullswitch) {
					buffer->id = i;
					(*object)[i] = *buffer;
					pprintf(PRI_SPAM, "Object %i here = {%lf, %lf, %lf}\n", i, buffer->pos[0], buffer->pos[1], buffer->pos[2]);
					i++;
				} else
					nullswitch = 1;
			}
			obj_traverse_table(L, object, buffer);
		} else if(lua_isnumber(L, -1)) {
			if(!strcmp("posx", lua_tostring(L, -2)))
				buffer->pos[0] = lua_tonumber(L, -1);
			if(!strcmp("posy", lua_tostring(L, -2)))
				buffer->pos[1] = lua_tonumber(L, -1);
			if(!strcmp("posz", lua_tostring(L, -2)))
				buffer->pos[2] = lua_tonumber(L, -1);
			if(!strcmp("velx", lua_tostring(L, -2)))
				buffer->vel[0] = lua_tonumber(L, -1);
			if(!strcmp("vely", lua_tostring(L, -2)))
				buffer->vel[1] = lua_tonumber(L, -1);
			if(!strcmp("velz", lua_tostring(L, -2)))
				buffer->vel[2] = lua_tonumber(L, -1);
			if(!strcmp("charge", lua_tostring(L, -2)))
				buffer->charge = lua_tonumber(L, -1)*option->elcharge;
			if(!strcmp("mass", lua_tostring(L, -2)))
				buffer->mass = lua_tonumber(L, -1);
			if(!strcmp("radius", lua_tostring(L, -2)))
				buffer->radius = lua_tonumber(L, -1);
			if(!strcmp("atom", lua_tostring(L, -2)))
				buffer->atomnumber = (unsigned short int)lua_tonumber(L, -1);
		} else if(lua_isstring(L, -1)) {
			if(!strcmp("molfile", lua_tostring(L, -2))) {
				strcpy(molfile, lua_tostring(L, -1));
				molset = 1;
			}
		} else if(lua_isboolean(L, -1)) {
			if(!strcmp("ignore", lua_tostring(L, -2)))
				buffer->ignore = lua_toboolean(L, -1);
		}
		lua_pop(L, 1);
	}
}

/* Using realloc is too much of a mess when we try to import molecules, so we scan beforehand. */
static void molfiles_traverse_table(lua_State *L) {
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(lua_istable(L, -1)) {
			molfiles_traverse_table(L);
		} else if(lua_isstring(L, -1)) {
			if(!strcmp("molfile", lua_tostring(L, -2))) {
				if(!access(lua_tostring(L, -1), R_OK)) {
					pprintf(PRI_OK, "File %s found!\n", lua_tostring(L, -1));
					/* Lua counts a molecule as a single object which we need to get rid of. */
					option->obj += probefile(lua_tostring(L, -1)) - 1;
				} else {
					pprintf(PRI_ERR, "File %s not found!\n", lua_tostring(L, -1));
					exit(1);
				}
			}
		}
		lua_pop(L, 1);
	}
}

int parse_lua_simconf_options(char *filename) {
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	/* Load file */
	if(luaL_loadfile(L, filename)) {
		pprintf(PRI_ERR, "Opening Lua file %s failed!\n", filename);
		return 1;
	}
	/* Execute script */
	lua_pcall(L, 0, 0, 0);
	/* Read settings table */
	lua_getglobal(L, "settings");
	conf_traverse_table(L);
	
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
	lua_close(L);
	return 0;
}

int parse_lua_simconf_objects(char *filename, data** object)
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	/* Load file */
	if(luaL_loadfile(L, filename)) {
		pprintf(PRI_ERR, "Opening Lua file %s failed!\n", filename);
		return 1;
	}
	
	lua_pcall(L, 0, 0, 0);
	
	/* Read returned table of objects */
	lua_getglobal(L, "spawn_objects");
	lua_pushnumber(L, option->obj);
	/* The second returned value is the total number of objects */
	lua_call(L, 1, 2);
	/* Lua arrays are indexed from 1. Luckily, our object array is also
	 * indexed from 1 as the 0th object is the identity and is always at {0}. */
	option->obj = (unsigned int)lua_tonumber(L, -1)-1;
	lua_pop(L, 1);
	
	/* We still need to find the molfiles */
	molfiles_traverse_table(L);
	initphys(object);
	/* Finally read the objects */
	data buffer;
	obj_traverse_table(L, object, &buffer);
	
	lua_close(L);
	return 0;
}

const char* readshader(const char* filename)
{
	FILE* input = fopen(filename, "r");
	if(input == NULL) return NULL;
	
	if(fseek(input, 0, SEEK_END) == -1) return NULL;
	long size = ftell(input);
	if(size == -1) return NULL;
	if(fseek(input, 0, SEEK_SET) == -1) return NULL;
	
	char *content = malloc((size_t)size +1);
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
