/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2013 Alden S. Hart Jr.
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

#include "rtc.h"

#include "switch.h"

#include <avr/io.h>
#include <avr/interrupt.h>


rtClock_t rtc; // allocate clock control struct

/// Initialize and start the clock
/// This routine follows the code in app note AVR1314.
void rtc_init() {
  OSC.CTRL |= OSC_RC32KEN_bm;                       // enable internal 32kHz.
  while (!(OSC.STATUS & OSC_RC32KRDY_bm));          // 32kHz osc stabilize
  while (RTC.STATUS & RTC_SYNCBUSY_bm);             // wait RTC not busy

  CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc | CLK_RTCEN_bm; // 32kHz clock as RTC src
  while (RTC.STATUS & RTC_SYNCBUSY_bm);             // wait RTC not busy

  // the following must be in this order or it doesn't work
  RTC.PER = RTC_MILLISECONDS - 1;                   // overflow period ~10ms
  RTC.CNT = 0;
  RTC.COMP = RTC_MILLISECONDS - 1;
  RTC.CTRL = RTC_PRESCALER_DIV1_gc;                 // no prescale (1x)
  RTC.INTCTRL = RTC_COMPINTLVL;                     // interrupt on compare
  rtc.rtc_ticks = 0;                                // reset tick counter
  rtc.sys_ticks = 0;                                // reset tick counter
}


/* rtc ISR
 *
 * It is the responsibility of callback code to ensure atomicity and volatiles
 * are observed correctly as the callback will be run at the interrupt level.
 *
 * Here's the code in case the main loop (non-interrupt) function needs to
 * create a critical region for variables set or used by the callback:
 *
 *        #include "gpio.h"
 *        #include "rtc.h"
 *
 *        RTC.INTCTRL = RTC_OVFINTLVL_OFF_gc;    // disable interrupt
 *         blah blah blah critical region
 *        RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;     // enable interrupt
 */
ISR(RTC_COMP_vect) {
  rtc.sys_ticks = ++rtc.rtc_ticks * 10;      // advance both tick counters

  // callbacks to whatever you need to happen on each RTC tick go here:
  switch_rtc_callback();                     // switch debouncing
}


/// This is a hack to get around some compatibility problems
uint32_t rtc_get_time() {
  return rtc.sys_ticks;
}
