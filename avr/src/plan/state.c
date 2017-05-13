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

#include "state.h"
#include "machine.h"
#include "planner.h"
#include "runtime.h"
#include "buffer.h"
#include "arc.h"
#include "stepper.h"
#include "spindle.h"

#include "report.h"

#include <stdbool.h>


typedef struct {
  mp_state_t state;
  mp_cycle_t cycle;
  mp_hold_reason_t hold_reason;

  bool hold_requested;
  bool flush_requested;
  bool start_requested;
  bool resume_requested;
  bool optional_pause_requested;
} planner_state_t;


static planner_state_t ps = {
  .flush_requested = true, // Start out flushing
};


PGM_P mp_get_state_pgmstr(mp_state_t state) {
  switch (state) {
  case STATE_READY:     return PSTR("READY");
  case STATE_ESTOPPED:  return PSTR("ESTOPPED");
  case STATE_RUNNING:   return PSTR("RUNNING");
  case STATE_STOPPING:  return PSTR("STOPPING");
  case STATE_HOLDING:   return PSTR("HOLDING");
  }

  return PSTR("INVALID");
}


PGM_P mp_get_cycle_pgmstr(mp_cycle_t cycle) {
  switch (cycle) {
  case CYCLE_MACHINING:   return PSTR("MACHINING");
  case CYCLE_HOMING:      return PSTR("HOMING");
  case CYCLE_PROBING:     return PSTR("PROBING");
  case CYCLE_CALIBRATING: return PSTR("CALIBRATING");
  case CYCLE_JOGGING:     return PSTR("JOGGING");
  }

  return PSTR("INVALID");
}


PGM_P mp_get_hold_reason_pgmstr(mp_hold_reason_t reason) {
  switch (reason) {
  case HOLD_REASON_USER_PAUSE:    return PSTR("USER");
  case HOLD_REASON_PROGRAM_PAUSE: return PSTR("PROGRAM");
  case HOLD_REASON_PROGRAM_END:   return PSTR("END");
  case HOLD_REASON_PALLET_CHANGE: return PSTR("PALLET");
  case HOLD_REASON_TOOL_CHANGE:   return PSTR("TOOL");
  }

  return PSTR("INVALID");
}


mp_state_t mp_get_state() {return ps.state;}
mp_cycle_t mp_get_cycle() {return ps.cycle;}


static void _set_state(mp_state_t state) {
  if (ps.state == state) return; // No change
  if (ps.state == STATE_ESTOPPED) return; // Can't leave EStop state
  if (state == STATE_READY) mp_runtime_set_line(0);
  ps.state = state;
  report_request();
}


void mp_set_cycle(mp_cycle_t cycle) {
  if (ps.cycle == cycle) return; // No change

  if (ps.state != STATE_READY && cycle != CYCLE_MACHINING) {
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


mp_hold_reason_t mp_get_hold_reason() {return ps.hold_reason;}


void mp_set_hold_reason(mp_hold_reason_t reason) {
  if (ps.hold_reason == reason) return; // No change
  ps.hold_reason = reason;
  report_request();
}


bool mp_is_flushing() {return ps.flush_requested && !ps.resume_requested;}
bool mp_is_resuming() {return ps.resume_requested;}


bool mp_is_quiescent() {
  return (mp_get_state() == STATE_READY || mp_get_state() == STATE_HOLDING) &&
    !st_is_busy() && !mp_runtime_is_busy();
}


void mp_state_optional_pause() {
  if (ps.optional_pause_requested) {
    mp_set_hold_reason(HOLD_REASON_USER_PAUSE);
    mp_state_holding();
  }
}


void mp_state_holding() {
  _set_state(STATE_HOLDING);
  mp_set_plan_steps(false);
}


void mp_state_running() {
  if (mp_get_state() == STATE_READY) _set_state(STATE_RUNNING);
}


void mp_state_idle() {
  if (mp_get_state() == STATE_RUNNING) _set_state(STATE_READY);
}


void mp_state_estop() {_set_state(STATE_ESTOPPED);}


void mp_request_hold() {ps.hold_requested = true;}
void mp_request_start() {ps.start_requested = true;}
void mp_request_flush() {ps.flush_requested = true;}
void mp_request_resume() {if (ps.flush_requested) ps.resume_requested = true;}
void mp_request_optional_pause() {ps.optional_pause_requested = true;}


void mp_request_step() {
  mp_set_plan_steps(true);
  ps.start_requested = true;
}


/*** Feedholds, queue flushes and starts are all related.  Request functions
 * set flags.  The callback interprets the flags according to these rules:
 *
 *   A hold request received:
 *     - during motion is honored
 *     - during a feedhold is ignored and reset
 *     - when already stopped is ignored and reset
 *
 *   A flush request received:
 *     - during motion is ignored but not reset
 *     - during a feedhold is deferred until the feedhold enters HOLDING state.
 *       I.e. until deceleration is complete.
 *     - when stopped or holding and the planner is not busy, is honored
 *
 *   A start request received:
 *     - during motion is ignored and reset
 *     - during a feedhold is deferred until the feedhold enters HOLDING state.
 *       I.e. until deceleration is complete.  If a queue flush request is also
 *       present the queue flush is done first
 *     - when stopped is honored and starts to run anything in the planner queue
 */
void mp_state_callback() {
  if (ps.hold_requested || ps.flush_requested) {
    ps.hold_requested = false;
    mp_set_hold_reason(HOLD_REASON_USER_PAUSE);

    if (mp_get_state() == STATE_RUNNING) _set_state(STATE_STOPPING);
  }

  // Only flush queue when idle or holding.
  if (ps.flush_requested && mp_is_quiescent()) {
    mach_abort_arc();

    if (!mp_queue_is_empty()) {
      mp_flush_planner();

      // NOTE The following uses low-level mp calls for absolute position.
      // Reset to actual machine position.  Otherwise machine is set to the
      // position of the last queued move.
      for (int axis = 0; axis < AXES; axis++)
        mach_set_axis_position(axis, mp_runtime_get_axis_position(axis));
    }

    // Stop spindle
    spindle_stop();

    // Resume
    if (ps.resume_requested) {
      ps.flush_requested = ps.resume_requested = false;
      _set_state(STATE_READY);
    }
  }

  // Don't start while flushing or stopping
  if (ps.start_requested && !ps.flush_requested &&
      mp_get_state() != STATE_STOPPING) {
    ps.start_requested = false;
    ps.optional_pause_requested = false;

    if (mp_get_state() == STATE_HOLDING) {
      // Check if any moves are buffered
      if (!mp_queue_is_empty()) {
        // Always replan when coming out of a hold
        mp_replan_all();
        _set_state(STATE_RUNNING);

      } else _set_state(STATE_READY);
    }
  }
}
