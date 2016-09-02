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

#include "machine.h"
#include "config.h"

#include <stdbool.h>


typedef enum {
  MOVE_TYPE_NULL,          // null move - does a no-op
  MOVE_TYPE_ALINE,         // acceleration planned line
  MOVE_TYPE_DWELL,         // delay with no movement
  MOVE_TYPE_COMMAND,       // general command
} moveType_t;


typedef enum {
  MOVE_OFF,                // move inactive (MUST BE ZERO)
  MOVE_NEW,                // initial value
  MOVE_RUN,                // general run state (for non-acceleration moves)
} moveState_t;


// All the enums that equal zero must be zero. Don't change this
typedef enum {                    // bf->buffer_state values
  MP_BUFFER_EMPTY,                // struct is available for use (MUST BE 0)
  MP_BUFFER_LOADING,              // being written ("checked out")
  MP_BUFFER_QUEUED,               // in queue
  MP_BUFFER_RUNNING               // current running buffer
} bufferState_t;


// Callbacks
typedef void (*mach_exec_t)(float[], float[]);
struct mpBuffer;
typedef stat_t (*bf_func_t)(struct mpBuffer *bf);


typedef struct mpBuffer {         // See Planning Velocity Notes
  struct mpBuffer *pv;            // pointer to previous buffer
  struct mpBuffer *nx;            // pointer to next buffer

  bf_func_t bf_func;              // callback to buffer exec function
  mach_exec_t mach_func;          // callback to machine

  bufferState_t buffer_state;     // used to manage queuing/dequeuing
  moveType_t move_type;           // used to dispatch to run routine
  moveState_t move_state;         // move state machine sequence
  bool replannable;               // true if move can be re-planned

  float unit[AXES];               // unit vector for axis scaling & planning

  float length;                   // total length of line or helix in mm
  float head_length;
  float body_length;
  float tail_length;

  // See notes on these variables, in aline()
  float entry_velocity;           // entry velocity requested for the move
  float cruise_velocity;          // cruise velocity requested & achieved
  float exit_velocity;            // exit velocity requested for the move
  float braking_velocity;         // current value for braking velocity

  float entry_vmax;               // max junction velocity at entry of this move
  float cruise_vmax;              // max cruise velocity requested for move
  float exit_vmax;                // max exit velocity possible (redundant)
  float delta_vmax;               // max velocity difference for this move

  float jerk;                     // maximum linear jerk term for this move
  float recip_jerk;               // 1/Jm used for planning (computed & cached)
  float cbrt_jerk;                // cube root of Jm (computed & cached)

  MoveState_t ms;
} mpBuf_t;


uint8_t mp_get_planner_buffer_room();
void mp_wait_for_buffer();
void mp_init_buffers();
bool mp_queue_empty();
mpBuf_t *mp_get_write_buffer();
void mp_commit_write_buffer(uint32_t line, moveType_t type);
mpBuf_t *mp_get_run_buffer();
void mp_free_run_buffer();
mpBuf_t *mp_get_last_buffer();
/// Returns pointer to prev buffer in linked list
#define mp_get_prev_buffer(b) (b->pv)
/// Returns pointer to next buffer in linked list
#define mp_get_next_buffer(b) (b->nx)
void mp_clear_buffer(mpBuf_t *bf);
void mp_copy_buffer(mpBuf_t *bf, const mpBuf_t *bp);
