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

#include "estop.h"
#include "motor.h"
#include "stepper.h"
#include "spindle.h"
#include "switch.h"
#include "report.h"
#include "hardware.h"
#include "homing.h"
#include "config.h"

#include "plan/planner.h"

#include <avr/eeprom.h>


typedef struct {
  bool triggered;
} estop_t;


static estop_t estop = {0};

static uint16_t estop_reason_eeprom EEMEM;


static void _set_reason(estop_reason_t reason) {
  eeprom_update_word(&estop_reason_eeprom, reason);
}


static estop_reason_t _get_reason() {
  return eeprom_read_word(&estop_reason_eeprom);
}


static void _switch_callback(switch_id_t id, bool active) {
  if (active) estop_trigger(ESTOP_SWITCH);
  else estop_clear();
}


void estop_init() {
  if (switch_is_active(SW_ESTOP)) _set_reason(ESTOP_SWITCH);
  if (ESTOP_MAX <= _get_reason()) _set_reason(ESTOP_NONE);
  estop.triggered = _get_reason() != ESTOP_NONE;

  switch_set_callback(SW_ESTOP, _switch_callback);

  if (estop.triggered) mach_set_state(STATE_ESTOP);

  // Fault signal
  if (estop.triggered) OUTSET_PIN(FAULT_PIN); // High
  else OUTCLR_PIN(FAULT_PIN); // Low
  DIRSET_PIN(FAULT_PIN); // Output
}


bool estop_triggered() {
  return estop.triggered || switch_is_active(SW_ESTOP);
}


void estop_trigger(estop_reason_t reason) {
  estop.triggered = true;

  // Hard stop the motors and the spindle
  st_shutdown();
  mach_spindle_estop();

  // Set machine state
  mach_set_state(STATE_ESTOP);

  // Set axes not homed
  mach_set_not_homed();

  // Save reason
  _set_reason(reason);

  report_request();
}


void estop_clear() {
  // Check if estop switch is set
  if (switch_is_active(SW_ESTOP)) {
    if (_get_reason() != ESTOP_SWITCH) _set_reason(ESTOP_SWITCH);
    return; // Can't clear while estop switch is still active
  }

  // Clear fault signal
  OUTCLR_PIN(FAULT_PIN); // Low

  estop.triggered = false;

  // Clear reason
  _set_reason(ESTOP_NONE);

  // Reboot
  // Note, hardware.c waits until any spindle stop command has been delivered
  hw_request_hard_reset();
}


bool get_estop() {
  return estop_triggered();
}


void set_estop(bool value) {
  if (value == estop_triggered()) return;
  if (value) estop_trigger(ESTOP_USER);
  else estop_clear();
}


PGM_P get_estop_reason() {
  switch (_get_reason()) {
  case ESTOP_NONE:   return PSTR("NONE");
  case ESTOP_USER:   return PSTR("USER");
  case ESTOP_SWITCH: return PSTR("SWITCH");
  case ESTOP_LIMIT:  return PSTR("LIMIT");
  case ESTOP_ALARM:  return PSTR("ALARM");
  default: return PSTR("INVALID");
  }
}
