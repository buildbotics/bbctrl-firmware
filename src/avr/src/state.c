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
#include "seek.h"
#include "report.h"

#include <stdio.h>


static struct {
  state_t state;
  hold_reason_t hold_reason;

  bool stop_requested;
  bool pause_requested;
  bool optional_pause_requested;
  bool unpause_requested;
  bool flushing;
  bool resuming;

} s = {
  .flushing = true, // Start out flushing
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
  case HOLD_REASON_USER_PAUSE:    return PSTR("User pause");
  case HOLD_REASON_USER_STOP:     return PSTR("User stop");
  case HOLD_REASON_PROGRAM_PAUSE: return PSTR("Program pause");
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


bool state_is_flushing() {return s.flushing && !s.resuming;}
bool state_is_resuming() {return s.resuming;}


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
  _set_hold_reason(HOLD_REASON_USER_STOP);

  switch (state_get()) {
  case STATE_RUNNING:
    _set_state(STATE_STOPPING);
    break;

  case STATE_JOGGING:
  case STATE_READY:
  case STATE_HOLDING:
    jog_stop();
    spindle_stop();
    outputs_stop();
    seek_cancel();
    break;

  case STATE_STOPPING:
  case STATE_ESTOPPED:
    break; // Ignore
  }
}


void state_holding() {
  _set_state(STATE_HOLDING);
  if (s.hold_reason == HOLD_REASON_USER_STOP) _stop();
}


void state_pause() {
  _set_hold_reason(HOLD_REASON_PROGRAM_PAUSE);
  state_holding();
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


void state_callback() {
  if (estop_triggered()) return;

  // Pause
  if (s.pause_requested || s.flushing) {
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
  if (s.flushing && state_is_quiescent()) {
    command_flush_queue();

    // Resume
    if (s.resuming) {
      s.flushing = s.resuming = false;
      _set_state(STATE_READY);
    }
  }

  // Unpause
  if (s.unpause_requested && !s.flushing && state_get() != STATE_STOPPING) {
    s.unpause_requested = false;

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
bool get_optional_pause() {return s.optional_pause_requested;}
void set_optional_pause(bool x) {s.optional_pause_requested = x;}


// Command callbacks
stat_t command_pause(char *cmd) {
  pause_t type = (pause_t)(cmd[1] - '0');

  if (type == PAUSE_USER) s.pause_requested = true;
  else command_push(cmd[0], &type);

  return STAT_OK;
}


unsigned command_pause_size() {return sizeof(pause_t);}


void command_pause_exec(void *data) {
  switch (*(pause_t *)data) {
  case PAUSE_PROGRAM_OPTIONAL:
    if (!s.optional_pause_requested) return;
    s.optional_pause_requested = false;
    // Fall through

  case PAUSE_PROGRAM: state_pause(); break;
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
  if (s.flushing) s.resuming = true;
  return STAT_OK;
}


stat_t command_flush(char *cmd) {
  s.flushing = true;
  return STAT_OK;
}
