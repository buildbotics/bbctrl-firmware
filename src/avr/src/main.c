/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#include "hardware.h"
#include "stepper.h"
#include "motor.h"
#include "usart.h"
#include "drv8711.h"
#include "vars.h"
#include "rtc.h"
#include "report.h"
#include "command.h"
#include "estop.h"
#include "i2c.h"
#include "pgmspace.h"
#include "io.h"
#include "modbus.h"
#include "input.h"
#include "exec.h"
#include "state.h"
#include "seek.h"
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
  io_init();                      // io pins
  estop_init();                   // emergency stop handler
  usart_init();                   // serial port
  i2c_init();                     // i2c port
  drv8711_init();                 // motor drivers
  stepper_init();                 // steppers
  motor_init();                   // motors
  exec_init();                    // motion exec
  seek_init();                    // seeking moves
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
    input_callback();             // handle digital input
    report_callback();            // report changes
  }

  return 0;
}
