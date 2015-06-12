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
#include "server.h"
#include "input/parser.h"
#include "shared/options.h"
#include "shared/msg_phys.h"
#include "config.h"

struct option_struct *option;

int main(int argc, char *argv[])
{
    /*  Default settings.  */
        option = &(struct option_struct ){
            /* Visuals */
            .def_radius = 1.0,
            .width = 1280, .height = 720,
            .fontsize = 38,
            .fontname = strdup("Sans"),
            .default_draw_mode = strdup("MODE_POINTS_COL"),
            .spawn_funct = strdup("spawn_objects"),
            .timestep_funct = NULL,
            .exec_funct_freq = 0,
            .lua_expose_obj_array = 0,
            .quit_main_now = 0,
            .skip_model_vec = 250,
            .novid = 0,
            .rng_seed = 0,

            /* Graph */
            .bgcolor[0] = 0.06,
            .bgcolor[1] = 0.06,
            .bgcolor[2] = 0.06,
            .bgcolor[3] = 1.0,
            .lua_print = NULL,

            /* Input */
            .input_thread_enable = 1,
            .elements_file = NULL,
            .lua_gc_sweep_freq = 1000,

            /* Physics */
            .threads = 0,
            .dt = 0.008, .verbosity = 5,
            .gconst = 0,
            .algorithm = strdup("none"),
            .thread_schedule_mode = strdup("SCHED_RR"),
            .custom_sprite_png = NULL,
            .step_back_buffer = 0,

            /* Physics - ctrl */
            .dump_xyz = 0,
            .xyz_temp = strdup("system_%0.2Lf.xyz"),
            .dump_sshot = 0,
            .sshot_temp = strdup("sshot_%3.3Lf.png"),
        };
    /*  Default settings.  */

    /*  Register parser options */
        struct parser_map opts_map[] = {
            {"threads",                P_TYPE(option->threads)                },
            {"dt",                     P_TYPE(option->dt)                     },
            {"rng_seed",               P_TYPE(option->rng_seed)               },
            {"width",                  P_TYPE(option->width)                  },
            {"height",                 P_TYPE(option->height)                 },
            {"gconst",                 P_TYPE(option->gconst)                 },
            {"verbosity",              P_TYPE(option->verbosity)              },
            {"fontsize",               P_TYPE(option->fontsize)               },
            {"step_back_buffer",       P_TYPE(option->step_back_buffer)       },
            {"def_radius",             P_TYPE(option->def_radius)             },
            {"dump_xyz",               P_TYPE(option->dump_xyz)               },
            {"dump_sshot",             P_TYPE(option->dump_sshot)             },
            {"exec_funct_freq",        P_TYPE(option->exec_funct_freq)        },
            {"default_draw_mode",      P_TYPE(option->default_draw_mode)      },
            {"lua_gc_sweep_freq",      P_TYPE(option->lua_gc_sweep_freq)      },
            {"skip_model_vec",         P_TYPE(option->skip_model_vec)         },
            {"algorithm",              P_TYPE(option->algorithm)              },
            {"spawn_funct",            P_TYPE(option->spawn_funct)            },
            {"custom_sprite_png",      P_TYPE(option->custom_sprite_png)      },
            {"timestep_funct",         P_TYPE(option->timestep_funct)         },
            {"file_template",          P_TYPE(option->xyz_temp)               },
            {"screenshot_template",    P_TYPE(option->sshot_temp)             },
            {"bgcolor",                P_TYPE(option->bgcolor)                },
            {"fontname",               P_TYPE(option->fontname)               },
            {"elements_file",          P_TYPE(option->elements_file)          },
            {"simconf_id",             P_TYPE(option->simconf_id)             },
            {"lua_expose_obj_array",   P_TYPE(option->lua_expose_obj_array)   },
            {"input_thread_enable",    P_TYPE(option->input_thread_enable)    },
            {"thread_schedule_mode",   P_TYPE(option->thread_schedule_mode)   },
            {0},
        };
        register_parser_map(opts_map, &total_opt_map);
    /*  Register parser options */

    pprint("Server doesn't work yet. Work. IN. Progress. In cinemas autumn 2015.\n");

    /* Free options */
    parser_map_free_strings(opts_map);

    return 0;
}
