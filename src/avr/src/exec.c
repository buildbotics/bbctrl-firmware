/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#include "exec.h"

#include "stepper.h"
#include "motor.h"
#include "axis.h"
#include "util.h"
#include "command.h"
#include "seek.h"
#include "estop.h"
#include "state.h"
#include "spindle.h"
#include "config.h"
#include "SCurve.h"


static struct {
  exec_cb_t cb;

  float position[AXES];
  float velocity;
  float accel;
  float jerk;
  float peak_vel;
  float peak_accel;

  float feed_override;

  struct {
    float target[AXES];
    float time;
    float vel;
    float accel;
    float max_accel;
    float max_jerk;
    power_update_t power_updates[2 * POWER_MAX_UPDATES];
    exec_cb_t cb;
  } seg;
} ex;


void exec_init() {
  memset(&ex, 0, sizeof(ex));
  ex.feed_override = 1;
}


void exec_get_position(float p[AXES]) {
  memcpy(p, ex.position, sizeof(ex.position));
}


float exec_get_axis_position(int axis) {return ex.position[axis];}


void exec_set_velocity(float v) {
  ex.velocity = v;
  if (ex.peak_vel < v) ex.peak_vel = v;
}


float exec_get_velocity() {return ex.velocity;}


void exec_set_acceleration(float a) {
  ex.accel = a;
  if (ex.peak_accel < a) ex.peak_accel = a;
}


float exec_get_acceleration() {return ex.accel;}
void exec_set_jerk(float j) {ex.jerk = j;}


void exec_set_cb(exec_cb_t cb) {ex.cb = cb;}


void exec_move_to_target(const float target[]) {
  ESTOP_ASSERT(isfinite(target[AXIS_X]) && isfinite(target[AXIS_Y]) &&
               isfinite(target[AXIS_Z]) && isfinite(target[AXIS_A]) &&
               isfinite(target[AXIS_B]) && isfinite(target[AXIS_C]),
               STAT_BAD_FLOAT);

  // Prep power updates
  st_prep_power(ex.seg.power_updates);

  // Shift power updates
  for (unsigned i = 0; i < POWER_MAX_UPDATES; i++) {
    ex.seg.power_updates[i] = ex.seg.power_updates[i + POWER_MAX_UPDATES];
    ex.seg.power_updates[i + POWER_MAX_UPDATES].state = POWER_IGNORE;
  }

  // Update position
  copy_vector(ex.position, target);

  // Call the stepper prep function
  st_prep_line(target);
}


stat_t _segment_exec() {
  float t = ex.seg.time;
  float v = ex.seg.vel;
  float a = ex.seg.accel;

  // Handle pause
  if (state_get() == STATE_STOPPING) {
    a = SCurve::nextAccel(SEGMENT_TIME, 0, ex.velocity, ex.accel,
                          ex.seg.max_accel, ex.seg.max_jerk);
    v = ex.velocity + SEGMENT_TIME * a;
    t *= ex.seg.vel / v;

    if (v < MIN_VELOCITY) {
      t = v = 0;
      ex.seg.cb = 0;
      command_reset_position();
      seek_end(); // Must be before state_holding()
      state_holding();
      spindle_update_speed();
    }
  }

  // Wait for next seg if time is too short and we are still moving
  if (t < SEGMENT_TIME && (!t || v)) {
    if (!v) {
      exec_set_velocity(0);
      exec_set_acceleration(0);
      exec_set_jerk(0);
      ex.seg.time = 0;
    }
    ex.cb = ex.seg.cb;
    return STAT_AGAIN;
  }

  // Update velocity and accel
  exec_set_velocity(v);
  exec_set_acceleration(a);

  if (t <= SEGMENT_TIME) {
    // Move
    exec_move_to_target(ex.seg.target);
    ex.seg.time = 0;

  } else {
    // Compute next target
    float ratio = SEGMENT_TIME / t;
    float target[AXES];
    for (int axis = 0; axis < AXES; axis++) {
      float diff = ex.seg.target[axis] - ex.position[axis];
      target[axis] = ex.position[axis] + ratio * diff;
    }

    // Move
    exec_move_to_target(target);

    // Update time
    if (t == ex.seg.time) ex.seg.time -= SEGMENT_TIME;
    else ex.seg.time -= SEGMENT_TIME * v / ex.seg.vel;
  }

  // Check switch
  if (seek_found()) state_seek_hold(true);

  return STAT_OK;
}


stat_t exec_segment(float time, const float target[], float vel, float accel,
                    float maxAccel, float maxJerk,
                    const power_update_t power_updates[]) {
  // Copy power updates in to the correct position given the time offset
  float nextT       = ex.seg.time + time;
  const float stepT = 1.0 / 60000; // 1ms in mins
  float t           = 0.5 / 60000; // 0.5ms in mins
  unsigned j        = 0;

  for (unsigned i = 0; t < nextT && j < POWER_MAX_UPDATES; i++) {
    if (ex.seg.time < t) ex.seg.power_updates[i] = power_updates[j++];
    t += stepT;
  }

  copy_vector(ex.seg.target, target);
  ex.seg.time      = nextT;
  ex.seg.vel       = vel;
  ex.seg.accel     = accel;
  ex.seg.max_accel = maxAccel;
  ex.seg.max_jerk  = maxJerk;
  ex.seg.cb        = ex.cb;
  ex.cb            = _segment_exec;

  // TODO To be precise, seek_end() should not be called until the current
  // segment has completed execution.
  if (!ex.seg.cb) seek_end(); // No callback when at line end

  return _segment_exec();
}


// Called by stepper.c from low-level interrupt
stat_t exec_next() {
  // Hold if we've reached zero velocity between commands and stopping
  if (!ex.cb && !exec_get_velocity() && state_get() == STATE_STOPPING)
    state_holding();

  if (state_get() == STATE_HOLDING) return STAT_NOP;
  if (!ex.cb && !command_exec()) return STAT_NOP; // Queue empty
  if (!ex.cb) return STAT_AGAIN; // Non-exec command
  return ex.cb(); // Exec
}


// Variable callbacks
float get_axis_position(int axis)    {return ex.position[axis];}
float get_velocity()                 {return ex.velocity / VELOCITY_MULTIPLIER;}
float get_acceleration()             {return ex.accel / ACCEL_MULTIPLIER;}
float get_jerk()                     {return ex.jerk / JERK_MULTIPLIER;}
float get_peak_vel()                 {return ex.peak_vel / VELOCITY_MULTIPLIER;}
void  set_peak_vel(float x)          {ex.peak_vel = 0;}
float get_peak_accel()               {return ex.peak_accel / ACCEL_MULTIPLIER;}
void  set_peak_accel(float x)        {ex.peak_accel = 0;}
float get_feed_override()            {return ex.feed_override;}
void  set_feed_override(float value) {ex.feed_override = value;}


// Command callbacks
typedef struct {
  uint8_t axis;
  float position;
} set_axis_t;


stat_t command_set_axis(char *cmd) {
  cmd++; // Skip command name

  // Decode axis
  int axis = axis_get_id(*cmd++);
  if (axis < 0) return STAT_INVALID_ARGUMENTS;

  // Decode position
  float position;
  if (!decode_float(&cmd, &position)) return STAT_BAD_FLOAT;

  // Check for end of command
  if (*cmd) return STAT_INVALID_ARGUMENTS;

  // Update command
  command_set_axis_position((uint8_t)axis, position);

  // Queue
  set_axis_t set_axis = {(uint8_t)axis, position};
  command_push(COMMAND_set_axis, &set_axis);

  return STAT_OK;
}


unsigned command_set_axis_size() {return sizeof(set_axis_t);}


void command_set_axis_exec(void *data) {
  set_axis_t *cmd = (set_axis_t *)data;

  // Update exec
  ex.position[cmd->axis] = cmd->position;

  // Update motors
  for (int motor = 0; motor < MOTORS; motor++)
    if (motor_get_axis(motor) == cmd->axis)
      motor_set_position(motor, cmd->position);
}
