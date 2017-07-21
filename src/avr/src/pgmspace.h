/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#ifdef __AVR__

#include <avr/pgmspace.h>
#define PRPSTR "S"

#else // __AVR__

#define PRPSTR "s"
#define PROGMEM
#define PGM_P char *
#define PSTR(X) X
#define vfprintf_P vfprintf
#define printf_P printf
#define puts_P puts
#define sprintf_P sprintf
#define strcmp_P strcmp
#define pgm_read_ptr(x) *(x)
#define pgm_read_word(x) *(x)
#define pgm_read_byte(x) *(x)

#endif // __AVR__
