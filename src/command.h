/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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


#include "status.h"

#include <stdint.h>


#define MAX_ARGS 16

typedef uint8_t (*command_cb_t)(int argc, char *argv[]);

typedef struct {
  const char *name;
  command_cb_t cb;
  uint8_t minArgs;
  uint8_t maxArgs;
  const char *help;
} command_t;


stat_t command_dispatch();
int command_find(const char *name);
int command_exec(int argc, char *argv[]);
int command_eval(char *cmd);
