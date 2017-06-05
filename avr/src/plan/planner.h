/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                  Copyright (c) 2013 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2013 - 2015 Robert Giseburt
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


#include "machine.h" // used for gcode_state_t
#include "buffer.h"
#include "util.h"
#include "config.h"

#include <stdbool.h>


// Most of these factors are the result of a lot of tweaking.
// Change with caution.
#define JERK_MULTIPLIER         1000000.0
#define JERK_MATCH_PRECISION    1000.0 // jerk precision to be considered same

/// Error percentage for iteration convergence. As percent - 0.01 = 1%
#define TRAPEZOID_ITERATION_ERROR_PERCENT   0.1

/// Adaptive velocity tolerance term
#define TRAPEZOID_VELOCITY_TOLERANCE        (max(2, bf->entry_velocity / 100))

/*** If the absolute value of the remaining deceleration length would be less
 * than this value then finish the deceleration in the current move.  This is
 * used to avoid creating segements before or after the hold which are too
 * short to process correctly.
 */
#define HOLD_DECELERATION_TOLERANCE 1 // In mm
#define HOLD_VELOCITY_TOLERANCE 60 // In mm/min


typedef enum {
  SECTION_HEAD,           // acceleration
  SECTION_BODY,           // cruise
  SECTION_TAIL,           // deceleration
} move_section_t;


void mp_init();

void mp_set_axis_position(int axis, float position);
float mp_get_axis_position(int axis);
void mp_set_position(const float p[]);
void mp_set_plan_steps(bool plan_steps);

void mp_flush_planner();
void mp_kinematics(const float travel[], float steps[]);

void mp_plan(mp_buffer_t *bf);
void mp_replan_all();

void mp_queue_push_nonstop(buffer_cb_t cb, uint32_t line);

float mp_get_target_length(float Vi, float Vf, float jerk);
float mp_get_target_velocity(float Vi, float L, const mp_buffer_t *bf);
