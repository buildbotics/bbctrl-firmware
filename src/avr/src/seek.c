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

#include "seek.h"

#include "command.h"
#include "io.h"
#include "estop.h"
#include "util.h"
#include "state.h"
#include "exec.h"
#include "stepper.h"
#include "drv8711.h"

#include <stdint.h>


enum {
  SEEK_ACTIVE = 1 << 0,
  SEEK_ERROR  = 1 << 1,
  SEEK_FOUND  = 1 << 2,
};


typedef struct {
  bool active;
  io_function_t sw;
  uint8_t flags;
} seek_t;


static seek_t seek = {false, IO_DISABLED, 0};


static bool _is_stall_input(int sw) {
  return INPUT_STALL_0 <= sw && sw <= INPUT_STALL_3;
}


static int _input_motor(int sw) {return sw - INPUT_STALL_0;}



static void _input_cb(io_function_t sw, bool active) {
  if (_is_stall_input(sw)) drv8711_set_stalled(_input_motor(sw), active);

  if (sw != seek_get_input() && !_is_stall_input(sw) &&
      exec_get_velocity() && active)
    estop_trigger(STAT_ESTOP_SWITCH);
}


void seek_init() {
  for (int sw = INPUT_MOTOR_0_MAX; sw <= INPUT_MOTOR_3_MIN; sw++)
    io_set_callback((io_function_t)sw, _input_cb);

  for (int sw = INPUT_STALL_0; sw <= INPUT_STALL_3; sw++)
    io_set_callback((io_function_t)sw, _input_cb);
}


io_function_t seek_get_input() {return seek.active ? seek.sw : IO_DISABLED;}


bool seek_found() {
  if (!seek.active) return false;

  bool inactive = !(seek.flags & SEEK_ACTIVE);
  bool found = io_get_input(seek.sw) ^ inactive;
  bool stall_sw = _is_stall_input(seek.sw);

  if (found && (!stall_sw || drv8711_detect_stall(_input_motor(seek.sw)))) {
    seek.flags |= SEEK_FOUND;
    return true;
  }

  return false;
}


static void _done() {
  if (!seek.active) return;
  seek.active = false;

  if (_is_stall_input(seek.sw))
    drv8711_set_stall_detect(_input_motor(seek.sw), false);
}


void seek_end() {
  if (!seek.active) return;

  if (!(SEEK_FOUND & seek.flags) && (SEEK_ERROR & seek.flags) &&
      state_get() != STATE_STOPPING)
    estop_trigger(STAT_SEEK_NOT_FOUND);

  _done();
}


void seek_cancel() {_done();}


io_function_t _decode_id(int8_t id) {
  switch (id) {
  case 1: return INPUT_PROBE;
  case 2: return INPUT_MOTOR_0_MIN;
  case 3: return INPUT_MOTOR_0_MAX;
  case 4: return INPUT_MOTOR_1_MIN;
  case 5: return INPUT_MOTOR_1_MAX;
  case 6: return INPUT_MOTOR_2_MIN;
  case 7: return INPUT_MOTOR_2_MAX;
  case 8: return INPUT_MOTOR_3_MIN;
  case 9: return INPUT_MOTOR_3_MAX;
  default: return IO_DISABLED;
  }
}


// Command callbacks
stat_t command_seek(char *cmd) {
  io_function_t sw = _decode_id(decode_hex_nibble(cmd[1]));

  if (sw == IO_DISABLED) return STAT_INVALID_ARGUMENTS;
  if (!io_is_enabled(sw)) return STAT_SEEK_NOT_ENABLED;

  uint8_t flags = decode_hex_nibble(cmd[2]);
  if (flags & 0xfc) return STAT_INVALID_ARGUMENTS;

  seek_t seek = {true, sw, flags};
  command_push(*cmd, &seek);

  return STAT_OK;
}


unsigned command_seek_size() {return sizeof(seek_t);}


void command_seek_exec(void *data) {
  seek = *(seek_t *)data;

  if (_is_stall_input(seek.sw))
    drv8711_set_stall_detect(_input_motor(seek.sw), true);
}
