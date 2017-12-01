/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "status.h"

#include <stdint.h>
#include <stdbool.h>


typedef enum {
  MOTOR_DISABLED,                 // motor enable is deactivated
  MOTOR_ALWAYS_POWERED,           // motor is always powered while machine is ON
  MOTOR_POWERED_IN_CYCLE,         // motor fully powered during cycles,
                                  // de-powered out of cycle
  MOTOR_POWERED_ONLY_WHEN_MOVING, // idles shortly after stopped, even in cycle
} motor_power_mode_t;


void motor_init();

bool motor_is_enabled(int motor);
int motor_get_axis(int motor);
void motor_set_position(int motor, float position);

stat_t motor_rtc_callback();

void motor_end_move(int motor);
void motor_load_move(int motor);
void motor_prep_move(int motor, float time, float target);
