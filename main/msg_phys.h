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
#ifndef PHYSENGINE_MSG
#define PHYSENGINE_MSG

enum PRIORITIY {
	 PRI_ESSENTIAL = 1,
	 PRI_VERYHIGH = 2,
	 PRI_HIGH = 3,
	 PRI_HIGHMED = 4,
	 PRI_MEDIUM = 5,
	 PRI_LOWMED = 6,
	 PRI_LOW = 7,
	 PRI_VERYLOW = 8,
	 PRI_SPAM = 9,
	 PRI_OK = 10,
	 PRI_WARN = 11,
	 PRI_ERR = 12,
};

void pprintf(int priority, const char *format, ...);

#define pprint_ok(...) pprintf(PRI_OK, __VA_ARGS__)
#define pprint_warn(...) pprintf(PRI_WARN, __VA_ARGS__)
#define pprint_err(...) pprintf(PRI_ERR, __VA_ARGS__)
#define pprint(...) pprintf(PRI_ESSENTIAL, __VA_ARGS__)
#define pprint_verb(...) pprintf(PRI_MEDIUM, __VA_ARGS__)
#define pprint_deb(...) pprintf(PRI_SPAM, __VA_ARGS__)

#endif
