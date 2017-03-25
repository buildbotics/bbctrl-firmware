/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                  Copyright (c) 2013 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2013 - 2015 Robert Giseburt
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

#include "pgmspace.h"

#include <stdbool.h>


typedef enum {
  STATE_READY,
  STATE_ESTOPPED,
  STATE_RUNNING,
  STATE_STOPPING,
  STATE_HOLDING,
} mp_state_t;


typedef enum {
  CYCLE_MACHINING,
  CYCLE_HOMING,
  CYCLE_PROBING,
  CYCLE_CALIBRATING,
  CYCLE_JOGGING,
} mp_cycle_t;


typedef enum {
  HOLD_REASON_USER_PAUSE,
  HOLD_REASON_PROGRAM_PAUSE,
  HOLD_REASON_PROGRAM_END,
  HOLD_REASON_PALLET_CHANGE,
  HOLD_REASON_TOOL_CHANGE,
} mp_hold_reason_t;


PGM_P mp_get_state_pgmstr(mp_state_t state);
PGM_P mp_get_cycle_pgmstr(mp_cycle_t cycle);
PGM_P mp_get_hold_reason_pgmstr(mp_hold_reason_t reason);

mp_state_t mp_get_state();

mp_cycle_t mp_get_cycle();
void mp_set_cycle(mp_cycle_t cycle);

mp_hold_reason_t mp_get_hold_reason();
void mp_set_hold_reason(mp_hold_reason_t reason);

bool mp_is_flushing();
bool mp_is_resuming();
bool mp_is_quiescent();

void mp_state_optional_pause();
void mp_state_holding();
void mp_state_running();
void mp_state_idle();
void mp_state_estop();

void mp_request_hold();
void mp_request_start();
void mp_request_flush();
void mp_request_resume();
void mp_request_optional_pause();
void mp_request_step();

void mp_state_callback();
