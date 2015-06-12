/*
 * This file is part of glauss.
 * Copyright (c) 2013 Rostislav Pehlivanov <atomnuker@gmail.com>
 *
 * glauss is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glauss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glauss.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "physics/physics.h"
#include "parser.h"
#include "sighandle.h"
#include "shared/options.h"
#include "shared/msg_phys.h"
#include "graph/graph_thread.h"
#include "input_thread.h"

/* Report stats on command line */
void on_usr1_signal(int signo)
{
    pprintf(PRI_ESSENTIAL, "\n");
    if(!signo)
        pprintf(PRI_ESSENTIAL, "USR1 signal received, current stats:\n");

    if (!phys_stats) {
        pprint_err("Not running!\n");
        return;
    }

    for (struct parser_map *i = phys_stats->global_stats_map; i->name; i++) {
        char res[50];
        parser_get_value_str(*i, res, 50);
        printf("%s = %s\n", i->name, res);
    }

    if (phys_stats->t_stats) {
        for (unsigned int t = 0; t < phys_stats->threads; t++) {
            for (struct parser_map *i = phys_stats->t_stats[t].thread_stats_map; i->name; i++) {
                char res[50];
                parser_get_value_str(*i, res, 50);
                printf("%u.	%s = %s\n", t, i->name, res);
            }
        }
    }
}

/* Function given to signal handler */
void on_quit_signal(int signo)
{
    pprint_enable();
    pprintf(PRI_ESSENTIAL, "\nSignal to quit %i received!\n", signo);

    /* Physics */
    phys_ctrl(PHYS_SHUTDOWN, NULL);

    /* Lua */
    pprintf(PRI_ESSENTIAL, "Closing: Lua: ");
    parse_lua_close();
    free_input_parse_opts();
    pprintf(PRI_OK, "");

    /* Window */
    if(!option->novid) {
        pprintf(PRI_ESSENTIAL, "&& Window: ");
        option->novid = true;
        graph_thread_quit();
        pprintf(PRI_OK, "");
    }

    /* Input thread */
    if(option->input_thread_enable) {
        pprintf(PRI_ESSENTIAL, "&& Input: ");
        input_thread_quit();
        input_thread_reset_term();
        pprintf(PRI_OK, "");
    }

    /* Logfile */
    if(option->logenable) {
        pprintf(PRI_ESSENTIAL, "&& Log: ");
        option->logenable = false;
        fclose(option->logfile);
        pprintf(PRI_OK, "");
    }

    pprintf(PRI_ESSENTIAL, "\n");

    /* Main */
    option->quit_main_now = true;
}

void on_alrm_signal(int signo)
{
    pprint_warn("Watchdog timer kicked in! Program probably hanged up.\n");
}
