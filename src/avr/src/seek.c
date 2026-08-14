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

#include "seek.h"

#include "command.h"
#include "io.h"
#include "estop.h"
#include "util.h"
#include "state.h"
#include "exec.h"
#include "stepper.h"
#include "drv8711.h"
#include "motor.h"
#include "axis.h"

#include <stdint.h>
#include <math.h>


enum {
  SEEK_ACTIVE = 1 << 0,
  SEEK_ERROR  = 1 << 1,
  SEEK_FOUND  = 1 << 2,
};


typedef struct {
  bool active;
  io_function_t sw;
  uint8_t flags;
  io_function_t sw2; // Second switch, IO_DISABLED unless squaring an axis
  uint8_t found;     // One bit per switch, set as each one triggers
} seek_t;


static seek_t seek = {false, IO_DISABLED, 0, IO_DISABLED, 0};


static bool _is_stall_input(int sw) {
  return INPUT_STALL_0 <= sw && sw <= INPUT_STALL_3;
}


static int _input_motor(int sw) {return sw - INPUT_STALL_0;}


/// The motor a limit or stall input belongs to, or -1
static int _switch_motor(io_function_t sw) {
  if (INPUT_MOTOR_0_MAX <= sw && sw <= INPUT_MOTOR_3_MAX)
    return sw - INPUT_MOTOR_0_MAX;

  if (INPUT_MOTOR_0_MIN <= sw && sw <= INPUT_MOTOR_3_MIN)
    return sw - INPUT_MOTOR_0_MIN;

  if (_is_stall_input(sw)) return _input_motor(sw);

  return -1;
}


static bool _is_seek_input(io_function_t sw) {
  return seek.active && (sw == seek.sw || sw == seek.sw2);
}


static void _hold_motor(io_function_t sw, bool hold) {
  const int motor = _switch_motor(sw);
  if (0 <= motor) motor_set_homing_hold(motor, hold);
}


static void _release_holds() {
  if (seek.sw2 == IO_DISABLED) return;
  _hold_motor(seek.sw, false);
  _hold_motor(seek.sw2, false);
}


static void _input_cb(io_function_t sw, bool active) {
  if (_is_stall_input(sw)) drv8711_set_stalled(_input_motor(sw), active);

  if (!_is_seek_input(sw) && !_is_stall_input(sw) &&
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


/// True once a single switch has triggered
static bool _switch_found(io_function_t sw) {
  const bool inactive = !(seek.flags & SEEK_ACTIVE);
  if (!(io_get_input(sw) ^ inactive)) return false;

  return !_is_stall_input(sw) || drv8711_detect_stall(_input_motor(sw));
}


static void _done() {
  if (!seek.active) return;
  seek.active = false;

  _release_holds();

  if (_is_stall_input(seek.sw))
    drv8711_set_stall_detect(_input_motor(seek.sw), false);

  if (_is_stall_input(seek.sw2))
    drv8711_set_stall_detect(_input_motor(seek.sw2), false);
}


/// True when the motor already at its switch has waited too long for the other
static bool _square_error_exceeded() {
  // The limit is configured on the axis' first motor, which owns seek.sw
  const int master = _switch_motor(seek.sw);
  const int held   = (seek.found & 1) ? master : _switch_motor(seek.sw2);
  if (master < 0 || held < 0) return false;

  const float max = motor_get_square_limit(master);

  return 0 < max && max < fabs(motor_get_square_offset(held));
}


bool seek_found() {
  if (!seek.active) return false;

  // Single switch seek
  if (seek.sw2 == IO_DISABLED) {
    if (!_switch_found(seek.sw)) return false;
    seek.flags |= SEEK_FOUND;
    return true;
  }

  // Dual switch seek.  Hold each motor as its own switch triggers, letting the
  // other keep going until it reaches its switch.  That squares the axis.
  const io_function_t sws[2] = {seek.sw, seek.sw2};

  for (int i = 0; i < 2; i++) {
    const uint8_t bit = 1 << i;
    if ((seek.found & bit) || !_switch_found(sws[i])) continue;

    seek.found |= bit;
    _hold_motor(sws[i], true);
  }

  // Release before stopping so both motors decelerate together
  if (seek.found == 0x3) {
    _release_holds();
    seek.flags |= SEEK_FOUND;
    return true;
  }

  // A switch that never triggers would rack the axis until the seek runs out
  // of travel.  Give up as soon as the motors are too far apart.  Ending the
  // seek here also keeps seek_end() from overriding the hold reason.
  if (seek.found && _square_error_exceeded()) {
    _done();
    state_square_hold();
  }

  return false;
}


void seek_end() {
  if (!seek.active) return;

  if (!(SEEK_FOUND & seek.flags))
    state_seek_hold(false, SEEK_ERROR & seek.flags);

  _done();
}


void seek_cancel() {_done();}


io_function_t _decode_id(int8_t id) {
  switch (id) {
  case 1:  return INPUT_PROBE;
  case 2:  return INPUT_MOTOR_0_MIN;
  case 3:  return INPUT_MOTOR_0_MAX;
  case 4:  return INPUT_MOTOR_1_MIN;
  case 5:  return INPUT_MOTOR_1_MAX;
  case 6:  return INPUT_MOTOR_2_MIN;
  case 7:  return INPUT_MOTOR_2_MAX;
  case 8:  return INPUT_MOTOR_3_MIN;
  case 9:  return INPUT_MOTOR_3_MAX;
  case 10: return INPUT_STALL_0;
  case 11: return INPUT_STALL_1;
  case 12: return INPUT_STALL_2;
  case 13: return INPUT_STALL_3;
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

  // An optional second switch homes a second motor on the same axis.  Each
  // motor stops at its own switch, squaring the axis.
  io_function_t sw2 = IO_DISABLED;

  if (cmd[3]) {
    sw2 = _decode_id(decode_hex_nibble(cmd[3]));

    if (sw2 == IO_DISABLED || sw2 == sw) return STAT_INVALID_ARGUMENTS;
    if (!io_is_enabled(sw2)) return STAT_SEEK_NOT_ENABLED;

    // The switches must be the ones belonging to the two motors of a single
    // axis.  Holding a motor that does not share an axis with the other would
    // drive them apart with nothing to stop it.
    const int m1 = _switch_motor(sw);
    const int m2 = _switch_motor(sw2);
    if (m1 < 0 || m2 < 0) return STAT_INVALID_ARGUMENTS;

    const int axis = motor_get_axis(m1);
    if (axis_get_motor(axis) != m1 || axis_get_slave_motor(axis) != m2)
      return STAT_INVALID_ARGUMENTS;
  }

  seek_t seek = {true, sw, flags, sw2, 0};
  command_push(*cmd, &seek);

  return STAT_OK;
}


unsigned command_seek_size() {return sizeof(seek_t);}


void command_seek_exec(void *data) {
  seek = *(seek_t *)data;

  if (_is_stall_input(seek.sw))
    drv8711_set_stall_detect(_input_motor(seek.sw), true);

  if (_is_stall_input(seek.sw2))
    drv8711_set_stall_detect(_input_motor(seek.sw2), true);
}
