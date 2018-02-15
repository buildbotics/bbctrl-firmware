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

#define PORT(PIN) pin_ports[(PIN >> 3) - 1]
#define BM(PIN) (1 << (PIN & 7))

#ifdef __AVR__
#include <avr/io.h>

extern PORT_t *pin_ports[];

#define DIRSET_PIN(PIN) PORT(PIN)->DIRSET = BM(PIN)
#define DIRCLR_PIN(PIN) PORT(PIN)->DIRCLR = BM(PIN)
#define OUTCLR_PIN(PIN) PORT(PIN)->OUTCLR = BM(PIN)
#define OUTSET_PIN(PIN) PORT(PIN)->OUTSET = BM(PIN)
#define OUTTGL_PIN(PIN) PORT(PIN)->OUTTGL = BM(PIN)
#define OUT_PIN(PIN) (!!(PORT(PIN)->OUT & BM(PIN)))
#define IN_PIN(PIN) (!!(PORT(PIN)->IN & BM(PIN)))
#define PINCTRL_PIN(PIN) ((&PORT(PIN)->PIN0CTRL)[PIN & 7])

#define SET_PIN(PIN, X) \
  do {if (X) OUTSET_PIN(PIN); else OUTCLR_PIN(PIN);} while (0);

#endif // __AVR__
