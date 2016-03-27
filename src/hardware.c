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
#include "rtc.h"
#include "usart.h"
#include "clock.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <stdbool.h>
#include <stddef.h>


typedef struct {
  char id[26];
  bool hard_reset_requested;         // flag to perform a hard reset
  bool bootloader_requested;         // flag to enter the bootloader
} hw_t;

static hw_t hw = {};


#define PROD_SIGS (*(NVM_PROD_SIGNATURES_t *)0x0000)
#define HEXNIB(x) "0123456789abcdef"[(x) & 0xf]


static void _load_hw_id_byte(int i, register8_t *reg) {
  NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
  uint8_t byte = pgm_read_byte(reg);
  NVM.CMD = NVM_CMD_NO_OPERATION_gc;

  hw.id[i] = HEXNIB(byte >> 4);
  hw.id[i + 1] = HEXNIB(byte);
}


static void _read_hw_id() {
  int i = 0;
  _load_hw_id_byte(i, &PROD_SIGS.LOTNUM5); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.LOTNUM4); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.LOTNUM3); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.LOTNUM2); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.LOTNUM1); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.LOTNUM0); i += 2;
  hw.id[i++] = '-';
  _load_hw_id_byte(i, &PROD_SIGS.WAFNUM);  i += 2;
  hw.id[i++] = '-';
  _load_hw_id_byte(i, &PROD_SIGS.COORDX1); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.COORDX0); i += 2;
  hw.id[i++] = '-';
  _load_hw_id_byte(i, &PROD_SIGS.COORDY1); i += 2;
  _load_hw_id_byte(i, &PROD_SIGS.COORDY0); i += 2;
  hw.id[i] = 0;
}


/// Lowest level hardware init
void hardware_init() {
  clock_init();                            // set system clock
  rtc_init();                              // real time counter
  _read_hw_id();

  // Round-robin, interrupts in application section, all interupts levels
  CCP = CCP_IOREG_gc;
  PMIC.CTRL =
    PMIC_RREN_bm | PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
}


void hw_request_hard_reset() {hw.hard_reset_requested = true;}


/// Hard reset using watchdog timer
/// software hard reset using the watchdog timer
void hw_hard_reset() {
  usart_flush();
  cli();
  CCP = CCP_IOREG_gc;
  RST.CTRL = RST_SWRST_bm;
}


/// Controller's rest handler
stat_t hw_reset_handler() {
  if (hw.hard_reset_requested) hw_hard_reset();

  if (hw.bootloader_requested) {
    // TODO enable bootloader interrupt vectors and jump to BOOT_SECTION_START
    hw.bootloader_requested = false;
  }

  return STAT_NOOP;
}


/// Executes a software reset using CCPWrite
void hw_request_bootloader() {hw.bootloader_requested = true;}


uint8_t hw_disable_watchdog() {
  uint8_t state = WDT.CTRL;
  wdt_disable();
  return state;
}


void hw_restore_watchdog(uint8_t state) {
  cli();
  CCP = CCP_IOREG_gc;
  WDT.CTRL = state | WDT_CEN_bm;
  sei();
}


const char *get_hw_id() {
  return hw.id;
}
