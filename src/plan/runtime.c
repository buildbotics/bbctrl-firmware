/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2012 - 2015 Rob Giseburt
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

#include "planner.h"
#include "buffer.h"
#include "stepper.h"
#include "motor.h"
#include "util.h"
#include "report.h"
#include "state.h"
#include "config.h"

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>


typedef struct {
  float target[MOTORS];    // Current absolute target in steps
  float position[MOTORS];  // Current position, target from previous segment
  float commanded[MOTORS]; // Should align with next encoder sample (2nd prev)
  float encoder[MOTORS];   // Encoder steps, ideally the same as commanded steps
  float error[MOTORS];     // Difference between encoder & commanded steps
} mp_steps_t;


typedef struct {
  int32_t line;            // Current move GCode line number
  uint8_t tool;

  float feed;
  feed_rate_mode_t feed_rate_mode;
  float feed_override;

  float speed;
  spindle_mode_t spindle_mode;
  float spindle_override;

  motion_mode_t motion_mode;
  plane_t plane;
  units_mode_t units_mode;
  coord_system_t coord_system;
  bool absolute_mode;
  path_mode_t path_control;
  distance_mode_t distance_mode;
  distance_mode_t arc_distance_mode;

  bool mist_coolant;
  bool flood_coolant;

} mach_state_t;


typedef struct {
  bool busy;               // True if a move is running
  float position[AXES];    // Current move position
  float work_offset[AXES]; // Current move work offset
  float velocity;          // Current move velocity
  mp_steps_t steps;
  mach_state_t mach;
} mp_runtime_t;

static mp_runtime_t rt;


bool mp_runtime_is_busy() {return rt.busy;}
void mp_runtime_set_busy(bool busy) {rt.busy = busy;}
int32_t mp_runtime_get_line() {return rt.mach.line;}


void mp_runtime_set_line(int32_t line) {
  rt.mach.line = line;
  report_request();
}


/// Returns current segment velocity
float mp_runtime_get_velocity() {return rt.velocity;}


void mp_runtime_set_velocity(float velocity) {
  rt.velocity = velocity;
  report_request();
}


/// Set encoder counts to the runtime position
void mp_runtime_set_steps_from_position() {
  // Convert lengths to steps in floating point
  float step_position[MOTORS];
  mp_kinematics(rt.position, step_position);

  for (int motor = 0; motor < MOTORS; motor++) {
    rt.steps.target[motor] = rt.steps.position[motor] =
      rt.steps.commanded[motor] = step_position[motor];

    // Write steps to encoder register
    motor_set_encoder(motor, step_position[motor]);

    // Must be zero
    rt.steps.error[motor] = 0;
  }
}


/* Since steps are in motor space you have to run the position vector
 * through inverse kinematics to get the right numbers.  This means
 * that in a non-Cartesian robot changing any position can result in
 * changes to multiple step values.  So this operation is provided as a
 * single function and always uses the new position vector as an
 * input.
 *
 * Keeping track of position is complicated by the fact that moves
 * exist in several reference frames.  The scheme to keep this
 * straight is:
 *
 *   - mm.position    - start and end position for planning
 *   - rt.position    - current position of runtime segment
 *   - rt.steps.*     - position in steps
 *
 * Note that position is set immediately when called and may not be
 * an accurate representation of the tool position.  The motors
 * are still processing the action and the real tool position is
 * still close to the starting point.
 */


/// Set runtime position for a single axis
void mp_runtime_set_axis_position(uint8_t axis, const float position) {
  rt.position[axis] = position;
  report_request();
}


/// Returns current axis position in machine coordinates
float mp_runtime_get_axis_position(uint8_t axis) {return rt.position[axis];}
float *mp_runtime_get_position() {return rt.position;}


void mp_runtime_set_position(float position[]) {
  copy_vector(rt.position, position);
  report_request();
}


/// Returns axis position in work coordinates that were in effect at plan time
float mp_runtime_get_work_position(uint8_t axis) {
  return rt.position[axis] - rt.work_offset[axis];
}


/// Set offsets
void mp_runtime_set_work_offsets(float offset[]) {
  copy_vector(rt.work_offset, offset);
}


/*** Segment runner
 *
 * Notes on step error correction:
 *
 * The commanded_steps are the target_steps delayed by one more
 * segment.  This lines them up in time with the encoder readings so a
 * following error can be generated
 *
 * The rt.steps.error term is positive if the encoder reading is
 * greater than (ahead of) the commanded steps, and negative (behind)
 * if the encoder reading is less than the commanded steps. The
 * following error is not affected by the direction of movement - it's
 * purely a statement of relative position. Examples:
 *
 *    Encoder Commanded   Following Err
 *     100      90        +10    encoder is 10 steps ahead of commanded steps
 *     -90    -100        +10    encoder is 10 steps ahead of commanded steps
 *      90     100        -10    encoder is 10 steps behind commanded steps
 *    -100     -90        -10    encoder is 10 steps behind commanded steps
 */
stat_t mp_runtime_move_to_target(float target[], float time) {
  // Bucket-brigade old target down the chain before getting new target
  for (int motor = 0; motor < MOTORS; motor++) {
    // previous segment's position, delayed by 1 segment
    rt.steps.commanded[motor] = rt.steps.position[motor];
    // previous segment's target becomes position
    rt.steps.position[motor] = rt.steps.target[motor];
    // current encoder position (aligns to commanded_steps)
    rt.steps.encoder[motor] = motor_get_encoder(motor);
    rt.steps.error[motor] = rt.steps.encoder[motor] - rt.steps.commanded[motor];
  }

  // Convert target position to steps.
  mp_kinematics(target, rt.steps.target);

  // Compute distances in steps to be traveled.
  //
  // The direct manipulation of steps to compute travel_steps only
  // works for Cartesian kinematics.  Other kinematics may require
  // transforming travel distance as opposed to simply subtracting steps.
  float travel_steps[MOTORS];
  for (int motor = 0; motor < MOTORS; motor++)
    travel_steps[motor] = rt.steps.target[motor] - rt.steps.position[motor];

  // Call the stepper prep function
  RITORNO(st_prep_line(travel_steps, rt.steps.error, time));

  // Update position
  mp_runtime_set_position(target);

  return STAT_OK;
}
