/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "jog.h"

#include "axis.h"
#include "planner.h"
#include "buffer.h"
#include "line.h"
#include "velocity_curve.h"
#include "runtime.h"
#include "machine.h"
#include "state.h"
#include "config.h"

#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  bool writing;
  float Vi;
  float Vt;
  float velocity_delta[AXES];
  float velocity_t[AXES];

  int sign[AXES];
  float velocity[AXES];
  float next_velocity[AXES];
  float initial_velocity[AXES];
  float target_velocity[AXES];
} jog_runtime_t;


static jog_runtime_t jr;


static stat_t _exec_jog(mp_buffer_t *bf) {
  // Load next velocity
  bool changed = false;
  bool done = true;
  if (!jr.writing)
    for (int axis = 0; axis < AXES; axis++) {
      float Vn = jr.next_velocity[axis] * axis_get_velocity_max(axis);
      float Vi = jr.velocity[axis];
      float Vt = jr.target_velocity[axis];

      if (JOG_MIN_VELOCITY < fabs(Vn)) done = false;

      if (!fp_ZERO(Vi) && (Vn < 0) != (Vi < 0))
        Vn = 0; // Plan to zero on sign change

      if (fabs(Vn) < JOG_MIN_VELOCITY) Vn = 0;

      if (Vt != Vn) {
        jr.target_velocity[axis] = Vn;
        if (Vn) jr.sign[axis] = Vn < 0 ? -1 : 1;
        changed = true;
      }
    }

  float velocity_sqr = 0;

  // Compute per axis velocities
  for (int axis = 0; axis < AXES; axis++) {
    float V = fabs(jr.velocity[axis]);
    float Vt = fabs(jr.target_velocity[axis]);

    if (changed) {
      if (fp_EQ(V, Vt)) {
        V = Vt;
        jr.velocity_t[axis] = 1;

      } else {
        // Compute axis max jerk
        float jerk = axis_get_jerk_max(axis) * JERK_MULTIPLIER;

        // Compute length to velocity given max jerk
        float length = mp_get_target_length(V, Vt, jerk * JOG_JERK_MULT);

        // Compute move time
        float move_time = 2 * length / (V + Vt);

        if (move_time < SEGMENT_TIME) {
          V = Vt;
          jr.velocity_t[axis] = 1;

        } else {
          jr.initial_velocity[axis] = V;
          jr.velocity_t[axis] = jr.velocity_delta[axis] =
            SEGMENT_TIME / move_time;
        }
      }
    }

    if (jr.velocity_t[axis] < 1) {
      // Compute quintic Bezier curve
      V = velocity_curve(jr.initial_velocity[axis], Vt, jr.velocity_t[axis]);
      jr.velocity_t[axis] += jr.velocity_delta[axis];

    } else V = Vt;

    if (JOG_MIN_VELOCITY < V || JOG_MIN_VELOCITY < Vt) done = false;

    velocity_sqr += square(V);
    jr.velocity[axis] = V * jr.sign[axis];
  }

  // Check if we are done
  if (done) {
    // Update machine position
    for (int axis = 0; axis < AXES; axis++)
      mach_set_axis_position(axis, mp_runtime_get_work_position(axis));

    mp_set_cycle(CYCLE_MACHINING); // Default cycle

    return STAT_NOOP; // Done, no move executed
  }

  // Compute target from velocity
  float target[AXES];
  for (int axis = 0; axis < AXES; axis++)
    target[axis] = mp_runtime_get_axis_position(axis) +
      jr.velocity[axis] * SEGMENT_TIME;

  // Set velocity and target
  mp_runtime_set_velocity(sqrt(velocity_sqr));
  stat_t status = mp_runtime_move_to_target(target);
  if (status != STAT_OK) return status;

  return STAT_EAGAIN;
}


bool mp_jog_busy() {return mp_get_cycle() == CYCLE_JOGGING;}


uint8_t command_jog(int argc, char *argv[]) {
  if (!mp_jog_busy() &&
      (mp_get_state() != STATE_READY || mp_get_cycle() != CYCLE_MACHINING))
    return STAT_NOOP;

  float velocity[AXES];

  for (int axis = 0; axis < AXES; axis++)
    if (axis < argc - 1) velocity[axis] = atof(argv[axis + 1]);
    else velocity[axis] = 0;

  // Reset
  if (!mp_jog_busy()) memset(&jr, 0, sizeof(jr));

  jr.writing = true;
  for (int axis = 0; axis < AXES; axis++)
    jr.next_velocity[axis] = velocity[axis];
  jr.writing = false;

  if (!mp_jog_busy()) {
    mp_set_cycle(CYCLE_JOGGING);
    mp_queue_push_nonstop(_exec_jog, -1);
  }

  return STAT_OK;
}
