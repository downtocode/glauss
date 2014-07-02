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
#ifndef PHYSENGINE_PHYS_N_BODY
#define PHYSENGINE_PHYS_N_BODY

#define sigma 0.5
#define epsilon 0.0001

struct thread_config_nbody {
	data* obj;
	unsigned int id, objs_low, objs_high;
	struct thread_statistics *stats;
};

void** nbody_init(data** object, struct thread_statistics **stats);
void *thread_nbody(void *thread_setts);

#endif
