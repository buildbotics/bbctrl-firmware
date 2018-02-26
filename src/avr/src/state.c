/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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
#include "command.h"
#include "stepper.h"
#include "spindle.h"
#include "outputs.h"
#include "jog.h"
#include "estop.h"
#include "report.h"

#include <stdio.h>


static struct {
  state_t state;
  hold_reason_t hold_reason;

  bool stop_requested;
  bool pause_requested;
  bool optional_pause_requested;
  bool unpause_requested;
  bool flush_requested;
  bool resume_requested;

} s = {
  .flush_requested = true, // Start out flushing
};


PGM_P state_get_pgmstr(state_t state) {
  switch (state) {
  case STATE_READY:     return PSTR("READY");
  case STATE_ESTOPPED:  return PSTR("ESTOPPED");
  case STATE_RUNNING:   return PSTR("RUNNING");
  case STATE_JOGGING:   return PSTR("JOGGING");
  case STATE_STOPPING:  return PSTR("STOPPING");
  case STATE_HOLDING:   return PSTR("HOLDING");
  }

  return PSTR("INVALID");
}


PGM_P state_get_hold_reason_pgmstr(hold_reason_t reason) {
  switch (reason) {
  case HOLD_REASON_USER_PAUSE:    return PSTR("User paused");
  case HOLD_REASON_USER_STOP:     return PSTR("User stop");
  case HOLD_REASON_PROGRAM_PAUSE: return PSTR("Program paused");
  case HOLD_REASON_SWITCH_FOUND:  return PSTR("Switch found");
  }

  return PSTR("INVALID");
}


state_t state_get() {return s.state;}


static void _set_state(state_t state) {
  if (s.state == state) return; // No change
  if (s.state == STATE_ESTOPPED) return; // Can't leave EStop state
  s.state = state;
  report_request();
}


static void _set_hold_reason(hold_reason_t reason) {
  if (s.hold_reason == reason) return; // No change
  s.hold_reason = reason;
  report_request();
}


bool state_is_flushing() {return s.flush_requested && !s.resume_requested;}
bool state_is_resuming() {return s.resume_requested;}


bool state_is_quiescent() {
  return (state_get() == STATE_READY || state_get() == STATE_HOLDING) &&
    !st_is_busy();
}


void state_seek_hold() {
  if (state_get() == STATE_RUNNING) {
    _set_hold_reason(HOLD_REASON_SWITCH_FOUND);
    _set_state(STATE_STOPPING);
  }
}


static void _stop() {
  switch (state_get()) {
  case STATE_STOPPING:
    if (!exec_get_velocity()) {
      _set_state(STATE_READY);
      _stop();
      return;
    }

  case STATE_RUNNING:
    _set_hold_reason(HOLD_REASON_USER_STOP);
    _set_state(STATE_STOPPING);
    break;

  case STATE_JOGGING:
    jog_stop();
    // Fall through

  case STATE_READY:
  case STATE_HOLDING:
    s.flush_requested = true;
    spindle_stop();
    outputs_stop();
    _set_state(STATE_READY);
    break;

  case STATE_ESTOPPED:
    break; // Ignore
  }
}


void state_holding() {
  _set_state(STATE_HOLDING);

  switch (s.hold_reason) {
  case HOLD_REASON_PROGRAM_PAUSE: break;

  case HOLD_REASON_USER_PAUSE:
  case HOLD_REASON_SWITCH_FOUND:
    s.flush_requested = true;
    break;

  case HOLD_REASON_USER_STOP:
    _stop();
    break;
  }
}


void state_pause(bool optional) {
  if (!optional || s.optional_pause_requested) {
    _set_hold_reason(HOLD_REASON_PROGRAM_PAUSE);
    state_holding();
  }
}


void state_running() {
  if (state_get() == STATE_READY) _set_state(STATE_RUNNING);
}


void state_jogging() {
  if (state_get() == STATE_READY) _set_state(STATE_JOGGING);
}


void state_idle() {
  if (state_get() == STATE_RUNNING || state_get() == STATE_JOGGING)
    _set_state(STATE_READY);
}


void state_estop() {_set_state(STATE_ESTOPPED);}


/*** Pauses, queue flushes and starts are all related.  Request functions
 * set flags.  The callback interprets the flags according to these rules:
 *
 *   A pause request received:
 *     - during motion is honored
 *     - during a pause is ignored and reset
 *     - when already stopped is ignored and reset
 *
 *   A flush request received:
 *     - during motion is ignored but not reset
 *     - during a hold is deferred until HOLDING state is entered.
 *       I.e. until deceleration is complete.
 *     - when stopped or holding and the exec is not busy, is honored
 *
 *   A start request received:
 *     - during motion is ignored and reset
 *     - during a hold is deferred until HOLDING state is entered.
 *       I.e. until deceleration is complete.  If a queue flush request is also
 *       present the queue flush is done first
 *     - when stopped is honored and starts to run anything in the queue
 */
void state_callback() {
  if (estop_triggered()) return;

  // Pause
  if (s.pause_requested || s.flush_requested) {
    if (state_get() == STATE_RUNNING) {
      if (s.pause_requested) _set_hold_reason(HOLD_REASON_USER_PAUSE);
      _set_state(STATE_STOPPING);
    }

    s.pause_requested = false;
  }

  // Stop
  if (s.stop_requested) {
    _stop();
    s.stop_requested = false;
  }

  // Only flush queue when idle or holding
  if (s.flush_requested && state_is_quiescent()) {
    command_flush_queue();

    // Resume
    if (s.resume_requested) {
      s.flush_requested = s.resume_requested = false;
      _set_state(STATE_READY);
    }
  }

  // Don't start while flushing or stopping
  if (s.unpause_requested && !s.flush_requested &&
      state_get() != STATE_STOPPING) {
    s.unpause_requested = false;
    s.optional_pause_requested = false;

    if (state_get() == STATE_HOLDING) {
      // Check if any moves are buffered
      if (command_get_count()) _set_state(STATE_RUNNING);
      else _set_state(STATE_READY);
    }
  }
}


// Var callbacks
PGM_P get_state() {return state_get_pgmstr(state_get());}
PGM_P get_hold_reason() {return state_get_hold_reason_pgmstr(s.hold_reason);}


// Command callbacks
stat_t command_pause(char *cmd) {
  pause_t type = (pause_t)(cmd[1] - '0');

  switch (type) {
  case PAUSE_USER: s.pause_requested = true; break;
  case PAUSE_OPTIONAL: s.optional_pause_requested = true; break;
  default: command_push(cmd[0], &type); break;
  }

  return STAT_OK;
}


unsigned command_pause_size() {return sizeof(pause_t);}


void command_pause_exec(void *data) {
  switch (*(pause_t *)data) {
  case PAUSE_PROGRAM: state_pause(false); break;
  case PAUSE_PROGRAM_OPTIONAL: state_pause(true); break;
  default: break;
  }
}


stat_t command_stop(char *cmd) {
  s.stop_requested = true;
  return STAT_OK;
}


stat_t command_unpause(char *cmd) {
  s.unpause_requested = true;
  return STAT_OK;
}


stat_t command_resume(char *cmd) {
  if (s.flush_requested) s.resume_requested = true;
  return STAT_OK;
}


stat_t command_flush(char *cmd) {
  s.flush_requested = true;
  return STAT_OK;
}
