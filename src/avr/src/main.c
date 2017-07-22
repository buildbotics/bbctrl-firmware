/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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
#include "machine.h"
#include "stepper.h"
#include "motor.h"
#include "switch.h"
#include "usart.h"
#include "drv8711.h"
#include "vars.h"
#include "rtc.h"
#include "report.h"
#include "command.h"
#include "estop.h"
#include "i2c.h"
#include "pgmspace.h"
#include "outputs.h"
#include "lcd.h"

#include "plan/planner.h"
#include "plan/arc.h"
#include "plan/state.h"

#include <avr/wdt.h>

#include <stdio.h>
#include <stdbool.h>


int main() {
  //wdt_enable(WDTO_250MS); TODO

  // Init
  cli();                          // disable interrupts

  hardware_init();                // hardware setup - must be first
  outputs_init();                 // output pins
  usart_init();                   // serial port
  i2c_init();                     // i2c port
  drv8711_init();                 // motor drivers
  stepper_init();                 // steppers
  motor_init();                   // motors
  switch_init();                  // switches
  mp_init();                      // motion planning
  machine_init();                 // gcode machine
  vars_init();                    // configuration variables
  estop_init();                   // emergency stop handler
  command_init();

  sei();                          // enable interrupts

  // Splash
  fprintf_P(stdout, PSTR("\n{\"firmware\": \"Buildbotics AVR\", "
                         "\"version\": \"" VERSION "\"}\n"));

  // Main loop
  while (true) {
    hw_reset_handler();           // handle hard reset requests
    if (!estop_triggered()) {
      mp_state_callback();
      mach_arc_callback();          // arc generation runs
    }
    command_callback();           // process next command
    report_callback();            // report changes
    wdt_reset();
  }

  return 0;
}
