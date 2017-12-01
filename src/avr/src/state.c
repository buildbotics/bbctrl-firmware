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

#include "exec.h"
#include "queue.h"
#include "stepper.h"
#include "spindle.h"
#include "report.h"

#include <stdbool.h>


static struct {
  state_t state;
  cycle_t cycle;
  hold_reason_t hold_reason;
  bool pause;

  bool hold_requested;
  bool flush_requested;
  bool start_requested;
  bool resume_requested;
  bool optional_pause_requested;

} s = {
  .flush_requested = true, // Start out flushing
};


PGM_P state_get_pgmstr(state_t state) {
  switch (state) {
  case STATE_READY:     return PSTR("READY");
  case STATE_ESTOPPED:  return PSTR("ESTOPPED");
  case STATE_RUNNING:   return PSTR("RUNNING");
  case STATE_STOPPING:  return PSTR("STOPPING");
  case STATE_HOLDING:   return PSTR("HOLDING");
  }

  return PSTR("INVALID");
}


PGM_P state_get_cycle_pgmstr(cycle_t cycle) {
  switch (cycle) {
  case CYCLE_MACHINING:   return PSTR("MACHINING");
  case CYCLE_HOMING:      return PSTR("HOMING");
  case CYCLE_PROBING:     return PSTR("PROBING");
  case CYCLE_JOGGING:     return PSTR("JOGGING");
  }

  return PSTR("INVALID");
}


PGM_P state_get_hold_reason_pgmstr(hold_reason_t reason) {
  switch (reason) {
  case HOLD_REASON_USER_PAUSE:    return PSTR("USER");
  case HOLD_REASON_PROGRAM_PAUSE: return PSTR("PROGRAM");
  case HOLD_REASON_PROGRAM_END:   return PSTR("END");
  case HOLD_REASON_PALLET_CHANGE: return PSTR("PALLET");
  case HOLD_REASON_TOOL_CHANGE:   return PSTR("TOOL");
  }

  return PSTR("INVALID");
}


state_t state_get() {return s.state;}
cycle_t state_get_cycle() {return s.cycle;}


static void _set_state(state_t state) {
  if (s.state == state) return; // No change
  if (s.state == STATE_ESTOPPED) return; // Can't leave EStop state
  if (state == STATE_READY) exec_set_line(0);
  s.state = state;
  report_request();
}


void state_set_cycle(cycle_t cycle) {
  if (s.cycle == cycle) return; // No change

  if (s.state != STATE_READY && cycle != CYCLE_MACHINING) {
    STATUS_ERROR(STAT_INTERNAL_ERROR, "Cannot transition to %S while %S",
                 state_get_cycle_pgmstr(cycle),
                 state_get_pgmstr(s.state));
    return;
  }

  if (s.cycle != CYCLE_MACHINING && cycle != CYCLE_MACHINING) {
    STATUS_ERROR(STAT_INTERNAL_ERROR,
                 "Cannot transition to cycle %S while in %S",
                 state_get_cycle_pgmstr(cycle),
                 state_get_cycle_pgmstr(s.cycle));
    return;
  }

  s.cycle = cycle;
  report_request();
}


void state_set_hold_reason(hold_reason_t reason) {
  if (s.hold_reason == reason) return; // No change
  s.hold_reason = reason;
  report_request();
}


bool state_is_flushing() {return s.flush_requested && !s.resume_requested;}
bool state_is_resuming() {return s.resume_requested;}


bool state_is_quiescent() {
  return (state_get() == STATE_READY || state_get() == STATE_HOLDING) &&
    !st_is_busy() && !exec_is_busy();
}


bool state_is_ready() {
  return queue_get_room() && !state_is_resuming() && !s.pause;
}


void state_pause_queue(bool x) {s.pause = x;}


void state_optional_pause() {
  if (s.optional_pause_requested) {
    state_set_hold_reason(HOLD_REASON_USER_PAUSE);
    state_holding();
  }
}


static void _set_plan_steps(bool plan_steps) {} // TODO


void state_holding() {
  _set_state(STATE_HOLDING);
  _set_plan_steps(false);
}


void state_running() {
  if (state_get() == STATE_READY) _set_state(STATE_RUNNING);
}


void state_idle() {
  if (state_get() == STATE_RUNNING) _set_state(STATE_READY);
}


void state_estop() {
  _set_state(STATE_ESTOPPED);
  state_pause_queue(false);
}


void state_request_hold() {s.hold_requested = true;}
void state_request_start() {s.start_requested = true;}
void state_request_flush() {s.flush_requested = true;}


void state_request_resume() {
  if (s.flush_requested) s.resume_requested = true;
}


void state_request_optional_pause() {s.optional_pause_requested = true;}


void state_request_step() {
  _set_plan_steps(true);
  s.start_requested = true;
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
 *     - when stopped or holding and the exec is not busy, is honored
 *
 *   A start request received:
 *     - during motion is ignored and reset
 *     - during a feedhold is deferred until the feedhold enters HOLDING state.
 *       I.e. until deceleration is complete.  If a queue flush request is also
 *       present the queue flush is done first
 *     - when stopped is honored and starts to run anything in the queue
 */
void state_callback() {
  if (s.hold_requested || s.flush_requested) {
    s.hold_requested = false;
    state_set_hold_reason(HOLD_REASON_USER_PAUSE);

    if (state_get() == STATE_RUNNING) _set_state(STATE_STOPPING);
  }

  // Only flush queue when idle or holding.
  if (s.flush_requested && state_is_quiescent()) {

    if (!queue_is_empty()) queue_flush();

    // Stop spindle
    spindle_stop();

    // Resume
    if (s.resume_requested) {
      s.flush_requested = s.resume_requested = false;
      _set_state(STATE_READY);
    }
  }

  // Don't start while flushing or stopping
  if (s.start_requested && !s.flush_requested &&
      state_get() != STATE_STOPPING) {
    s.start_requested = false;
    s.optional_pause_requested = false;

    if (state_get() == STATE_HOLDING) {
      // Check if any moves are buffered
      if (!queue_is_empty()) _set_state(STATE_RUNNING);
      else _set_state(STATE_READY);
    }
  }
}


// Var callbacks
PGM_P get_state() {return state_get_pgmstr(state_get());}
PGM_P get_cycle() {return state_get_cycle_pgmstr(state_get_cycle());}
PGM_P get_hold_reason() {return state_get_hold_reason_pgmstr(s.hold_reason);}
