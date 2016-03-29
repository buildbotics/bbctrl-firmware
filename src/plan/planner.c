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

/* Planner Notes
 *
 * The planner works below the canonical machine and above the
 * motor mapping and stepper execution layers. A rudimentary
 * multitasking capability is implemented for long-running commands
 * such as lines, arcs, and dwells.  These functions are coded as
 * non-blocking continuations - which are simple state machines
 * that are re-entered multiple times until a particular operation
 * is complete. These functions have 2 parts - the initial call,
 * which sets up the local context, and callbacks (continuations)
 * that are called from the main loop (in controller.c).
 *
 * One important concept is isolation of the three layers of the
 * data model - the Gcode model (gm), planner model (bf queue &
 * mm), and runtime model (mr).  These are designated as "model",
 * "planner" and "runtime" in function names.
 *
 * The Gcode model is owned by the canonical machine and should
 * only be accessed by cm_xxxx() functions. Data from the Gcode
 * model is transferred to the planner by the mp_xxx() functions
 * called by the canonical machine.
 *
 * The planner should only use data in the planner model. When a
 * move (block) is ready for execution the planner data is
 * transferred to the runtime model, which should also be isolated.
 *
 * Lower-level models should never use data from upper-level models
 * as the data may have changed and lead to unpredictable results.
 */

#include "planner.h"

#include "buffer.h"
#include "arc.h"
#include "canonical_machine.h"
#include "kinematics.h"
#include "stepper.h"
#include "motor.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>


mpMoveRuntimeSingleton_t mr = {};    // context for line runtime


void planner_init() {
  mp_init_buffers();
}


/* Flush all moves in the planner and all arcs
 *
 * Does not affect the move currently running in mr.  Does not affect
 * mm or gm model positions.  This function is designed to be called
 * during a hold to reset the planner.  This function should not
 * generally be called; call cm_queue_flush() instead.
 */
void mp_flush_planner() {
  cm_abort_arc();
  mp_init_buffers();
  cm_set_motion_state(MOTION_STOP);
}


/* Since steps are in motor space you have to run the position vector
 * through inverse kinematics to get the right numbers. This means
 * that in a non-Cartesian robot changing any position can result in
 * changes to multiple step values. So this operation is provided as a
 * single function and always uses the new position vector as an
 * input.
 *
 * Keeping track of position is complicated by the fact that moves
 * exist in several reference frames. The scheme to keep this
 * straight is:
 *
 *   - mm.position    - start and end position for planning
 *   - mr.position    - current position of runtime segment
 *   - mr.ms.target   - target position of runtime segment
 *   - mr.endpoint    - final target position of runtime segment
 *
 * Note that position is set immediately when called and may not be
 * not an accurate representation of the tool position. The motors
 * are still processing the action and the real tool position is
 * still close to the starting point.
 */


/// Set runtime position for a single axis
void mp_set_runtime_position(uint8_t axis, const float position) {
  mr.position[axis] = position;
}


/// Set encoder counts to the runtime position
void mp_set_steps_to_runtime_position() {
  float step_position[MOTORS];

  // convert lengths to steps in floating point
  ik_kinematics(mr.position, step_position);

  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    mr.target_steps[motor] = step_position[motor];
    mr.position_steps[motor] = step_position[motor];
    mr.commanded_steps[motor] = step_position[motor];

    // write steps to encoder register
    motor_set_encoder(motor, step_position[motor]);

    // must be zero
    mr.following_error[motor] = 0;
  }
}


/// Correct velocity in last segment for reporting purposes
void mp_zero_segment_velocity() {mr.segment_velocity = 0;}


/// Returns current velocity (aggregate)
float mp_get_runtime_velocity() {return mr.segment_velocity;}


/// Returns current axis position in machine coordinates
float mp_get_runtime_absolute_position(uint8_t axis) {return mr.position[axis];}


/// Set offsets in the MR struct
void mp_set_runtime_work_offset(float offset[]) {
  copy_vector(mr.ms.work_offset, offset);
}


/// Returns current axis position in work coordinates
/// that were in effect at move planning time
float mp_get_runtime_work_position(uint8_t axis) {
  return mr.position[axis] - mr.ms.work_offset[axis];
}


/// Return TRUE if motion control busy (i.e. robot is moving)
/// Use this function to sync to the queue. If you wait until it returns
/// FALSE you know the queue is empty and the motors have stopped.
uint8_t mp_get_runtime_busy() {
  return st_runtime_isbusy() || mr.move_state == MOVE_RUN;
}
