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
} jogRuntime_t;


static jogRuntime_t jr;


static stat_t _exec_jog(mpBuf_t *bf) {
  if (bf->move_state == MOVE_OFF) return STAT_NOOP;
  if (bf->move_state == MOVE_NEW) bf->move_state = MOVE_RUN;

  // Load next velocity
  if (!jr.writing)
    for (int axis = 0; axis < AXES; axis++)
      jr.target_velocity[axis] = jr.next_velocity[axis];

  // Compute next line segment
  float target[AXES];
  const float time = MIN_SEGMENT_TIME; // In minutes
  const float maxDeltaV = JOG_ACCELERATION * time;
  float velocitySqr = 0;
  bool done = true;

  // Compute new axis velocities and target
  for (int axis = 0; axis < AXES; axis++) {
    float targetV = jr.target_velocity[axis] * mach.a[axis].velocity_max;
    float deltaV = targetV - jr.current_velocity[axis];
    float sign = deltaV < 0 ? -1 : 1;

    if (maxDeltaV < fabs(deltaV)) jr.current_velocity[axis] += maxDeltaV * sign;
    else jr.current_velocity[axis] = targetV;

    if (!fp_ZERO(jr.current_velocity[axis])) done = false;

    // Compute target from axis velocity
    target[axis] =
      mp_runtime_get_position(axis) + time * jr.current_velocity[axis];

    // Accumulate velocities squared
    velocitySqr += square(jr.current_velocity[axis]);
  }

  // Check if we are done
  if (done) {
    // Update machine position
    for (int axis = 0; axis < AXES; axis++)
      mach_set_position(axis, mp_runtime_get_work_position(axis));

    // Release buffer
    mp_free_run_buffer();

    mp_set_cycle(CYCLE_MACHINING);

    return STAT_NOOP;
  }

  return mp_runtime_move_to_target(target, sqrt(velocitySqr), time);
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
    // Should always be at least one free buffer
    mpBuf_t *bf = mp_get_write_buffer();
    if (!bf) return STAT_BUFFER_FULL_FATAL;

    // Start
    mp_set_cycle(CYCLE_JOGGING);
    bf->bf_func = _exec_jog; // register callback
    mp_commit_write_buffer(-1, MOVE_TYPE_COMMAND);
  }

  return STAT_OK;
}
