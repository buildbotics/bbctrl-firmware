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
#include "config.h"

#include "plan/planner.h"


typedef struct {
  bool triggered;
} estop_t;


static estop_t estop = {0};


void estop_init() {
}


bool estop_triggered() {
  return estop.triggered;
}


void estop_trigger() {
  estop.triggered = true;

  // Hard stop the motors and the spindle
  st_shutdown();
  cm_spindle_control(SPINDLE_OFF);

  // Stop and flush motion
  cm_request_feedhold();
  cm_request_queue_flush();

  // Set alarm state
  cm_set_machine_state(MACHINE_ALARM);
}


void estop_clear() {
  estop.triggered = false;

  // Clear motor errors
  for (int motor = 0; motor < MOTORS; motor++)
    motor_reset(motor);

  // Clear alarm state
  cm_set_machine_state(MACHINE_READY);
}


bool get_estop() {
  return estop.triggered;
}


void set_estop(bool value) {
  if (estop.triggered != value) {
    if (value) estop_trigger();
    else estop_clear();
  }
}
