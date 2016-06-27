/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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

#include "hardware.h"
#include "canonical_machine.h"
#include "stepper.h"
#include "motor.h"
#include "switch.h"
#include "usart.h"
#include "tmc2660.h"
#include "vars.h"
#include "rtc.h"
#include "report.h"
#include "command.h"
#include "estop.h"

#include "plan/planner.h"
#include "plan/buffer.h"
#include "plan/arc.h"
#include "plan/feedhold.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <stdio.h>
#include <stdbool.h>


static bool _dispatch(stat_t (*func)()) {
  stat_t err = func();

  switch (err) {
  case STAT_EAGAIN: return true;
  case STAT_OK: case STAT_NOOP: break;
  default: status_error(err); break;
  }

  return false;
}


/* Main loop
 *
 * The task order is very important. Tasks are ordered by increasing dependency
 * (blocking hierarchy).  Tasks that are dependent on completion of lower-level
 * tasks must be later in the list.  Tasks are called repeatedly even if they
 * are not currently active.
 *
 * The DISPATCH macro calls a function and returns if not finished
 * (STAT_EAGAIN).  This prevents later routines from running.  Any other
 * condition - OK or ERR - drops through and runs the next routine in the list.
 */
static void _run() {
#define DISPATCH(func) if (_dispatch(func)) return;

  DISPATCH(hw_reset_handler);                // handle hard reset requests

  DISPATCH(command_hi);

  DISPATCH(tmc2660_sync);                    // synchronize driver config
  DISPATCH(motor_power_callback);            // stepper motor power sequencing
  DISPATCH(cm_feedhold_sequencing_callback); // feedhold state machine
  DISPATCH(mp_plan_hold_callback);           // plan a feedhold
  DISPATCH(cm_arc_callback);                 // arc generation runs
  DISPATCH(cm_homing_callback);              // G28.2 continuation
  DISPATCH(cm_probe_callback);               // G38.2 continuation
  DISPATCH(report_callback);                 // report changes

  DISPATCH(command_lo);
}


static void _init() {
  cli(); // disable interrupts

  hardware_init();                // hardware setup - must be first
  usart_init();                   // serial port
  tmc2660_init();                 // motor drivers
  stepper_init();                 // steppers
  motor_init();                   // motors
  switch_init();                  // switches
  planner_init();                 // motion planning
  canonical_machine_init();       // gcode machine
  vars_init();                    // configuration variables
  estop_init();                   // emergency stop handler

  sei(); // enable interrupts

  fprintf_P(stderr, PSTR("\n{\"firmware\": \"Buildbotics AVR\", "
                         "\"version\": \"" VERSION "\"}\n"));
}


int main() {
  //wdt_enable(WDTO_250MS);

  _init();

  // main loop
  while (true) {
    _run(); // single pass
    wdt_reset();
  }

  return 0;
}
