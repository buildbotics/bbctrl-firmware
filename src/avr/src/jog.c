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

#include "jog.h"

#include "axis.h"
#include "util.h"
#include "exec.h"
#include "state.h"
#include "scurve.h"
#include "command.h"
#include "config.h"

#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  float delta;
  float t;
  bool changed;

  int sign;
  float velocity;
  float accel;
  float next;
  float initial;
  float target;
} jog_axis_t;


typedef struct {
  bool writing;
  bool done;

  jog_axis_t axes[AXES];
} jog_runtime_t;


static jog_runtime_t jr;


static bool _axis_velocity_target(int axis) {
  jog_axis_t *a = &jr.axes[axis];

  float Vn = a->next * axis_get_velocity_max(axis);
  float Vi = a->velocity;
  float Vt = a->target;

  if (MIN_VELOCITY < fabs(Vn)) jr.done = false; // Still jogging

  if (!fp_ZERO(Vi) && (Vn < 0) != (Vi < 0))
    Vn = 0; // Plan to zero on sign change

  if (fabs(Vn) < MIN_VELOCITY) Vn = 0;

  if (Vt == Vn) return false; // No change

  a->target = Vn;
  if (Vn) a->sign = Vn < 0 ? -1 : 1;

  return true; // Velocity changed
}


#if 0
// Numeric version
static float _compute_deccel_dist(float vel, float accel, float jerk) {
  float dist = 0;
  float Ad = jerk * SEGMENT_TIME; // Delta accel

  while (true) {
    // Compute next accel
    float At2 = -jerk * vel;
    if (accel * accel < At2) accel += Ad;
    else accel -= Ad;

    // Compute next velocity
    vel += accel * SEGMENT_TIME;
    if (vel <= 0) break;

    // Compute distance traveled
    dist += vel * SEGMENT_TIME;
  }

  return dist;
}


// Analytical version
static float _compute_deccel_dist(float vel, float accel, float jerk) {
  float dist = 0;

  // Compute distance to decrease accel to zero
  if (0 < accel) {
    // s(a/j) = v * a / j + 2 * a^3 / (3 * j^2)
    dist += vel * accel / jerk + 2 * accel * accel * accel / (3 * jerk * jerk);
    // v(a/j) = a^2 / 2j + v
    vel += accel * accel / (2 * jerk);
    accel = 0;
  }

  // Compute max deccel given accel, vel and jerk
  float maxDeccel = -sqrt(0.5 * accel * accel + vel * jerk);

  // Compute distance to max deccel
  if (maxDeccel < accel) {
    float t = (maxDeccel - accel) / -jerk;
    dist += vel * t + accel * t * t / 2 - jerk * t * t * t / 6;
    vel += accel * t - jerk * t * t / 2;
    accel = maxDeccel;
  }

  // Compute distance to zero vel
  float t = -accel / jerk;
  dist += vel * t + accel * t * t / 2 + jerk * t * t * t / 6;

  return dist;
}
#endif


#if 0
static bool _soft_limit(int axis, float V, float A) {
  jog_axis_t *a = &jr.axes[axis];

  // Check if axis is homed
  if (!axis_get_homed(axis)) return false;

  // Check if limits are enabled
  float min = axis_get_travel_min(axis);
  float max = axis_get_travel_max(axis);
  if (min == max) return false;

  // Check if we need to stop to avoid exceeding a limit
  float jerk = axis_get_jerk_max(axis);
  float deccelDist = _compute_deccel_dist(V, A, jerk);

  float position = exec_get_axis_position(axis);
  if (a->velocity < 0 && position <= min + deccelDist) return true;
  if (0 < a->velocity && max - deccelDist <= position) return true;

  return false;
}
#endif


static float _compute_axis_velocity(int axis) {
  jog_axis_t *a = &jr.axes[axis];

  float V = fabs(a->velocity);
  float Vt = fabs(a->target);

  // Apply soft limits
  //if (_soft_limit(axis, V, a->accel)) Vt = MIN_VELOCITY;

  // Check if velocity has reached its target
  if (fp_EQ(V, Vt)) {
    a->accel = 0;
    return Vt;
  }

  // Compute axis max jerk
  float jerk = axis_get_jerk_max(axis);

  // Compute next accel
  a->accel = scurve_next_accel(SEGMENT_TIME, V, Vt, a->accel, jerk);

  // Limit acceleration
  if (axis_get_accel_max(axis) < fabs(a->accel))
    a->accel = (a->accel < 0 ? -1 : 1) * axis_get_accel_max(axis);

  return V + a->accel * SEGMENT_TIME;
}


stat_t jog_exec() {
  // Load next velocity
  jr.done = true;

  if (!jr.writing)
    for (int axis = 0; axis < AXES; axis++) {
      if (!axis_is_enabled(axis)) continue;
      jr.axes[axis].changed = _axis_velocity_target(axis);
    }

  float velocity_sqr = 0;

  // Compute per axis velocities
  for (int axis = 0; axis < AXES; axis++) {
    if (!axis_is_enabled(axis)) continue;
    float V = _compute_axis_velocity(axis);
    if (MIN_VELOCITY < V) jr.done = false;
    velocity_sqr += square(V);
    jr.axes[axis].velocity = V * jr.axes[axis].sign;
  }

  // Check if we are done
  if (jr.done) {
    command_reset_position();
    exec_set_velocity(0);
    exec_set_cb(0);
    state_idle();

    return STAT_NOP; // Done, no move executed
  }

  // Compute target from velocity
  float target[AXES];
  exec_get_position(target);
  for (int axis = 0; axis < AXES; axis++)
    target[axis] += jr.axes[axis].velocity * SEGMENT_TIME;

  // Set velocity and target
  exec_set_velocity(sqrt(velocity_sqr));
  stat_t status = exec_move_to_target(SEGMENT_TIME, target);
  if (status != STAT_OK) return status;

  return STAT_OK;
}



stat_t command_jog(char *cmd) {
  // Ignore jog commands when not already idle
  if (state_get() != STATE_READY && state_get() != STATE_JOGGING)
    return STAT_NOP;

  // Skip command code
  cmd++;

  // Get velocities
  float velocity[AXES] = {0,};
  stat_t status = decode_axes(&cmd, velocity);
  if (status) return status;

  // Check for end of command
  if (*cmd) return STAT_INVALID_ARGUMENTS;

  // Reset
  if (state_get() != STATE_JOGGING) memset(&jr, 0, sizeof(jr));

  jr.writing = true;
  for (int axis = 0; axis < AXES; axis++)
    jr.axes[axis].next = velocity[axis];
  jr.writing = false;

  if (state_get() != STATE_JOGGING) {
    state_jogging();
    exec_set_cb(jog_exec);
  }

  return STAT_OK;
}
