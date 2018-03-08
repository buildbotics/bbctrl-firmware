/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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

#include "io.h"

#include "status.h"
#include "util.h"
#include "command.h"
#include "exec.h"
#include "rtc.h"
#include "analog.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>


typedef struct {
  int8_t port;
  bool digital;
  input_mode_t mode;
  float timeout;
} input_cmd_t;


static input_cmd_t active_cmd = {-1,};
static uint32_t timeout;


void io_rtc_callback() {
  if (active_cmd.port == -1) return;

  bool done = false;
  float result = 0;
  if (active_cmd.mode == INPUT_IMMEDIATE ||
      rtc_expired(active_cmd.timeout)) done = true;

  // TODO handle modes

  if (done) {
    if (active_cmd.digital) { // TODO
    } else result = analog_get(active_cmd.port);

    // TODO find a better way to send this
    printf("{\"result\": %f}\n", (double)result);
    active_cmd.port = -1;
  }
}


static stat_t _exec_cb() {
  if (active_cmd.port == -1) exec_set_cb(0);
  return STAT_NOP;
}


// Command callbacks
stat_t command_input(char *cmd) {
  input_cmd_t input_cmd;
  cmd++; // Skip command

  // Analog or digital
  if (*cmd == 'd') input_cmd.digital = true;
  else if (*cmd == 'a') input_cmd.digital = false;
  else return STAT_INVALID_ARGUMENTS;
  cmd++;

  // Port index
  if (!isdigit(*cmd)) return STAT_INVALID_ARGUMENTS;
  input_cmd.port = *cmd - '0';
  cmd++;

  // Mode
  if (!isdigit(*cmd)) return STAT_INVALID_ARGUMENTS;
  input_cmd.mode = (input_mode_t)(*cmd - '0');
  if (INPUT_LOW < input_cmd.mode) return STAT_INVALID_ARGUMENTS;
  cmd++;

  // Timeout
  if (!decode_float(&cmd, &input_cmd.timeout)) return STAT_BAD_FLOAT;

  command_push(COMMAND_input, &input_cmd);

  return STAT_OK;
}


unsigned command_input_size() {return sizeof(input_cmd_t);}


void command_input_exec(void *data) {
  active_cmd = *(input_cmd_t *)data;

  timeout = rtc_get_time() + active_cmd.timeout * 1000;
  exec_set_cb(_exec_cb);
}
