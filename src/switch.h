/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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


#include "config.h"

#include <stdint.h>
#include <stdbool.h>

// macros for finding the index into the switch table give the axis number
#define MIN_SWITCH(axis) (axis * 2)
#define MAX_SWITCH(axis) (axis * 2 + 1)

/// switch modes
typedef enum {
  SW_MODE_DISABLED,
  SW_HOMING_BIT,
  SW_LIMIT_BIT
} swMode_t;

#define SW_MODE_HOMING        SW_HOMING_BIT   // enable switch for homing only
#define SW_MODE_LIMIT         SW_LIMIT_BIT    // enable switch for limits only
#define SW_MODE_HOMING_LIMIT  (SW_HOMING_BIT | SW_LIMIT_BIT) // homing & limits

typedef enum {
  SW_TYPE_NORMALLY_OPEN,
  SW_TYPE_NORMALLY_CLOSED
} swType_t;

/// indices into switch arrays
typedef enum {
  SW_MIN_X, SW_MAX_X,
  SW_MIN_Y, SW_MAX_Y,
  SW_MIN_Z, SW_MAX_Z,
  SW_MIN_A, SW_MAX_A
} swNums_t;


void switch_init();
void switch_rtc_callback();
bool switch_get_closed(int index);
swType_t switch_get_type(int index);
void switch_set_type(int index, swType_t type);
swMode_t switch_get_mode(int index);
void switch_set_mode(int index, swMode_t mode);
bool switch_get_limit_thrown();
