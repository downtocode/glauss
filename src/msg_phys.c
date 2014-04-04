/*
 * This file is part of physengine.
 * Copyright (c) 2012 Rostislav Pehlivanov <atomnuker@gmail.com>
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
#include <stdarg.h>
#include "msg_phys.h"
#include "options.h"

/* pprintf - a priority printing function. */

void pprintf(unsigned int priority, const char *format, ...)
{
	va_list args;
	if(priority <= option->verbosity) {
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		if(option->logenable) {
			va_start(args, format);
			vfprintf(option->logfile, format, args);
			va_end(args);
		}
	}
}
