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

#include "status.h"

#include <stdio.h>

stat_t status_code; // allocate a variable for the ritorno macro

#define STAT_MSG(NAME, TEXT) static const char stat_##NAME[] PROGMEM = TEXT;
#include "messages.def"
#undef STAT_MSG

static const char *const stat_msg[] PROGMEM = {
#define STAT_MSG(NAME, TEXT) stat_##NAME,
#include "messages.def"
#undef STAT_MSG
};


const char *status_to_pgmstr(stat_t status) {
  return pgm_read_ptr(&stat_msg[status]);
}


stat_t status_error(stat_t status) {
  return status_error_P(0, 0, status);
}


stat_t status_error_P(const char *location, const char *msg, stat_t status) {
  printf_P(PSTR("\n{\"error\": %d, \"code\": %d"),
           status_to_pgmstr(status), status);

  if (msg) printf_P(PSTR(", \"msg\": %S"), msg);
  if (location) printf_P(PSTR(", \"location\": %S"), location);

  putchar('}');
  putchar('\n');

  return status;
}
