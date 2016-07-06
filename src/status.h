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


// RITORNO is a handy way to provide exception returns
// It returns only if an error occurred. (ritorno is Italian for return)
#define RITORNO(a) if ((status_code = a) != STAT_OK) {return status_code;}

typedef enum {
#define STAT_MSG(NAME, TEXT) STAT_##NAME,
#include "messages.def"
#undef STAT_MSG

  STAT_DO_NOT_EXCEED = 255 // Do not exceed 255
} stat_t;


typedef enum {
  STAT_LEVEL_INFO,
  STAT_LEVEL_DEBUG,
  STAT_LEVEL_WARNING,
  STAT_LEVEL_ERROR,
} status_level_t;


extern stat_t status_code;

const char *status_to_pgmstr(stat_t code);
const char *status_level_pgmstr(status_level_t level);
stat_t status_error(stat_t code);
stat_t status_message_P(const char *location, status_level_t level,
                        stat_t code, const char *msg, ...);
void status_help();

#define TO_STRING(x) _TO_STRING(x)
#define _TO_STRING(x) #x

#define STATUS_LOCATION PSTR(__FILE__ ":" TO_STRING(__LINE__))

#define STATUS_MESSAGE(LEVEL, CODE, MSG, ...)                           \
  status_message_P(STATUS_LOCATION, LEVEL, CODE, PSTR(MSG), ##__VA_ARGS__)

#define STATUS_INFO(MSG, ...)                                   \
  STATUS_MESSAGE(STAT_LEVEL_INFO, STAT_OK, MSG, ##__VA_ARGS__)

#define STATUS_DEBUG(MSG, ...)                                  \
  STATUS_MESSAGE(STAT_LEVEL_WARNING, STAT_OK, MSG, ##__VA_ARGS__)

#define STATUS_WARNING(MSG, ...)                                \
  STATUS_MESSAGE(STAT_LEVEL_WARNING, STAT_OK, MSG, ##__VA_ARGS__)

#define STATUS_ERROR(MSG, CODE, ...)                        \
  STATUS_MESSAGE(STAT_LEVEL_ERROR, CODE, MSG, ##__VA_ARGS__)
