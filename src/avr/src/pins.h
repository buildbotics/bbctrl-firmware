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

#pragma once

enum {PORT_A = 1, PORT_B, PORT_C, PORT_D, PORT_E, PORT_F};

#define PIN_INDEX(PIN) (PIN & 7)
#define PORT_INDEX(PIN) ((PIN >> 3) - 1)

#define PIN_PORT(PIN) pin_ports[PORT_INDEX(PIN)]
#define PIN_BM(PIN) (1 << PIN_INDEX(PIN))

#define PIN_ID(PORT, PIN) (PORT << 3 | (PIN & 7))

#ifdef __AVR__
#include <avr/io.h>

extern PORT_t *pin_ports[];

#define DIRSET_PIN(PIN) PIN_PORT(PIN)->DIRSET = PIN_BM(PIN)
#define DIRCLR_PIN(PIN) PIN_PORT(PIN)->DIRCLR = PIN_BM(PIN)
#define OUTCLR_PIN(PIN) PIN_PORT(PIN)->OUTCLR = PIN_BM(PIN)
#define OUTSET_PIN(PIN) PIN_PORT(PIN)->OUTSET = PIN_BM(PIN)
#define OUTTGL_PIN(PIN) PIN_PORT(PIN)->OUTTGL = PIN_BM(PIN)
#define OUT_PIN(PIN) (!!(PIN_PORT(PIN)->OUT & PIN_BM(PIN)))
#define IN_PIN(PIN) (!!(PIN_PORT(PIN)->IN & PIN_BM(PIN)))
#define PINCTRL_PIN(PIN) ((&PIN_PORT(PIN)->PIN0CTRL)[PIN_INDEX(PIN)])

#define SET_PIN(PIN, X) \
  do {if (X) OUTSET_PIN(PIN); else OUTCLR_PIN(PIN);} while (0);

#define PIN_EVSYS_CHMUX(PIN) \
  (EVSYS_CHMUX_PORTA_PIN0_gc + (8 * PORT_INDEX(PIN)) + PIN_INDEX(PIN))

#endif // __AVR__
