/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include "state.h"
#include "machine.h"
#include "planner.h"
#include "report.h"
#include "buffer.h"

#include <stdbool.h>


typedef struct {
  plannerState_t state;
  plannerCycle_t cycle;

  bool hold_requested;
  bool flush_requested;
  bool start_requested;
} planner_state_t;


static planner_state_t ps = {0};


plannerState_t mp_get_state() {return ps.state;}
plannerCycle_t mp_get_cycle() {return ps.cycle;}


PGM_P mp_get_state_pgmstr(plannerState_t state) {
  switch (state) {
  case STATE_READY:    return PSTR("ready");
  case STATE_ESTOPPED: return PSTR("estopped");
  case STATE_RUNNING:  return PSTR("running");
  case STATE_STOPPING: return PSTR("stopping");
  case STATE_HOLDING:  return PSTR("holding");
  }

  return PSTR("invalid");
}


PGM_P mp_get_cycle_pgmstr(plannerCycle_t cycle) {
  switch (cycle) {
  case CYCLE_MACHINING:   return PSTR("machining");
  case CYCLE_HOMING:      return PSTR("homing");
  case CYCLE_PROBING:     return PSTR("probing");
  case CYCLE_CALIBRATING: return PSTR("calibrating");
  case CYCLE_JOGGING:     return PSTR("jogging");
  }

  return PSTR("invalid");
}


void mp_set_state(plannerState_t state) {
  if (ps.state == state) return; // No change
  if (ps.state == STATE_ESTOPPED) return; // Can't leave EStop state
  ps.state = state;
  report_request();
}


void mp_set_cycle(plannerCycle_t cycle) {
  if (ps.cycle == cycle) return; // No change

  if (ps.state != STATE_READY) {
    STATUS_ERROR(STAT_INTERNAL_ERROR, "Cannot transition to %S while %S",
                 mp_get_cycle_pgmstr(cycle),
                 mp_get_state_pgmstr(ps.state));
    return;
  }

  if (ps.cycle != CYCLE_MACHINING && cycle != CYCLE_MACHINING) {
    STATUS_ERROR(STAT_INTERNAL_ERROR,
                 "Cannot transition to cycle %S while in %S",
                 mp_get_cycle_pgmstr(cycle),
                 mp_get_cycle_pgmstr(ps.cycle));
    return;
  }

  ps.cycle = cycle;
  report_request();
}


void mp_state_holding() {mp_set_state(STATE_HOLDING);}


void mp_state_running() {
  if (mp_get_state() == STATE_READY) mp_set_state(STATE_RUNNING);
}


void mp_state_idle() {
  if (mp_get_state() == STATE_RUNNING) mp_set_state(STATE_READY);
}


void mp_state_estop() {mp_set_state(STATE_ESTOPPED);}


void mp_request_hold() {ps.hold_requested = true;}
void mp_request_flush() {ps.flush_requested = true;}
void mp_request_start() {ps.start_requested = true;}


/*** Feedholds, queue flushes and starts are all related. The request functions
 * set flags. The callback interprets the flags according to these rules:
 *
 *   A hold request received:
 *     - during motion is honored
 *     - during a feedhold is ignored and reset
 *     - when already stopped is ignored and reset
 *
 *   A flush request received:
 *     - during motion is ignored but not reset
 *     - during a feedhold is deferred until the feedhold enters HOLD state.
 *       I.e. until deceleration is complete.
 *     - when stopped or holding and the planner is not busy, is honored
 *
 *   A start request received:
 *     - during motion is ignored and reset
 *     - during a feedhold is deferred until the feedhold enters a HOLD state.
 *       I.e. until deceleration is complete.  If a queue flush request is also
 *       present the queue flush is done first
 *     - when stopped is honored and starts to run anything in the planner queue
 */
void mp_state_callback() {
  if (ps.hold_requested) {
    ps.hold_requested = false;

    if (mp_get_state() == STATE_RUNNING) mp_set_state(STATE_STOPPING);
  }

  // Only flush queue when we are stopped or holding
  if (ps.flush_requested &&
      (mp_get_state() == STATE_READY || mp_get_state() == STATE_HOLDING) &&
      !mp_get_runtime_busy()) {
    ps.flush_requested = false;

    mp_flush_planner();

    // NOTE: The following uses low-level mp calls for absolute position
    for (int axis = 0; axis < AXES; axis++)
      mach_set_position(axis, mp_get_runtime_absolute_position(axis));
  }

  // Don't start while stopping
  if (ps.start_requested && mp_get_state() != STATE_STOPPING) {
    ps.start_requested = false;

    if (mp_get_state() == STATE_HOLDING) {
      // Check if any moves are buffered
      if (mp_get_run_buffer()) {
        mp_replan_blocks();
        mp_set_state(STATE_RUNNING);

      } else mp_set_state(STATE_READY);
    }
  }
}
