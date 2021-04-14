/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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
#include "switch.h"
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
  switch_id_t sw;
  uint8_t flags;
} seek_t;


static seek_t seek = {false, SW_INVALID, 0};


static bool _is_stall_switch(int sw) {
  return SW_STALL_0 <= sw && sw <= SW_STALL_3;
}


static int _switch_motor(int sw) {return sw - SW_STALL_0;}



static void _switch_cb(switch_id_t sw, bool active) {
  if (_is_stall_switch(sw)) drv8711_set_stalled(_switch_motor(sw), active);

  if (sw != seek_get_switch() && !_is_stall_switch(sw) &&
      exec_get_velocity() && active)
    estop_trigger(STAT_ESTOP_SWITCH);
}


void seek_init() {
  for (int sw = SW_MIN_0; sw <= SW_STALL_3; sw++) {
    if (_is_stall_switch(sw))
      switch_set_type((switch_id_t)sw, SW_NORMALLY_OPEN);
    switch_set_callback((switch_id_t)sw, _switch_cb);
  }
}


switch_id_t seek_get_switch() {return seek.active ? seek.sw : SW_INVALID;}


bool seek_switch_found() {
  if (!seek.active) return false;

  bool inactive = !(seek.flags & SEEK_ACTIVE);
  bool found = switch_is_active(seek.sw) ^ inactive;
  bool stall_sw = _is_stall_switch(seek.sw);

  if (found && (!stall_sw || drv8711_detect_stall(_switch_motor(seek.sw)))) {
    seek.flags |= SEEK_FOUND;
    return true;
  }

  return false;
}


static void _done() {
  if (!seek.active) return;
  seek.active = false;

  if (_is_stall_switch(seek.sw))
    drv8711_set_stall_detect(_switch_motor(seek.sw), false);
}


void seek_end() {
  if (!seek.active) return;

  if (!(SEEK_FOUND & seek.flags) && (SEEK_ERROR & seek.flags) &&
      state_get() != STATE_STOPPING)
    estop_trigger(STAT_SEEK_NOT_FOUND);

  _done();
}


void seek_cancel() {_done();}


// Command callbacks
stat_t command_seek(char *cmd) {
  switch_id_t sw = (switch_id_t)decode_hex_nibble(cmd[1]);
  if (sw <= 0) return STAT_INVALID_ARGUMENTS; // Don't allow seek to ESTOP
  if (!switch_is_enabled(sw)) return STAT_SEEK_NOT_ENABLED;

  uint8_t flags = decode_hex_nibble(cmd[2]);
  if (flags & 0xfc) return STAT_INVALID_ARGUMENTS;

  seek_t seek = {true, sw, flags};
  command_push(*cmd, &seek);

  return STAT_OK;
}


unsigned command_seek_size() {return sizeof(seek_t);}


void command_seek_exec(void *data) {
  seek = *(seek_t *)data;

  if (_is_stall_switch(seek.sw))
    drv8711_set_stall_detect(_switch_motor(seek.sw), true);
}
