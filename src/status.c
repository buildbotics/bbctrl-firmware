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

#include <avr/pgmspace.h>
#include <stdio.h>

stat_t status_code; // allocate a variable for the ritorno macro

#define MSG(N, TEXT) static const char stat_##N[] PROGMEM = TEXT;
#include "messages.def"
#undef MSG

static const char *const stat_msg[] PROGMEM = {
#define MSG(N, TEXT) stat_##N,
#include "messages.def"
#undef MSG
};


/// Return the status message
void print_status_message(const char *msg, stat_t status) {
  printf_P("%S: %S (%d)\n", pgm_read_word(&stat_msg[status]));
}
