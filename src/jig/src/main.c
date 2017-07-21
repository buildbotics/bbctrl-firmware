/******************************************************************************\

           This file is part of the Buildbotics test jig firmware.

                    Copyright (c) 2017 Buildbotics LLC
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
