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
  float delta;
  float t;
  bool changed;

  int sign;
  float velocity;
  float next;
  float initial;
  float target;
} jog_axis_t;


typedef struct {
  bool writing;
  bool done;

  float Vi;
  float Vt;

  jog_axis_t axes[AXES];
} jog_runtime_t;


static jog_runtime_t jr;


static bool _next_axis_velocity(int axis) {
  jog_axis_t *a = &jr.axes[axis];

  float Vn = a->next * axis_get_velocity_max(axis);
  float Vi = a->velocity;
  float Vt = a->target;

  if (JOG_MIN_VELOCITY < fabs(Vn)) jr.done = false;

  if (!fp_ZERO(Vi) && (Vn < 0) != (Vi < 0))
    Vn = 0; // Plan to zero on sign change

  if (fabs(Vn) < JOG_MIN_VELOCITY) Vn = 0;

  if (Vt == Vn) return false; // No change

  a->target = Vn;
  if (Vn) a->sign = Vn < 0 ? -1 : 1;

  return true;
}


static float _compute_axis_velocity(int axis) {
  jog_axis_t *a = &jr.axes[axis];

  float V = fabs(a->velocity);
  float Vt = fabs(a->target);

  if (JOG_MIN_VELOCITY < Vt) jr.done = false;

  if (fp_EQ(V, Vt)) return Vt;

  if (a->changed) {
    // Compute axis max jerk
    float jerk = axis_get_jerk_max(axis) * JERK_MULTIPLIER;

    // Compute length to velocity given max jerk
    float length = mp_get_target_length(V, Vt, jerk * JOG_JERK_MULT);

    // Compute move time
    float move_time = 2 * length / (V + Vt);

    if (move_time <= SEGMENT_TIME) return Vt;

    a->initial = V;
    a->t = a->delta = SEGMENT_TIME / move_time;
  }

  if (a->t <= 0) return V;
  if (1 <= a->t) return Vt;

  // Compute quintic Bezier curve
  V = velocity_curve(a->initial, Vt, a->t);
  a->t += a->delta;

  return V;
}


#if 0
static float _axis_velocity_at_limits(int axis) {
  float V = jr.axes[axis].velocity;

  if (axis_get_homed(axis)) {
    float min = axis_get_travel_min(axis);
    float max = axis_get_travel_max(axis);

    if (position <= min) return 0;
    if (max <= position) return 0;

    float position = mp_runtime_get_axis_position(axis);
    float jerk = axis_get_jerk_max(axis) * JERK_MULTIPLIER;
    float decelDist = mp_get_target_length(V, 0, jerk * JOG_JERK_MULT);

    if (position < min + decelDist) {
    }

    if (max - decelDist < position) {
    }
  }

  return V;
}
#endif


static stat_t _exec_jog(mp_buffer_t *bf) {
  // Load next velocity
  jr.done = true;

  if (!jr.writing)
    for (int axis = 0; axis < AXES; axis++) {
      if (!axis_is_enabled(axis)) continue;
      jr.axes[axis].changed = _next_axis_velocity(axis);
    }

  float velocity_sqr = 0;

  // Compute per axis velocities
  for (int axis = 0; axis < AXES; axis++) {
    if (!axis_is_enabled(axis)) continue;
    float V = _compute_axis_velocity(axis);
    velocity_sqr += square(V);
    jr.axes[axis].velocity = V * jr.axes[axis].sign;
    if (JOG_MIN_VELOCITY < V) jr.done = false;
  }

  // Check if we are done
  if (jr.done) {
    // Update machine position
    mach_set_position_from_runtime();
    mp_set_cycle(CYCLE_MACHINING); // Default cycle
    mp_pause_queue(false);

    return STAT_NOOP; // Done, no move executed
  }

  // Compute target from velocity
  float target[AXES];
  for (int axis = 0; axis < AXES; axis++)
    target[axis] = mp_runtime_get_axis_position(axis) +
      jr.axes[axis].velocity * SEGMENT_TIME;

  // Set velocity and target
  mp_runtime_set_velocity(sqrt(velocity_sqr));
  stat_t status = mp_runtime_move_to_target(SEGMENT_TIME, target);
  if (status != STAT_OK) return status;

  return STAT_EAGAIN;
}


uint8_t command_jog(int argc, char *argv[]) {
  if (mp_get_cycle() != CYCLE_JOGGING &&
      (mp_get_state() != STATE_READY || mp_get_cycle() != CYCLE_MACHINING))
    return STAT_NOOP;

  float velocity[AXES];

  for (int axis = 0; axis < AXES; axis++)
    if (axis < argc - 1) velocity[axis] = atof(argv[axis + 1]);
    else velocity[axis] = 0;

  // Reset
  if (mp_get_cycle() != CYCLE_JOGGING) memset(&jr, 0, sizeof(jr));

  jr.writing = true;
  for (int axis = 0; axis < AXES; axis++)
    jr.axes[axis].next = velocity[axis];
  jr.writing = false;

  if (mp_get_cycle() != CYCLE_JOGGING) {
    mp_set_cycle(CYCLE_JOGGING);
    mp_pause_queue(true);
    mp_queue_push_nonstop(_exec_jog, -1);
  }

  return STAT_OK;
}
