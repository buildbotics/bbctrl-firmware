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

#include <stdbool.h>


enum {
  AXIS_X, AXIS_Y, AXIS_Z,
  AXIS_A, AXIS_B, AXIS_C,
  AXIS_U, AXIS_V, AXIS_W // reserved
};


typedef enum {
  HOMING_DISABLED,
  HOMING_STALL_MIN,
  HOMING_STALL_MAX,
  HOMING_SWITCH_MIN,
  HOMING_SWITCH_MAX,
} homing_mode_t;


bool axis_is_enabled(int axis);
char axis_get_char(int axis);
int axis_get_id(char axis);
int axis_get_motor(int axis);
void axis_set_motor(int axis, int motor);
float axis_get_vector_length(const float a[], const float b[]);

float axis_get_velocity_max(int axis);
float axis_get_feedrate_max(int axis);
float axis_get_jerk_max(int axis);
void axis_set_jerk_max(int axis, float jerk);
bool axis_get_homed(int axis);
void axis_set_homed(int axis, bool homed);
homing_mode_t axis_get_homing_mode(int axis);
void axis_set_homing_mode(int axis, homing_mode_t mode);
float axis_get_radius(int axis);
float axis_get_travel_min(int axis);
float axis_get_travel_max(int axis);
float axis_get_search_velocity(int axis);
float axis_get_latch_velocity(int axis);
float axis_get_zero_backoff(int axis);
float axis_get_latch_backoff(int axis);
float axis_get_junction_dev(int axis);
float axis_get_recip_jerk(int axis);
float axis_get_jerk_max(int axis);
