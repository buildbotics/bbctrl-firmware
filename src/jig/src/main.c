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

#include "usart.h"
#include "rtc.h"
#include "encoder.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include <stdio.h>
#include <stdbool.h>


static void _clock_init()  {
  // 12-16 MHz crystal; 0.4-16 MHz XTAL w/ 16K CLK startup
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
  OSC.CTRL = OSC_XOSCEN_bm;                // enable external crystal oscillator
  while (!(OSC.STATUS & OSC_XOSCRDY_bm));  // wait for oscillator ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 2;    // PLL source, 2x (32 MHz sys clock)
  OSC.CTRL = OSC_PLLEN_bm | OSC_XOSCEN_bm; // Enable PLL & External Oscillator
  while (!(OSC.STATUS & OSC_PLLRDY_bm));   // wait for PLL ready

  CCP = CCP_IOREG_gc;
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;           // switch to PLL clock

  OSC.CTRL &= ~OSC_RC2MEN_bm;              // disable internal 2 MHz clock
}


int main() {
  // Init
  cli();                          // disable interrupts

  _clock_init();
  rtc_init();                     // real-time clock
  usart_init();                   // serial port
  encoder_init();                 // rotary encoders

  sei();                          // enable interrupts

  // Splash
  fprintf_P(stdout, PSTR("\nBuildbotics Jig " VERSION "\n"));

  usart_set(USART_ECHO, true);

  uint32_t timer = 0;
  uint16_t last = 0;

  // Main loop
  while (true) {
    char *line = usart_readline();
    if (line) {
      printf("\n%s\n", line);
      printf("%ld\n", rtc_get_time());
    }

    if (rtc_expired(timer)) {
      timer += 100;

      uint16_t encoder = encoder_get_position(0);
      if (encoder != last) {
        last = encoder;
        printf("%5d step %8.2f step/sec\n", encoder, encoder_get_velocity(0));
      }
    }
  }

  return 0;
}
