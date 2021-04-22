/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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

#define PIN_INDEX(PIN) (PIN & 7)
#define PORT_INDEX(PIN) ((PIN >> 3) - 1)

#define PIN_PORT(PIN) pin_ports[PORT_INDEX(PIN)]
#define PIN_BM(PIN) (1 << PIN_INDEX(PIN))

#define PIN_ID(PORT, PIN) (PORT << 3 | (PIN & 7))

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
