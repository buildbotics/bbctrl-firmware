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

enum {PORT_A = 1, PORT_B, PORT_C};
enum {IO_REG_PIN, IO_REG_DDR, IO_REG_PORT, IO_REG_PUE};

#define IO_REG(PIN, REG) _SFR_IO8(io_base[((PIN) >> 3) - 1] + (REG))
#define BM(PIN) (1 << ((PIN) & 7))

#include <avr/io.h>

extern uint16_t io_base[];

#define IO_REG_GET(PIN, REG) (!!(IO_REG(PIN, REG) & BM(PIN)))
#define IO_REG_SET(PIN, REG) do {IO_REG(PIN, REG) |= BM(PIN);} while (0)
#define IO_REG_CLR(PIN, REG) do {IO_REG(PIN, REG) &= ~BM(PIN);} while (0)
#define IO_REG_TGL(PIN, REG) do {IO_REG(PIN, REG) ^= BM(PIN);} while (0)

#define IO_PIN_GET(PIN) IO_REG_GET(PIN, IO_REG_PIN)
#define IO_PIN_SET(PIN) IO_REG_SET(PIN, IO_REG_PIN)
#define IO_PIN_CLR(PIN) IO_REG_CLR(PIN, IO_REG_PIN)
#define IO_PIN_TGL(PIN) IO_REG_TGL(PIN, IO_REG_PIN)

#define IO_DDR_GET(PIN) IO_REG_GET(PIN, IO_REG_DDR)
#define IO_DDR_SET(PIN) IO_REG_SET(PIN, IO_REG_DDR)
#define IO_DDR_CLR(PIN) IO_REG_CLR(PIN, IO_REG_DDR)
#define IO_DDR_TGL(PIN) IO_REG_TGL(PIN, IO_REG_DDR)

#define IO_PORT_GET(PIN) IO_REG_GET(PIN, IO_REG_PORT)
#define IO_PORT_SET(PIN) IO_REG_SET(PIN, IO_REG_PORT)
#define IO_PORT_CLR(PIN) IO_REG_CLR(PIN, IO_REG_PORT)
#define IO_PORT_TGL(PIN) IO_REG_TGL(PIN, IO_REG_PORT)

#define IO_PUE_GET(PIN) IO_REG_GET(PIN, IO_REG_PUE)
#define IO_PUE_SET(PIN) IO_REG_SET(PIN, IO_REG_PUE)
#define IO_PUE_CLR(PIN) IO_REG_CLR(PIN, IO_REG_PUE)
#define IO_PUE_TGL(PIN) IO_REG_TGL(PIN, IO_REG_PUE)
