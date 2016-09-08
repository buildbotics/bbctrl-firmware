/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include <stdbool.h>


// Most of these factors are the result of a lot of tweaking.
// Change with caution.
#define JERK_MULTIPLIER         1000000.0
#define JERK_MATCH_PRECISION    1000.0 // jerk precision to be considered same

#define NOM_SEGMENT_USEC        5000.0  // nominal segment time
#define MIN_SEGMENT_USEC        2500.0  // minimum segment time

#define NOM_SEGMENT_TIME        (NOM_SEGMENT_USEC / MICROSECONDS_PER_MINUTE)
#define MIN_SEGMENT_TIME        (MIN_SEGMENT_USEC / MICROSECONDS_PER_MINUTE)
#define MIN_BLOCK_TIME          MIN_SEGMENT_TIME // minimum size Gcode block

#define MIN_SEGMENT_TIME_PLUS_MARGIN \
  ((MIN_SEGMENT_USEC + 1) / MICROSECONDS_PER_MINUTE)

/// Max iterations for convergence in the HT asymmetric case.
#define TRAPEZOID_ITERATION_MAX             10

/// Error percentage for iteration convergence. As percent - 0.01 = 1%
#define TRAPEZOID_ITERATION_ERROR_PERCENT   0.1

/// Tolerance for "exact fit" for H and T cases
/// allowable mm of error in planning phase
#define TRAPEZOID_LENGTH_FIT_TOLERANCE      0.0001

/// Adaptive velocity tolerance term
#define TRAPEZOID_VELOCITY_TOLERANCE        (max(2, bf->entry_velocity / 100))


typedef enum {
  SECTION_HEAD,           // acceleration
  SECTION_BODY,           // cruise
  SECTION_TAIL,           // deceleration
} move_section_t;


void planner_init();
void mp_flush_planner();
void mp_kinematics(const float travel[], float steps[]);
void mp_plan_block_list(mp_buffer_t *bf);
void mp_replan_blocks();
float mp_get_target_length(float Vi, float Vf, const mp_buffer_t *bf);
float mp_get_target_velocity(float Vi, float L, const mp_buffer_t *bf);
