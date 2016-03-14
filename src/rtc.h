/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2013 Alden S. Hart Jr.
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


#include <stdint.h>

#define RTC_MILLISECONDS 10 // interrupt on every 10 RTC ticks (~10 ms)

// Interrupt level
#define RTC_COMPINTLVL RTC_COMPINTLVL_LO_gc;

// Note: sys_ticks is in ms but is only accurate to 10 ms as it's derived from
// rtc_ticks
typedef struct rtClock {
  uint32_t rtc_ticks; // RTC tick counter, 10 uSec each
  uint32_t sys_ticks; // system tick counter, 1 ms each
} rtClock_t;

extern rtClock_t rtc;

void rtc_init(); // initialize and start general timer
uint32_t rtc_get_time();
