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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <tgmath.h>
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include "physics_aux.h"
#include "main/msg_phys.h"
#include "main/options.h"

struct atomic_cont *atom_prop;

struct lua_parser_state {
	int i, buffer_cont;
	bool nullswitch;
	bool colset;
};

static const char elements_internal[] =
// Generated from elements.lua
#include "physics/resources/elements.h"
;

static void elements_traverse_table(lua_State *L, struct atomic_cont *buffer,
									struct lua_parser_state *parser_state)
{
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(lua_istable(L, -1)) {
			
			if(parser_state->nullswitch) {
				atom_prop[parser_state->i++] = *buffer;
			} else parser_state->nullswitch = 1;
			elements_traverse_table(L, buffer, parser_state);
			
		} else if(lua_isnumber(L, -1)) {
			
			if(!strcmp("mass", lua_tostring(L, -2)))
				buffer->mass = lua_tonumber(L, -1);
			if(!strcmp("R", lua_tostring(L, -2)))
				buffer->color[0] = lua_tonumber(L, -1);
			if(!strcmp("G", lua_tostring(L, -2)))
				buffer->color[1] = lua_tonumber(L, -1);
			if(!strcmp("B", lua_tostring(L, -2)))
				buffer->color[2] = lua_tonumber(L, -1);
			if(!strcmp("A", lua_tostring(L, -2)))
				buffer->color[3] = lua_tonumber(L, -1);
			
		} else if(lua_isstring(L, -1)) {
			
			if(!strcmp("name", lua_tostring(L, -2))) 
				buffer->name = strdup(lua_tostring(L, -1));
			
		}
		lua_pop(L, 1);
	}
}

int init_elements(const char *filepath)
{
	atom_prop = calloc(120, sizeof(struct atomic_cont));
	
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	
	/* Load file */
	if(filepath) {
		if(luaL_loadfile(L, "./resources/elements.lua"))
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
	
	struct atomic_cont buffer = {0};
	struct lua_parser_state parser_state = {1, 0, 0, 0};
	
	elements_traverse_table(L, &buffer, &parser_state);
	
	lua_close(L);
	
	return 0;
}

unsigned short int return_atom_num(const char *name)
{
	if(!name || name[0] == '\0') return 0;
	for(int i=1; i<121; i++) {
		if(strcmp(name, atom_prop[i].name) == 0) {
			return i;
		}
	}
	return 0;
}

const char *return_atom_str(unsigned int num)
{
	if(num > 120) return NULL;
	else return atom_prop[num].name;
}

/* Function to input numbers in an array and later extract a single number out.
 * Used in selecting objects. */
int getnumber(struct numbers_selection *numbers, int currentdigit, unsigned int status)
{
	if(status == NUM_ANOTHER) {
		numbers->digits[numbers->final_digit] = currentdigit;
		numbers->final_digit+=1;
		return 0;
	} else if(status == NUM_GIVEME) {
		unsigned int result = 0;
		for(int i=0; i < numbers->final_digit; i++) {
			result*=10;
			result+=numbers->digits[i];
		}
		numbers->final_digit = 0;
		return result;
	} else if(status == NUM_REMOVE) {
		numbers->final_digit-=1;
		return 0;
	}
	return 0;
}

/* Change algorithms */
void phys_shuffle_algorithms()
{
	if(option->status) {
		pprintf(PRI_WARN, "Physics needs to be stopped before changing modes.\n");
		return;
	}
	/* Shuffle algorithms */
	int num;
	/* Get number of algorithm */
	for(num = 0; phys_algorithms[num].name; num++) {
		if(!strcmp(option->algorithm, phys_algorithms[num].name))
			break;
	}
	/* Select next and check if we're on the last */
	if(!phys_algorithms[++num].name) num = 0;
	pprintf(PRI_HIGH, "Changing algorithm to \"%s\".\n",
			phys_algorithms[num].name);
	free(option->algorithm);
	option->algorithm = strdup(phys_algorithms[num].name);
}

/* We have to implement those here too as graphics may not be compiled */
void rotate_vec(vec3 *vec, vec3 *rot1)
{
	vec3 res = *vec, rot = *rot1;
	if(rot[0]) {
		double c = cos(rot[0]);
		double s = sin(rot[0]);
		res[1] = c*(*vec)[1] - s*(*vec)[2];
		res[2] = s*(*vec)[1] + c*(*vec)[2];
	}
	if(rot[1]) {
		vec3 tmp = res;
		double c = cos(rot[1]);
		double s = sin(rot[1]);
		res[0] = c*tmp[0] + s*tmp[2];
		res[2] = c*tmp[2] - s*tmp[0];
	}
	if(rot[2]) {
		vec3 tmp = res;
		double c = cos(rot[2]);
		double s = sin(rot[2]);
		res[0] = c*tmp[0] - s*tmp[1];
		res[1] = s*tmp[0] + c*tmp[1];
	}
	*vec = res;
}
