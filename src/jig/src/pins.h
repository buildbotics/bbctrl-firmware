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
