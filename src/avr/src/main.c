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

#include "hardware.h"
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
#include "analog.h"
#include "modbus.h"
#include "io.h"
#include "exec.h"
#include "state.h"
#include "emu.h"

#include <avr/wdt.h>

#include <stdio.h>
#include <stdbool.h>


// For emu
int __argc;
char **__argv;


int main(int argc, char *argv[]) {
  __argc = argc;
  __argv = argv;

  wdt_enable(WDTO_250MS);

  // Init
  cli();                          // disable interrupts

  emu_init();                     // Init emulator
  hw_init();                      // hardware setup - must be first
  outputs_init();                 // output pins
  switch_init();                  // switches
  estop_init();                   // emergency stop handler
  analog_init();                  // analog input pins
  usart_init();                   // serial port
  i2c_init();                     // i2c port
  drv8711_init();                 // motor drivers
  stepper_init();                 // steppers
  motor_init();                   // motors
  exec_init();                    // motion exec
  vars_init();                    // configuration variables
  command_init();                 // command queue

  sei();                          // enable interrupts

  // Splash
  printf_P(PSTR("\n{\"firmware\":\"Buildbotics AVR\"}\n"));

  // Main loop
  while (true) {
    emu_callback();               // Emulator callback
    hw_reset_handler();           // handle hard reset requests
    state_callback();             // manage state
    command_callback();           // process next command
    modbus_callback();            // handle modbus events
    io_callback();                // handle io input
    report_callback();            // report changes
  }

  return 0;
}
