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
