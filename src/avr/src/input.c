/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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

#include "input.h"

#include "status.h"
#include "util.h"
#include "command.h"
#include "exec.h"
#include "rtc.h"
#include "io.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>


typedef struct {
  io_function_t function;
  bool digital;
  input_mode_t mode;
  float timeout;
  bool callback_set;
  io_callback_t old_cb;
  float result;
  bool done;
} input_cmd_t;


static input_cmd_t active_cmd = {IO_DISABLED,};
static uint32_t timeout;


static void _io_callback(io_function_t function, bool active) {
  if (active_cmd.old_cb) active_cmd.old_cb(function, active);

  if ((active_cmd.mode == INPUT_RISE && active) ||
      (active_cmd.mode == INPUT_FALL && !active)) {
    active_cmd.callback_set = false;
    io_set_callback(active_cmd.function, active_cmd.old_cb);
    active_cmd.result = active;
    active_cmd.done = true;
  }
}


void input_callback() {
  // Called from main.c
  if (active_cmd.function == IO_DISABLED) return;

  if (!active_cmd.done) {
    if (!active_cmd.digital || rtc_expired(timeout)) active_cmd.done = true;
    else {
      // Handle digital modes: immediate, rise, fall, high, low
      active_cmd.result = io_get_input(active_cmd.function);

      switch (active_cmd.mode) {
      case INPUT_IMMEDIATE: active_cmd.done = true; break;

      case INPUT_RISE: case INPUT_FALL:
        if (!active_cmd.callback_set) {
          active_cmd.old_cb = io_get_callback(active_cmd.function);
          io_set_callback(active_cmd.function, _io_callback);
          active_cmd.callback_set = true;
        }
        break;

      case INPUT_HIGH: active_cmd.done =  active_cmd.result; break;
      case INPUT_LOW:  active_cmd.done = !active_cmd.result; break;
      }
    }

    if (active_cmd.done) {
      if (active_cmd.digital)
        active_cmd.result = io_get_input(active_cmd.function);
      else active_cmd.result = io_get_analog(active_cmd.function);
    }
  }

  if (active_cmd.done) {
    printf_P(PSTR("{\"result\": %f}\n"), active_cmd.result);

    active_cmd.function = IO_DISABLED;

    if (active_cmd.callback_set) {
      active_cmd.callback_set = false;
      io_set_callback(active_cmd.function, active_cmd.old_cb);
    }
  }
}


static stat_t _exec_cb() {
  if (active_cmd.function == IO_DISABLED) exec_set_cb(0);
  return STAT_NOP;
}


// Command callbacks
stat_t command_input(char *cmd) {
  input_cmd_t input_cmd = {IO_DISABLED, };
  cmd++; // Skip command

  // Analog or digital
  if (*cmd == 'd') input_cmd.digital = true;
  else if (*cmd == 'a') input_cmd.digital = false;
  else return STAT_INVALID_ARGUMENTS;
  cmd++;

  // Port index
  if (!isdigit(*cmd)) return STAT_INVALID_ARGUMENTS;
  input_cmd.function = io_get_port_function(input_cmd.digital, *cmd - '0');
  cmd++;

  // Mode
  if (!isdigit(*cmd)) return STAT_INVALID_ARGUMENTS;
  input_cmd.mode = (input_mode_t)(*cmd - '0');
  if (INPUT_LOW < input_cmd.mode) return STAT_INVALID_ARGUMENTS;
  if (!input_cmd.digital && input_cmd.mode) return STAT_INVALID_ARGUMENTS;
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
