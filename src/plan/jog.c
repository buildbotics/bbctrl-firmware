#include "jog.h"
#include "planner.h"
#include "stepper.h"
#include "canonical_machine.h"
#include "kinematics.h"
#include "encoder.h"

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
      int axis = st_cfg.mot[motor].motor_map;
      float steps = en_read_encoder(axis);
      cm_set_position(axis, steps / st_cfg.mot[motor].steps_per_unit);
    }

    // Release queue
    mp_free_run_buffer();

    jr.running = false;

    return STAT_NOOP;
  }

  // Convert to steps
  float steps[MOTORS] = {0};
  ik_kinematics(travel, steps);

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
