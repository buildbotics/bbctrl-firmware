/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include <avr/pgmspace.h>


// ritorno is a handy way to provide exception returns
// It returns only if an error occurred. (ritorno is Italian for return)
#define ritorno(a) if ((status_code = a) != STAT_OK) {return status_code;}

typedef enum {
#define MSG(NAME, TEXT) STAT_##NAME,
#include "messages.def"
#undef MSG

  STAT_MAX_VALUE = 255 // Do not exceed 255
} stat_t;


extern stat_t status_code;

const char *status_to_pgmstr(stat_t status);
stat_t status_error(stat_t status);
stat_t status_error_P(const char *location, const char *msg, stat_t status);

#define TO_STRING(x) _TO_STRING(x)
#define _TO_STRING(x) #x

#define STATUS_LOCATION PSTR(__FILE__ ":" TO_STRING(__LINE__))
#define STATUS_ERROR(MSG, CODE) status_error_P(STATUS_LOCATION, PSTR(MSG), CODE)
