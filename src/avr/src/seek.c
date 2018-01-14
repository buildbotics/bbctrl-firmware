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

#include "seek.h"

#include "command.h"
#include "switch.h"
#include "estop.h"
#include "util.h"

#include <stdint.h>


enum {
  SEEK_ACTIVE = 1 << 0,
  SEEK_ERROR  = 1 << 1,
  SEEK_FOUND  = 1 << 2,
};


typedef struct {
  int8_t sw;
  uint8_t flags;
} seek_t;


static seek_t seek = {-1, 0};


bool seek_switch_found() {
  if (seek.sw <= 0) return false;

  bool inactive = !(seek.flags & SEEK_ACTIVE);

  if (switch_is_active(seek.sw) ^ inactive) {
    seek.flags |= SEEK_FOUND;
    return true;
  }

  return false;
}


void seek_end() {
  if (seek.sw <= 0) return;

  if (!(SEEK_FOUND & seek.flags) && (SEEK_ERROR & seek.flags))
    estop_trigger(STAT_SEEK_NOT_FOUND);

  seek.sw = -1;
}


// Command callbacks
stat_t command_seek(char *cmd) {
  int8_t sw = decode_hex_nibble(cmd[1]);
  if (sw <= 0) return STAT_INVALID_ARGUMENTS; // Don't allow seek to ESTOP
  if (!switch_is_enabled(sw)) return STAT_SEEK_NOT_ENABLED;

  int8_t flags = decode_hex_nibble(cmd[2]);
  if (flags & 0xfc) return STAT_INVALID_ARGUMENTS;

  seek_t seek = {sw, flags};
  command_push(*cmd, &seek);

  return STAT_OK;
}


unsigned command_seek_size() {return sizeof(seek_t);}


void command_seek_exec(void *data) {seek = *(seek_t *)data;}
