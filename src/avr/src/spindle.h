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
#include <stdint.h>


typedef enum {
  POWER_IGNORE,
  POWER_FORWARD,
  POWER_REVERSE
} power_state_t;


typedef struct {
  power_state_t state;
  float power;
  uint16_t period; // Used by PWM
} power_update_t;


typedef enum {
  SPINDLE_TYPE_DISABLED,
  SPINDLE_TYPE_PWM,
  SPINDLE_TYPE_HUANYANG,
  SPINDLE_TYPE_CUSTOM,
  SPINDLE_TYPE_AC_TECH,
  SPINDLE_TYPE_NOWFOREVER,
  SPINDLE_TYPE_DELTA_VFD015M21A,
  SPINDLE_TYPE_YL600,
  SPINDLE_TYPE_FR_D700,
  SPINDLE_TYPE_SUNFAR_E300,
  SPINDLE_TYPE_OMRON_MX2,
  SPINDLE_TYPE_V70,
} spindle_type_t;


typedef void (*deinit_cb_t)();


spindle_type_t spindle_get_type();
void spindle_stop();
void spindle_estop();
void spindle_load_power_updates(power_update_t updates[], float minD,
                                float maxD);
void spindle_update(const power_update_t &update);
void spindle_update_speed();
void spindle_idle();
