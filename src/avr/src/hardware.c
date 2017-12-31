/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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
#include "huanyang.h"
#include "config.h"
#include "pgmspace.h"

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include <stdbool.h>
#include <stddef.h>


typedef struct {
  char id[26];
  bool hard_reset;         // flag to perform a hard reset
  bool bootloader;         // flag to enter the bootloader
} hw_t;

static hw_t hw = {{0}};


#define PROD_SIGS (*(NVM_PROD_SIGNATURES_t *)0x0000)
#define HEXNIB(x) "0123456789abcdef"[(x) & 0xf]


static void _init_clock()  {
#if 0 // 32Mhz Int RC
  OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;  // Enable 32MHz & 32KHz osc
  while (!(OSC.STATUS & OSC_RC32KRDY_bm));      // Wait for 32Khz oscillator
  while (!(OSC.STATUS & OSC_RC32MRDY_bm));      // Wait for 32MHz oscillator

  // Defaults to calibrate against internal 32Khz clock
  DFLLRC32M.CTRL = DFLL_ENABLE_bm;              // Enable DFLL
  CCP = CCP_IOREG_gc;                           // Disable register security
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc;              // Switch to 32MHz clock

#else
  // 12-16 MHz crystal; 0.4-16 MHz XTAL w/ 16K CLK startup
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
  OSC.CTRL = OSC_XOSCEN_bm;                // enable external crystal oscillator
  while (!(OSC.STATUS & OSC_XOSCRDY_bm));  // wait for oscillator ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 2;    // PLL source, 2x (32 MHz sys clock)
  OSC.CTRL = OSC_PLLEN_bm | OSC_XOSCEN_bm; // Enable PLL & External Oscillator
  while (!(OSC.STATUS & OSC_PLLRDY_bm));   // wait for PLL ready

  CCP = CCP_IOREG_gc;
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;           // switch to PLL clock
#endif

  OSC.CTRL &= ~OSC_RC2MEN_bm;              // disable internal 2 MHz clock
}


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
  _init_clock();                           // set system clock
  rtc_init();                              // real time counter
  _read_hw_id();

  // Round-robin, interrupts in application section, all interupts levels
  CCP = CCP_IOREG_gc;
  PMIC.CTRL =
    PMIC_RREN_bm | PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
}


void hw_request_hard_reset() {hw.hard_reset = true;}


/// Hard reset using watchdog timer
/// software hard reset using the watchdog timer
void hw_hard_reset() {
  usart_flush();
  cli();
  CCP = CCP_IOREG_gc;
  RST.CTRL = RST_SWRST_bm;
}


/// Controller's rest handler
void hw_reset_handler() {
  if (hw.hard_reset) {
    while (!usart_tx_empty() || !eeprom_is_ready())
      continue;

    hw_hard_reset();
  }

  if (hw.bootloader) {
    // TODO enable bootloader interrupt vectors and jump to BOOT_SECTION_START
    hw.bootloader = false;
  }
}


/// Executes a software reset using CCPWrite
void hw_request_bootloader() {hw.bootloader = true;}


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
