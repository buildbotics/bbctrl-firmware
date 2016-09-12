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


#include "axes.h"

#include "plan/planner.h"

#include <math.h>


axis_config_t axes[AXES] = {
  {
    .axis_mode =         X_AXIS_MODE,
    .velocity_max =      X_VELOCITY_MAX,
    .feedrate_max =      X_FEEDRATE_MAX,
    .travel_min =        X_TRAVEL_MIN,
    .travel_max =        X_TRAVEL_MAX,
    .jerk_max =          X_JERK_MAX,
    .jerk_homing =       X_JERK_HOMING,
    .junction_dev =      X_JUNCTION_DEVIATION,
    .search_velocity =   X_SEARCH_VELOCITY,
    .latch_velocity =    X_LATCH_VELOCITY,
    .latch_backoff =     X_LATCH_BACKOFF,
    .zero_backoff =      X_ZERO_BACKOFF,
  }, {
    .axis_mode =         Y_AXIS_MODE,
    .velocity_max =      Y_VELOCITY_MAX,
    .feedrate_max =      Y_FEEDRATE_MAX,
    .travel_min =        Y_TRAVEL_MIN,
    .travel_max =        Y_TRAVEL_MAX,
    .jerk_max =          Y_JERK_MAX,
    .jerk_homing =       Y_JERK_HOMING,
    .junction_dev =      Y_JUNCTION_DEVIATION,
    .search_velocity =   Y_SEARCH_VELOCITY,
    .latch_velocity =    Y_LATCH_VELOCITY,
    .latch_backoff =     Y_LATCH_BACKOFF,
    .zero_backoff =      Y_ZERO_BACKOFF,
  }, {
    .axis_mode =         Z_AXIS_MODE,
    .velocity_max =      Z_VELOCITY_MAX,
    .feedrate_max =      Z_FEEDRATE_MAX,
    .travel_min =        Z_TRAVEL_MIN,
    .travel_max =        Z_TRAVEL_MAX,
    .jerk_max =          Z_JERK_MAX,
    .jerk_homing =       Z_JERK_HOMING,
    .junction_dev =      Z_JUNCTION_DEVIATION,
    .search_velocity =   Z_SEARCH_VELOCITY,
    .latch_velocity =    Z_LATCH_VELOCITY,
    .latch_backoff =     Z_LATCH_BACKOFF,
    .zero_backoff =      Z_ZERO_BACKOFF,
  }, {
    .axis_mode =         A_AXIS_MODE,
    .velocity_max =      A_VELOCITY_MAX,
    .feedrate_max =      A_FEEDRATE_MAX,
    .travel_min =        A_TRAVEL_MIN,
    .travel_max =        A_TRAVEL_MAX,
    .jerk_max =          A_JERK_MAX,
    .jerk_homing =       A_JERK_HOMING,
    .junction_dev =      A_JUNCTION_DEVIATION,
    .radius =            A_RADIUS,
    .search_velocity =   A_SEARCH_VELOCITY,
    .latch_velocity =    A_LATCH_VELOCITY,
    .latch_backoff =     A_LATCH_BACKOFF,
    .zero_backoff =      A_ZERO_BACKOFF,
  }, {
    .axis_mode =         B_AXIS_MODE,
    .velocity_max =      B_VELOCITY_MAX,
    .feedrate_max =      B_FEEDRATE_MAX,
    .travel_min =        B_TRAVEL_MIN,
    .travel_max =        B_TRAVEL_MAX,
    .jerk_max =          B_JERK_MAX,
    .junction_dev =      B_JUNCTION_DEVIATION,
    .radius =            B_RADIUS,
  }, {
    .axis_mode =         C_AXIS_MODE,
    .velocity_max =      C_VELOCITY_MAX,
    .feedrate_max =      C_FEEDRATE_MAX,
    .travel_min =        C_TRAVEL_MIN,
    .travel_max =        C_TRAVEL_MAX,
    .jerk_max =          C_JERK_MAX,
    .junction_dev =      C_JUNCTION_DEVIATION,
    .radius =            C_RADIUS,
  }
};


/* Jerk functions
 *
 * Jerk values can be rather large. Jerk values are stored in the system in
 * truncated format; values are divided by 1,000,000 then multiplied before use.
 *
 * The axis_jerk() functions expect the jerk in divided by 1,000,000 form.
 */

/// Returns axis jerk
float axes_get_jerk(uint8_t axis) {return axes[axis].jerk_max;}


/// Sets jerk and its reciprocal for axis
void axes_set_jerk(uint8_t axis, float jerk) {
  axes[axis].jerk_max = jerk;
  axes[axis].recip_jerk = 1 / (jerk * JERK_MULTIPLIER);
}


uint8_t get_axis_mode(int axis) {return axes[axis].axis_mode;}


void set_axis_mode(int axis, uint8_t value) {
  if (value <= AXIS_RADIUS) axes[axis].axis_mode = value;
}


float get_max_velocity(int axis) {return axes[axis].velocity_max;}
void set_max_velocity(int axis, float value) {axes[axis].velocity_max = value;}
float get_max_feedrate(int axis) {return axes[axis].feedrate_max;}
void set_max_feedrate(int axis, float value) {axes[axis].feedrate_max = value;}
float get_max_jerk(int axis) {return axes[axis].jerk_max;}
void set_max_jerk(int axis, float value) {axes[axis].jerk_max = value;}
float get_junction_dev(int axis) {return axes[axis].junction_dev;}
void set_junction_dev(int axis, float value) {axes[axis].junction_dev = value;}
float get_travel_min(int axis) {return axes[axis].travel_min;}
void set_travel_min(int axis, float value) {axes[axis].travel_min = value;}
float get_travel_max(int axis) {return axes[axis].travel_max;}
void set_travel_max(int axis, float value) {axes[axis].travel_max = value;}
float get_jerk_homing(int axis) {return axes[axis].jerk_homing;}
void set_jerk_homing(int axis, float value) {axes[axis].jerk_homing = value;}
float get_search_vel(int axis) {return axes[axis].search_velocity;}
void set_search_vel(int axis, float value) {axes[axis].search_velocity = value;}
float get_latch_vel(int axis) {return axes[axis].latch_velocity;}
void set_latch_vel(int axis, float value) {axes[axis].latch_velocity = value;}
float get_latch_backoff(int axis) {return axes[axis].latch_backoff;}


void set_latch_backoff(int axis, float value) {
  axes[axis].latch_backoff = value;
}


float get_zero_backoff(int axis) {return axes[axis].zero_backoff;}
void set_zero_backoff(int axis, float value) {axes[axis].zero_backoff = value;}
