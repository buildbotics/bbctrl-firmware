/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                            All rights reserved.

     This file ("the software") is free software: you can redistribute it
     and/or modify it under the terms of the GNU General Public License,
      version 2 as published by the Free Software Foundation. You should
      have received a copy of the GNU General Public License, version 2
     along with the software. If not, see <http://www.gnu.org/licenses/>.

     The software is distributed in the hope that it will be useful, but
          WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
               Lesser General Public License for more details.

       You should have received a copy of the GNU Lesser General Public
                License along with the software.  If not, see
                       <http://www.gnu.org/licenses/>.

                For information regarding this software email:
                  "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#pragma once

#include "config.h"
#include "status.h"

#include <stdbool.h>


// Commands
typedef enum {
#define CMD(CODE, NAME, ...) COMMAND_##NAME = CODE,
#include "command.def"
#undef CMD
} command_t;


void command_init();
bool command_is_active();
unsigned command_get_count();
void command_print_json();
void command_flush_queue();
void command_push(char code, void *data);
bool command_callback();
void command_set_position(const float position[AXES]);
void command_get_position(float position[AXES]);
void command_reset_position();
bool command_exec();
