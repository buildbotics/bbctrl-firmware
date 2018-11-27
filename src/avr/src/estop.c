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

#include <avr/eeprom.h>


typedef struct {
  bool triggered;
} estop_t;


static estop_t estop = {false};

static uint16_t estop_reason_eeprom EEMEM;


static void _set_reason(stat_t reason) {
  eeprom_update_word(&estop_reason_eeprom, reason);
}


static stat_t _get_reason() {
  return (stat_t)eeprom_read_word(&estop_reason_eeprom);
}


static void _switch_callback(switch_id_t id, bool active) {
  if (active) estop_trigger(STAT_ESTOP_SWITCH);
  else estop_clear();
}


void estop_init() {
  if (switch_is_active(SW_ESTOP)) _set_reason(STAT_ESTOP_SWITCH);
  if (STAT_MAX <= _get_reason()) _set_reason(STAT_OK);
  estop.triggered = _get_reason() != STAT_OK;

  switch_set_callback(SW_ESTOP, _switch_callback);

  if (estop.triggered) state_estop();

  // Fault signal
  outputs_set_active(FAULT_PIN, estop.triggered);
}


bool estop_triggered() {return estop.triggered || switch_is_active(SW_ESTOP);}


void estop_trigger(stat_t reason) {
  if (estop.triggered) return;
  estop.triggered = true;

  // Set fault signal
  outputs_set_active(FAULT_PIN, true);

  // Hard stop peripherals
  st_shutdown();
  spindle_estop();
  jog_stop();
  outputs_stop();

  // Set machine state
  state_estop();

  // Save reason
  _set_reason(reason);
}


void estop_clear() {
  // It is important that we don't clear the estop if it's not set because
  // it can cause a reboot loop.
  if (!estop.triggered) return;

  // Check if estop switch is set
  if (switch_is_active(SW_ESTOP)) {
    if (_get_reason() != STAT_ESTOP_SWITCH) _set_reason(STAT_ESTOP_SWITCH);
    return; // Can't clear while estop switch is still active
  }

  // Clear fault signal
  outputs_set_active(FAULT_PIN, false);

  estop.triggered = false;

  // Clear reason
  _set_reason(STAT_OK);

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


PGM_P get_estop_reason() {return status_to_pgmstr(_get_reason());}


// Command callbacks
stat_t command_estop(char *cmd) {
  estop_trigger(STAT_ESTOP_USER);
  return STAT_OK;
}


stat_t command_clear(char *cmd) {estop_clear(); return STAT_OK;}
