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

#include "machine.h"
#include "config.h"

#include <stdbool.h>


typedef enum {
  BUFFER_OFF,                // move inactive
  BUFFER_NEW,                // initial value
  BUFFER_INIT,               // first run
  BUFFER_ACTIVE,             // subsequent runs
  BUFFER_RESTART,            // restart buffer when done
} buffer_state_t;


// Callbacks
struct mp_buffer_t;
typedef stat_t (*buffer_cb_t)(struct mp_buffer_t *bf);


typedef struct mp_buffer_t {      // See Planning Velocity Notes
  struct mp_buffer_t *prev;       // pointer to previous buffer
  struct mp_buffer_t *next;       // pointer to next buffer

  uint32_t ts;                    // Time stamp
  int32_t line;                   // gcode block line number
  buffer_cb_t cb;                 // callback to buffer exec function

  buffer_state_t state;           // buffer state
  bool replannable;               // true if move can be re-planned
  bool hold;                      // hold at the start of this block

  float value;                    // used in dwell and other callbacks

  float target[AXES];             // XYZABC where the move should go
  float unit[AXES];               // unit vector for axis scaling & planning

  float length;                   // total length of line or helix in mm
  float head_length;
  float body_length;
  float tail_length;

  // See notes on these variables, in mp_aline()
  float entry_velocity;           // entry velocity requested for the move
  float cruise_velocity;          // cruise velocity requested & achieved
  float exit_velocity;            // exit velocity requested for the move
  float braking_velocity;         // current value for braking velocity

  float entry_vmax;               // max junction velocity at entry of this move
  float cruise_vmax;              // max cruise velocity requested for move
  float exit_vmax;                // max exit velocity possible (redundant)
  float delta_vmax;               // max velocity difference for this move

  float jerk;                     // maximum linear jerk term for this move
  float cbrt_jerk;                // cube root of Jm (computed & cached)
} mp_buffer_t;


void mp_queue_init();

uint8_t mp_queue_get_room();
uint8_t mp_queue_get_fill();

bool mp_queue_is_empty();

mp_buffer_t *mp_queue_get_tail();
void mp_queue_push(buffer_cb_t func, uint32_t line);

mp_buffer_t *mp_queue_get_head();
void mp_queue_pop();

static inline mp_buffer_t *mp_buffer_prev(mp_buffer_t *bp) {return bp->prev;}
static inline mp_buffer_t *mp_buffer_next(mp_buffer_t *bp) {return bp->next;}
