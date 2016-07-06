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
#include "config.h"

#include "plan/planner.h"


typedef struct {
  bool triggered;
} estop_t;


static estop_t estop = {0};


static void _switch_callback(switch_id_t id, bool active) {
  if (active) estop_trigger();
  else estop_clear();

  report_request();
}


void estop_init() {
  switch_set_callback(SW_ESTOP, _switch_callback);

  OUTCLR_PIN(FAULT_PIN); // Low
  DIRSET_PIN(FAULT_PIN); // Output
}


bool estop_triggered() {
  return estop.triggered || switch_is_active(SW_ESTOP);
}


void estop_trigger() {
  estop.triggered = true;

  // Hard stop the motors and the spindle
  st_shutdown();
  mach_spindle_control(SPINDLE_OFF);

  // Stop and flush motion
  mach_request_feedhold();
  mach_request_queue_flush();

  // Set alarm state
  mach_set_machine_state(MACHINE_ALARM);

  // Assert fault signal
  OUTSET_PIN(FAULT_PIN); // High
}


void estop_clear() {
  estop.triggered = false;

  // Clear motor errors
  for (int motor = 0; motor < MOTORS; motor++)
    motor_reset(motor);

  // Clear alarm state
  mach_set_machine_state(MACHINE_READY);

  // Clear fault signal
  OUTCLR_PIN(FAULT_PIN); // Low
}


bool get_estop() {
  return estop_triggered();
}


void set_estop(bool value) {
  bool triggered = estop_triggered();
  estop.triggered = value;

  if (triggered != estop_triggered()) {
    if (value) estop_trigger();
    else estop_clear();
  }
}
