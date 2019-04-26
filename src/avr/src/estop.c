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

#include "estop.h"
#include "motor.h"
#include "stepper.h"
#include "spindle.h"
#include "switch.h"
#include "hardware.h"
#include "config.h"
#include "state.h"
#include "outputs.h"
#include "jog.h"
#include "exec.h"


static stat_t estop_reason = STAT_OK;


static void _switch_callback(switch_id_t id, bool active) {
  if (active) estop_trigger(STAT_ESTOP_SWITCH);
  else estop_clear();
}


void estop_init() {
  switch_set_callback(SW_ESTOP, _switch_callback);
  if (switch_is_active(SW_ESTOP)) estop_trigger(STAT_ESTOP_SWITCH);
}


bool estop_triggered() {return estop_reason != STAT_OK;}


void estop_trigger(stat_t reason) {
  if (estop_triggered()) return;
  estop_reason = reason;

  // Set fault signal
  outputs_set_active(FAULT_PIN, true);

  // Shutdown peripherals
  st_shutdown();
  spindle_estop();
  jog_stop();
  outputs_stop();

  // Set machine state
  state_estop();
}


void estop_clear() {
  // It is important that we don't clear the estop if it's not set because
  // it can cause a reboot loop.
  if (!estop_triggered()) return;

  // Check if estop switch is set
  if (switch_is_active(SW_ESTOP)) {
    estop_reason = STAT_ESTOP_SWITCH;
    return; // Can't clear while estop switch is still active
  }

  // Clear fault signal
  outputs_set_active(FAULT_PIN, false);

  estop_reason = STAT_OK;

  // Reboot
  // Note, hardware.c waits until any spindle stop command has been delivered
  hw_request_hard_reset();
}


// Var callbacks
bool get_estop() {return estop_triggered();}


void set_estop(bool value) {
  if (value == estop_triggered()) return;
  if (value) estop_trigger(STAT_ESTOP_USER);
  else estop_clear();
}


PGM_P get_estop_reason() {return status_to_pgmstr(estop_reason);}


// Command callbacks
stat_t command_estop(char *cmd) {
  estop_trigger(STAT_ESTOP_USER);
  return STAT_OK;
}


stat_t command_clear(char *cmd) {estop_clear(); return STAT_OK;}
