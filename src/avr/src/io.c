/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

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


void io_callback() {
  if (active_cmd.port == -1) return;

  bool done = false;
  float result = 0;
  if (active_cmd.mode == INPUT_IMMEDIATE || rtc_expired(timeout)) done = true;

  // TODO handle modes

  if (done) {
    if (active_cmd.digital) { // TODO
    } else result = analog_get(active_cmd.port);

    printf_P(PSTR("{\"result\": %f}\n"), (double)result);
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
