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

#include "clock.h"

#include "hardware.h"

#include <avr/io.h>
#include <avr/interrupt.h>


/// This routine is lifted and modified from Boston Android and from
/// http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=711659
void clock_init()  {
  // external 8 Mhx Xtal with 4x PLL = 32 Mhz
#ifdef __CLOCK_EXTERNAL_8MHZ
  OSC.XOSCCTRL = 0x4B; // 2-9 MHz crystal; 0.4-16 MHz XTAL w/16K CLK startup
  OSC.CTRL = 0x08; // enable external crystal oscillator
  while (!(OSC.STATUS & OSC_XOSCRDY_bm)); // wait for oscillator ready

  OSC.PLLCTRL = 0xC4; // XOSC is PLL Source; 4x Factor (32 MHz sys clock)
  OSC.CTRL = 0x18;                         // Enable PLL & External Oscillator
  while (!(OSC.STATUS & OSC_PLLRDY_bm));   // wait for PLL ready

  CCP = CCP_IOREG_gc;
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;           // switch to PLL clock

  OSC.CTRL &= ~OSC_RC2MEN_bm;              // disable internal 2 MHz clock
#endif

  // external 16 Mhx Xtal with 2x PLL = 32 Mhz
#ifdef __CLOCK_EXTERNAL_16MHZ
  OSC.XOSCCTRL = 0xCB; // 12-16 MHz crystal; 0.4-16 MHz XTAL w/16K CLK startup
  OSC.CTRL = 0x08;                         // enable external crystal oscillator
  while (!(OSC.STATUS & OSC_XOSCRDY_bm));  // wait for oscillator ready

  OSC.PLLCTRL = 0xC2; // XOSC is PLL Source; 2x Factor (32 MHz sys clock)
  OSC.CTRL = 0x18;                         // Enable PLL & External Oscillator
  while(!(OSC.STATUS & OSC_PLLRDY_bm));    // wait for PLL ready

  CCP = CCP_IOREG_gc;
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;           // switch to PLL clock

  OSC.CTRL &= ~OSC_RC2MEN_bm;              // disable internal 2 MHz clock
#endif

  // 32 MHz internal clock (Boston Android code)
#ifdef __CLOCK_INTERNAL_32MHZ
  CCP = CCP_IOREG_gc;                      // Security Signature to modify clk
  OSC.CTRL = OSC_RC32MEN_bm;               // enable internal 32MHz oscillator
  while (!(OSC.STATUS & OSC_RC32MRDY_bm)); // wait for oscillator ready

  CCP = CCP_IOREG_gc;                      // Security Signature to modify clk
  CLK.CTRL = 0x01;                         // select sysclock 32MHz osc
#endif
}
