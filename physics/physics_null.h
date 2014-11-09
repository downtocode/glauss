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
#ifndef PHYSENGINE_PHYS_NULL
#define PHYSENGINE_PHYS_NULL

#define PHYS_NULL {                                                            \
		.name = "null",                                                        \
		.version = PACKAGE_VERSION,                                            \
		.desc = "Only a control thread running, use with Lua",                 \
		.author = "Rostislav Pehlivanov",                                      \
		.thread_preconfiguration = null_preinit,                               \
		.thread_configuration = null_init,                                     \
		.thread_location = thread_null,                                        \
		.thread_destruction = null_quit,                                       \
	}

#define PHYS_NULL_STATS {                                                      \
		.name = "null_stats",                                                  \
		.version = PACKAGE_VERSION,                                            \
		.desc = "Outputs statistics(n-body), slow",                            \
		.author = "Rostislav Pehlivanov",                                      \
		.thread_preconfiguration = NULL,                                       \
		.thread_configuration = null_init,                                     \
		.thread_location = thread_stats,                                       \
		.thread_destruction = null_quit,                                       \
	}

struct thread_config_null {
	data *obj;
	struct global_statistics *glob_stats;
	struct thread_statistics *stats;
	unsigned int id, objs_low, objs_high;
	pthread_barrier_t *ctrl;
	pthread_mutex_t *mute;
	bool *quit;
};

void *null_preinit(struct glob_thread_config *cfg);
void **null_init(struct glob_thread_config *cfg);
void null_quit(void **threads);
void *thread_stats(void *thread_setts);
void *thread_null(void *thread_setts);

#endif
