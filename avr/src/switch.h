/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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


typedef enum {
  SW_DISABLED,
  SW_NORMALLY_OPEN,
  SW_NORMALLY_CLOSED
} switch_type_t;


/// Switch IDs
typedef enum {
  SW_MIN_X, SW_MAX_X,
  SW_MIN_Y, SW_MAX_Y,
  SW_MIN_Z, SW_MAX_Z,
  SW_MIN_A, SW_MAX_A,
  SW_ESTOP, SW_PROBE
} switch_id_t;


typedef void (*switch_callback_t)(switch_id_t sw, bool active);


void switch_init();
void switch_rtc_callback();
bool switch_is_active(int index);
bool switch_is_enabled(int index);
switch_type_t switch_get_type(int index);
void switch_set_type(int index, switch_type_t type);
void switch_set_callback(int index, switch_callback_t cb);
