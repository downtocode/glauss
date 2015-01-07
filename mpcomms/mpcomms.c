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
#include <stdlib.h>
#include <string.h>
#include "mpcomms.h"
#include "main/options.h"
#include "config.h"

/* Heavy work in progress, will probably use standard unix sockets */

int mpcomms_fetch_unique_client_id(unsigned int *id)
{
	/* Get a single node here and ask it for all clients */
	
	struct phys_interface *rem_iface = NULL;
	
	unsigned int *id_db = calloc(rem_iface->worker_num, sizeof(unsigned int));
	unsigned int counter = 0;
	unsigned int id_local = 0;
	for (int i = 0; i < rem_iface->worker_num + 1; i++) {
		id_db[counter++] = rem_iface->worker[i]->id;
	}
	
	matches:
		id_local = rand();
		for (int i = 0; i < counter; i++) {
			if (id_db[i] == id_local) {
				/* It's directly mapped to assembly and my brain is fried right
				 * now, if you got a better idea not using goto please say so */
				goto matches;
			}
		}
	
	*id = id_local;
	
	free(id_db);
	
	return 0;
}

int mpcomms_init(struct glob_thread_config *cfg, enum MPCOMMS_MODE mode, enum MPCOMMS_METHOD method)
{
	struct phys_interface *iface = calloc(1, sizeof(struct phys_interface));
	iface->ver_minor = COMMS_VER_MAJOR;
	iface->ver_minor = COMMS_VER_MINOR;
	iface->simconf_id = strdup(cfg->simconf_id);
	iface->algorithm = strdup(cfg->algorithm);
	iface->obj = cfg->obj;
	iface->stats = cfg->stats;
	switch (mode) {
		case MODE_CLIENT:
			mpcomms_fetch_unique_client_id(&iface->id);
			/* Broadcast ourselves to the server, which is still waiting */
			break;
		case MODE_SERVER:
			iface->id = 1;
			break;
		case MODE_LISTEN:
			/* Well I'd still like it to have an ID */
			mpcomms_fetch_unique_client_id(&iface->id);
			break;
		default:
			return 1;
			break;
	}
	
	/* Spawn thread */
	
	return 0;
}

void *mpcomms_thread(void *thread_setts)
{
	struct phys_interface *t = thread_setts;
	
	while (!*t->quit) {
		pthread_barrier_wait(t->iface_sync);
		
		
		pthread_barrier_wait(t->iface_sync);
	}
	
	return 0;
}
