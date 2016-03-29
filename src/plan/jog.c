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
#include "stepper.h"
#include "motor.h"
#include "canonical_machine.h"
#include "motor.h"

#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdio.h>


typedef struct {
  bool running;
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
  float travel[AXES]; // In mm
  float time = MIN_SEGMENT_TIME; // In minutes
  float maxDeltaV = JOG_ACCELERATION * time;
  bool done = true;

  // Compute new velocities and travel
  for (int axis = 0; axis < AXES; axis++) {
    float targetV = jr.target_velocity[axis] * cm.a[axis].velocity_max;
    float deltaV = targetV - jr.current_velocity[axis];
    float sign = deltaV < 0 ? -1 : 1;

    if (maxDeltaV < fabs(deltaV)) jr.current_velocity[axis] += maxDeltaV * sign;
    else jr.current_velocity[axis] = targetV;

    // Compute travel from velocity
    travel[axis] = time * jr.current_velocity[axis];

    if (travel[axis]) done = false;
  }

  // Check if we are done
  if (done) {
    // Update machine position
    for (int motor = 0; motor < MOTORS; motor++) {
      int axis = motor_get_axis(motor);
      float steps = motor_get_encoder(axis);
      cm_set_position(axis, steps / motor_get_steps_per_unit(motor));
    }

    // Release queue
    mp_free_run_buffer();

    jr.running = false;

    return STAT_NOOP;
  }

  // Convert to steps
  float steps[MOTORS] = {0};
  mp_kinematics(travel, steps);

  // Queue segment
  float error[MOTORS] = {0};
  st_prep_line(steps, error, time);

  return STAT_OK;
}


void mp_jog(float velocity[AXES]) {
  // Reset
  if (!jr.running) memset(&jr, 0, sizeof(jr));

  jr.writing = true;
  for (int axis = 0; axis < AXES; axis++)
    jr.next_velocity[axis] = velocity[axis];
  jr.writing = false;

  if (!jr.running) {
    // Should always be at least one free buffer
    mpBuf_t *bf = mp_get_write_buffer();
    if (!bf) {
      cm_hard_alarm(STAT_BUFFER_FULL_FATAL);
      return;
    }

    // Start
    jr.running = true;
    bf->bf_func = _exec_jog; // register callback
    mp_commit_write_buffer(MOVE_TYPE_JOG);
  }
}


bool mp_jog_busy() {
  return jr.running;
}
