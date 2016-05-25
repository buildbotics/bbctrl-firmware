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

#include "status.h"

#include <stdint.h>
#include <stdbool.h>


typedef enum {
  MOTOR_FLAG_ENABLED_bm       = 1 << 0,
  MOTOR_FLAG_STALLED_bm       = 1 << 1,
  MOTOR_FLAG_OVERTEMP_WARN_bm = 1 << 2,
  MOTOR_FLAG_OVERTEMP_bm      = 1 << 3,
  MOTOR_FLAG_SHORTED_bm       = 1 << 4,
  MOTOR_FLAG_ERROR_bm         = (MOTOR_FLAG_STALLED_bm |
                                 MOTOR_FLAG_OVERTEMP_WARN_bm |
                                 MOTOR_FLAG_OVERTEMP_bm |
                                 MOTOR_FLAG_SHORTED_bm)
} cmMotorFlags_t;


typedef enum {
  MOTOR_DISABLED,                 // motor enable is deactivated
  MOTOR_ALWAYS_POWERED,           // motor is always powered while machine is ON
  MOTOR_POWERED_IN_CYCLE,         // motor fully powered during cycles,
                                  // de-powered out of cycle
  MOTOR_POWERED_ONLY_WHEN_MOVING, // idles shortly after stopped, even in cycle
  MOTOR_POWER_MODE_MAX_VALUE      // for input range checking
} cmMotorPowerMode_t;


typedef enum {
  MOTOR_POLARITY_NORMAL,
  MOTOR_POLARITY_REVERSED
} cmMotorPolarity_t;


void motor_init();
void motor_shutdown(int motor);

int motor_get_axis(int motor);
float motor_get_steps_per_unit(int motor);
int32_t motor_get_encoder(int motor);
void motor_set_encoder(int motor, float encoder);
bool motor_error(int motor);
bool motor_stalled(int motor);
void motor_reset(int motor);

bool motor_energizing();

void motor_driver_callback(int motor);
stat_t motor_power_callback();
void motor_error_callback(int motor, cmMotorFlags_t errors);

void motor_prep_move(int motor, uint32_t seg_clocks, float travel_steps,
                     float error);
void motor_load_move(int motor);
void motor_end_move(int motor);
