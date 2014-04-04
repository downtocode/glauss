/*
 * This file is part of physengine.
 * Copyright (c) 2012 Rostislav Pehlivanov
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

#define PRI_ESSENTIAL 1
#define PRI_VERYHIGH 2
#define PRI_HIGH 3
#define PRI_HIGHMED 4
#define PRI_MEDIUM 5
#define PRI_LOWMED 6
#define PRI_LOW 7
#define PRI_VERYLOW 8
#define PRI_SPAM 9

void pprintf(unsigned int priority, const char *format, ...);

#endif
