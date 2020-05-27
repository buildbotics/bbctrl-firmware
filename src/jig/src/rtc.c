/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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

#include "rtc.h"

#include "encoder.h"

#include <avr/io.h>
#include <avr/interrupt.h>


static uint32_t ticks;


ISR(RTC_OVF_vect) {
  ticks++;
  encoder_rtc_callback();
}


/// Initialize and start the clock
/// This routine follows the code in app note AVR1314.
void rtc_init() {
  ticks = 0;

  PR.PRGEN &= ~PR_RTC_bm; // Disable power reduction

  OSC.CTRL |= OSC_RC32KEN_bm;                         // enable internal 32kHz.
  while (!(OSC.STATUS & OSC_RC32KRDY_bm));            // 32kHz osc stabilize
  while (RTC.STATUS & RTC_SYNCBUSY_bm);               // wait RTC not busy

  CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm; // 32kHz clock as RTC src
  while (RTC.STATUS & RTC_SYNCBUSY_bm);               // wait RTC not busy

  // the following must be in this order or it doesn't work
  RTC.PER = 33;                        // overflow period ~1ms
  RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;   // overflow LO interrupt
  RTC.CTRL = RTC_PRESCALER_DIV1_gc;    // no prescale

  PMIC.CTRL |= PMIC_LOLVLEN_bm; // Interrupt level on
}


uint32_t rtc_get_time() {return ticks;}
bool rtc_expired(uint32_t t) {return 0 <= (int32_t)(ticks - t);}
