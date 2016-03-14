/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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
#include "switch.h"
#include "controller.h"
#include "clock.h"
#include "rtc.h"

#include <avr/pgmspace.h> // defines PROGMEM and PSTR
#include <avr/wdt.h>      // used for software reset

#include <stdbool.h>


hwSingleton_t hw;


/// Bind XMEGA ports to hardware
static void _port_bindings() {
  hw.st_port[0] = &PORT_MOTOR_1;
  hw.st_port[1] = &PORT_MOTOR_2;
  hw.st_port[2] = &PORT_MOTOR_3;
  hw.st_port[3] = &PORT_MOTOR_4;

  hw.sw_port[0] = &PORT_SWITCH_X;
  hw.sw_port[1] = &PORT_SWITCH_Y;
  hw.sw_port[2] = &PORT_SWITCH_Z;
  hw.sw_port[3] = &PORT_SWITCH_A;

  hw.out_port[0] = &PORT_OUT_X;
  hw.out_port[1] = &PORT_OUT_Y;
  hw.out_port[2] = &PORT_OUT_Z;
  hw.out_port[3] = &PORT_OUT_A;
}


/// Lowest level hardware init
void hardware_init() {
  clock_init();                            // set system clock
  _port_bindings();
  rtc_init();                              // real time counter
}


/* Get a human readable device signature
 *
 * Produce a unique deviceID based on the factory calibration data.
 *
 *     Format is: 123456-ABC
 *
 * The number part is a direct readout of the 6 digit lot number
 * The alpha is the low 5 bits of wafer number and XY coords in printable ASCII
 * Refer to NVM_PROD_SIGNATURES_t in iox192a3.h for details.
 */
enum {
  LOTNUM0 = 8,  // Lot Number Byte 0, ASCII
  LOTNUM1,      // Lot Number Byte 1, ASCII
  LOTNUM2,      // Lot Number Byte 2, ASCII
  LOTNUM3,      // Lot Number Byte 3, ASCII
  LOTNUM4,      // Lot Number Byte 4, ASCII
  LOTNUM5,      // Lot Number Byte 5, ASCII
  WAFNUM = 16,  // Wafer Number
  COORDX0 = 18, // Wafer Coordinate X Byte 0
  COORDX1,      // Wafer Coordinate X Byte 1
  COORDY0,      // Wafer Coordinate Y Byte 0
  COORDY1,      // Wafer Coordinate Y Byte 1
};


void hw_get_id(char *id) {
  char printable[33] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  uint8_t i;

  // Load NVM Command register to read the calibration row
  NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;

  for (i = 0; i < 6; i++)
    id[i] = pgm_read_byte(LOTNUM0 + i);

  id[i++] = '-';
  id[i++] = printable[(pgm_read_byte(WAFNUM) & 0x1F)];
  id[i++] = printable[(pgm_read_byte(COORDX0) & 0x1F)];
  id[i++] = printable[(pgm_read_byte(COORDY0) & 0x1F)];
  id[i] = 0;

  NVM_CMD = NVM_CMD_NO_OPERATION_gc; // Clean up NVM Command register
}


void hw_request_hard_reset() {cs.hard_reset_requested = true;}


/// Hard reset using watchdog timer
/// software hard reset using the watchdog timer
void hw_hard_reset() {
  wdt_enable(WDTO_15MS);
  while (true) continue; // loops for about 15ms then resets
}


/// Controller's rest handler
stat_t hw_hard_reset_handler() {
  if (!cs.hard_reset_requested) return STAT_NOOP;
  hw_hard_reset(); // identical to hitting RESET button
  return STAT_EAGAIN;
}


/// Executes a software reset using CCPWrite
void hw_request_bootloader() {cs.bootloader_requested = true;}


stat_t hw_bootloader_handler() {
  if (!cs.bootloader_requested) return STAT_NOOP;

  cli();
  CCPWrite(&RST.CTRL, RST_SWRST_bm); // fire a software reset

  return STAT_EAGAIN; // never gets here but keeps the compiler happy
}
