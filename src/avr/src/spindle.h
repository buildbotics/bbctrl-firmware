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

#include <stdbool.h>


typedef enum {
  SPINDLE_TYPE_DISABLED,
  SPINDLE_TYPE_PWM,
  SPINDLE_TYPE_HUANYANG,
  SPINDLE_TYPE_TEST,
  SPINDLE_TYPE_CUSTOM,
  SPINDLE_TYPE_YL600,
  SPINDLE_TYPE_AC_TECH,
  SPINDLE_TYPE_FR_D700,
} spindle_type_t;


typedef void (*deinit_cb_t)();


spindle_type_t spindle_get_type();
void spindle_set_speed(float speed);
float spindle_get_speed();
void spindle_stop();
bool spindle_is_reversed();
