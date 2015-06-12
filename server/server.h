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
#pragma once

#include "physics/physics.h"

enum PHYS_SERVER_MODE {
    MODE_CLIENT,
    MODE_SERVER,
    MODE_LISTEN
};

/* server - the server handling and distributing everything, ID is always 1
* worker - subworkers, worker_num in total, random order
* id - unique identifier
* threads - server: the total amount, worker: the amount for that machine
* hostname - local machine's hostname
* algo - name of algorithm, guaranteed to exist
* obj - the entire set of objects
* stats - statistics */

struct phys_interface {
    struct phys_interface *server, **worker;
    unsigned int id, threads, worker_num, ver_major, ver_minor;
    const char *hostname, *simconf_id, *algorithm;
    phys_obj *obj;
    bool *quit;
    pthread_barrier_t *iface_sync;
    struct global_statistics *stats;
};
