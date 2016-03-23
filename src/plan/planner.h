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


#include "canonical_machine.h" // used for GCodeState_t
#include "util.h"
#include "config.h"

typedef enum {
  SECTION_HEAD,           // acceleration
  SECTION_BODY,           // cruise
  SECTION_TAIL,           // deceleration
  SECTIONS                // section count
} moveSection_t;

// Most of these factors are the result of a lot of tweaking.
// Change with caution.
#define JERK_MULTIPLIER         1000000.0
/// precision jerk must match to be considered same
#define JERK_MATCH_PRECISION    1000.0

#define NOM_SEGMENT_USEC        5000.0 // nominal segment time
/// minimum segment time / minimum move time
#define MIN_SEGMENT_USEC        2500.0
#define MIN_ARC_SEGMENT_USEC    10000.0 // minimum arc segment time

#define NOM_SEGMENT_TIME        (NOM_SEGMENT_USEC / MICROSECONDS_PER_MINUTE)
#define MIN_SEGMENT_TIME        (MIN_SEGMENT_USEC / MICROSECONDS_PER_MINUTE)
/// minimum time a move can be is one segment
#define MIN_TIME_MOVE           MIN_SEGMENT_TIME
/// factor for minimum size Gcode block to process
#define MIN_BLOCK_TIME          MIN_SEGMENT_TIME

#define MIN_SEGMENT_TIME_PLUS_MARGIN \
  ((MIN_SEGMENT_USEC + 1) / MICROSECONDS_PER_MINUTE)


/// common variables for planning (move master)
typedef struct mpMoveMasterSingleton {
  float position[AXES];             // final move position for planning purposes

  float jerk;                       // jerk values cached from previous block
  float recip_jerk;
  float cbrt_jerk;
} mpMoveMasterSingleton_t;


typedef struct mpMoveRuntimeSingleton { // persistent runtime variables
  uint8_t move_state;    // state of the overall move
  uint8_t section;       // what section is the move in?
  uint8_t section_state; // state within a move section

  /// unit vector for axis scaling & planning
  float unit[AXES];
  /// final target for bf (used to correct rounding errors)
  float target[AXES];
  /// current move position
  float position[AXES];
  /// for Kahan summation in _exec_aline_segment()
  float position_c[AXES];
  /// head/body/tail endpoints for correction
  float waypoint[SECTIONS][AXES];
  /// current MR target (absolute target as steps)
  float target_steps[MOTORS];
  /// current MR position (target from previous segment)
  float position_steps[MOTORS];
  /// will align with next encoder sample (target 2nd previous segment)
  float commanded_steps[MOTORS];
  /// encoder position in steps - ideally the same as commanded_steps
  float encoder_steps[MOTORS];
  /// difference between encoder & commanded steps
  float following_error[MOTORS];

  /// copies of bf variables of same name
  float head_length;
  float body_length;
  float tail_length;

  float entry_velocity;
  float cruise_velocity;
  float exit_velocity;

  float segments;                   // number of segments in line or arc
  uint32_t segment_count;           // count of running segments
  float segment_velocity;           // computed velocity for aline segment
  float segment_time;               // actual time increment per aline segment
  float jerk;                       // max linear jerk

#ifdef __JERK_EXEC // values used exclusively by computed jerk acceleration
  float jerk_div2;                  // cached value for efficiency
  float midpoint_velocity;          // velocity at accel/decel midpoint
  float midpoint_acceleration;
  float accel_time;
  float segment_accel_time;
  float elapsed_accel_time;
#else // values used exclusively by forward differencing acceleration
  float forward_diff_1;             // forward difference level 1
  float forward_diff_2;             // forward difference level 2
  float forward_diff_3;             // forward difference level 3
  float forward_diff_4;             // forward difference level 4
  float forward_diff_5;             // forward difference level 5
#ifdef __KAHAN
  float forward_diff_1_c;           // level 1 floating-point compensation
  float forward_diff_2_c;           // level 2 floating-point compensation
  float forward_diff_3_c;           // level 3 floating-point compensation
  float forward_diff_4_c;           // level 4 floating-point compensation
  float forward_diff_5_c;           // level 5 floating-point compensation
#endif
#endif

  GCodeState_t gm;                  // gcode model state currently executing
} mpMoveRuntimeSingleton_t;


// Reference global scope structures
extern mpMoveMasterSingleton_t mm;  // context for line planning
extern mpMoveRuntimeSingleton_t mr; // context for line runtime


void planner_init();
void mp_flush_planner();
void mp_set_planner_position(uint8_t axis, const float position);
void mp_set_runtime_position(uint8_t axis, const float position);
void mp_set_steps_to_runtime_position();
float mp_get_runtime_velocity();
float mp_get_runtime_work_position(uint8_t axis);
float mp_get_runtime_absolute_position(uint8_t axis);
void mp_set_runtime_work_offset(float offset[]);
void mp_zero_segment_velocity();
uint8_t mp_get_runtime_busy();
