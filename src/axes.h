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


typedef enum {
  AXIS_DISABLED,         // disabled axis
  AXIS_STANDARD,         // axis in coordinated motion w/standard behaviors
  AXIS_RADIUS,           // rotary axis calibrated to circumference
} axis_mode_t;


typedef struct {
  axis_mode_t axis_mode;
  float feedrate_max;    // max velocity in mm/min or deg/min
  float velocity_max;    // max velocity in mm/min or deg/min
  float travel_max;      // max work envelope for soft limits
  float travel_min;      // min work envelope for soft limits
  float jerk_max;        // max jerk (Jm) in mm/min^3 divided by 1 million
  float jerk_homing;     // homing jerk (Jh) in mm/min^3 divided by 1 million
  float recip_jerk;      // reciprocal of current jerk value - with million
  float junction_dev;    // aka cornering delta
  float radius;          // radius in mm for rotary axis modes
  float search_velocity; // homing search velocity
  float latch_velocity;  // homing latch velocity
  float latch_backoff;   // backoff from switches prior to homing latch movement
  float zero_backoff;    // backoff from switches for machine zero
} axis_config_t;


extern axis_config_t axes[AXES];

float axes_get_jerk(uint8_t axis);
void axes_set_jerk(uint8_t axis, float jerk);
