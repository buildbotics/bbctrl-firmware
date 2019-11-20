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

#include "axis.h"
#include "motor.h"
#include "switch.h"
#include "util.h"

#include <math.h>
#include <string.h>
#include <ctype.h>


int motor_map[AXES] = {-1, -1, -1, -1, -1, -1};


typedef struct {
  float velocity_max;    // max velocity in mm/min or deg/min
  float accel_max;       // max acceleration in mm/min^2
  float jerk_max;        // max jerk (Jm) in km/min^3
} axis_t;


axis_t axes[MOTORS] = {};


bool axis_is_enabled(int axis) {
  int motor = axis_get_motor(axis);
  return motor != -1 && motor_is_enabled(motor) &&
    !fp_ZERO(axis_get_velocity_max(axis));
}


int axis_get_id(char axis) {
  const char *axes = "XYZABCUVW";
  const char *ptr = strchr(axes, toupper(axis));
  return ptr == 0 ? -1 : (ptr - axes);
}


int axis_get_motor(int axis) {return motor_map[axis];}


bool axis_get_homed(int axis) {
  return axis_is_enabled(axis) ? motor_get_homed(axis_get_motor(axis)) : false;
}


float axis_get_soft_limit(int axis, bool min) {
  if (!axis_is_enabled(axis)) return min ? -INFINITY : INFINITY;
  return motor_get_soft_limit(axis_get_motor(axis), min);
}


// Map axes to first matching motor
void axis_map_motors() {
  for (int axis = 0; axis < AXES; axis++)
    for (int motor = 0; motor < MOTORS; motor++)
      if (motor_get_axis(motor) == axis) {
        motor_map[axis] = motor;
        break;
      }
}


#define AXIS_VAR_GET(NAME, TYPE)                        \
  TYPE get_##NAME(int axis) {return axes[axis].NAME;}


#define AXIS_VAR_SET(NAME, TYPE)                        \
  void set_##NAME(int axis, TYPE value) {axes[axis].NAME = value;}


/// Velocity is scaled by 1,000
float axis_get_velocity_max(int axis) {
  int motor = axis_get_motor(axis);
  return motor == -1 ? 0 : axes[motor].velocity_max * VELOCITY_MULTIPLIER;
}
AXIS_VAR_GET(velocity_max, float)


/// Acceleration is scaled by 1,000
float axis_get_accel_max(int axis) {
  int motor = axis_get_motor(axis);
  return motor == -1 ? 0 : axes[motor].accel_max * ACCEL_MULTIPLIER;
}
AXIS_VAR_GET(accel_max, float)


/// Jerk is scaled by 1,000,000
float axis_get_jerk_max(int axis) {
  int motor = axis_get_motor(axis);
  return motor == -1 ? 0 : axes[motor].jerk_max * JERK_MULTIPLIER;
}
AXIS_VAR_GET(jerk_max, float)


AXIS_VAR_SET(velocity_max, float)
AXIS_VAR_SET(accel_max, float)
AXIS_VAR_SET(jerk_max, float)
