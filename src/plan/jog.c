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

#include "jog.h"

#include "axes.h"
#include "planner.h"
#include "buffer.h"
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
  float next_velocity[AXES];
  float target_velocity[AXES];
  float current_velocity[AXES];
} jog_runtime_t;


static jog_runtime_t jr;


static stat_t _exec_jog(mp_buffer_t *bf) {
  // Load next velocity
  if (!jr.writing)
    for (int axis = 0; axis < AXES; axis++)
      jr.target_velocity[axis] = jr.next_velocity[axis];

  // Compute next line segment
  float target[AXES];
  const float time = MIN_SEGMENT_TIME; // In minutes
  const float max_delta_v = JOG_ACCELERATION * time;
  float velocity_sqr = 0;
  bool done = true;

  // Compute new axis velocities and target
  for (int axis = 0; axis < AXES; axis++) {
    float target_v = jr.target_velocity[axis] * axes[axis].velocity_max;
    float delta_v = target_v - jr.current_velocity[axis];
    float sign = delta_v < 0 ? -1 : 1;

    if (max_delta_v < fabs(delta_v))
      jr.current_velocity[axis] += max_delta_v * sign;
    else jr.current_velocity[axis] = target_v;

    if (!fp_ZERO(jr.current_velocity[axis])) done = false;

    // Compute target from axis velocity
    target[axis] =
      mp_runtime_get_axis_position(axis) + time * jr.current_velocity[axis];

    // Accumulate velocities squared
    velocity_sqr += square(jr.current_velocity[axis]);
  }

  // Check if we are done
  if (done) {
    // Update machine position
    for (int axis = 0; axis < AXES; axis++)
      mach_set_axis_position(axis, mp_runtime_get_work_position(axis));

    return STAT_NOOP; // Done, no move executed
  }

  mp_runtime_set_velocity(sqrt(velocity_sqr));
  stat_t status = mp_runtime_move_to_target(target, time);
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
