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

#include "status.h"
#include "estop.h"
#include "usart.h"

#include <stdio.h>
#include <stdarg.h>


stat_t status_code; // allocate a variable for the RITORNO macro


#define STAT_MSG(NAME, TEXT) static const char stat_##NAME[] PROGMEM = TEXT;
#include "messages.def"
#undef STAT_MSG


static const char *const stat_msg[] PROGMEM = {
#define STAT_MSG(NAME, TEXT) stat_##NAME,
#include "messages.def"
#undef STAT_MSG
};


const char *status_to_pgmstr(stat_t code) {
  return pgm_read_ptr(&stat_msg[code]);
}


const char *status_level_pgmstr(status_level_t level) {
  switch (level) {
  case STAT_LEVEL_INFO: return PSTR("info");
  case STAT_LEVEL_DEBUG: return PSTR("debug");
  case STAT_LEVEL_WARNING: return PSTR("warning");
  default: return PSTR("error");
  }
}


stat_t status_message_P(const char *location, status_level_t level,
                        stat_t code, const char *msg, ...) {
  va_list args;

  // Type
  printf_P(PSTR("\n{\"level\":\"%"PRPSTR"\",\"msg\":\""),
           status_level_pgmstr(level));

  // Message
  if (msg && pgm_read_byte(msg)) {
    va_start(args, msg);
    vfprintf_P(stdout, msg, args);
    va_end(args);

  } else printf_P(PSTR("%" PRPSTR), status_to_pgmstr(code));

  putchar('"');

  // Code
  if (code) printf_P(PSTR(",\"code\":%d"), code);

  // Location
  if (location) printf_P(PSTR(",\"where\":\"%"PRPSTR"\""), location);

  putchar('}');
  putchar('\n');

  return code;
}


/// Alarm state; send an exception report and stop processing input
stat_t status_alarm(const char *location, stat_t code, const char *msg) {
  status_message_P(location, STAT_LEVEL_ERROR, code, msg);
  estop_trigger(code);
  while (!usart_tx_empty()) continue;
  return code;
}
