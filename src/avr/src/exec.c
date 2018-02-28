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

#include "exec.h"

#include "stepper.h"
#include "motor.h"
#include "axis.h"
#include "util.h"
#include "command.h"
#include "report.h"
#include "switch.h"
#include "seek.h"
#include "estop.h"
#include "state.h"
#include "config.h"


static struct {
  exec_cb_t cb;

  float position[AXES];
  float velocity;
  float accel;
  float jerk;

  float feed_override;
  float spindle_override;
} ex;


static void _limit_switch_cb(switch_id_t sw, bool active) {
  if (sw == seek_get_switch()) return;
  if (ex.velocity && active) estop_trigger(STAT_ESTOP_SWITCH);
}


void exec_init() {
  memset(&ex, 0, sizeof(ex));
  ex.feed_override = 1;
  ex.spindle_override = 1;
  // TODO implement overrides

  // Set callback for limit switches
  for (switch_id_t sw = SW_MIN_X; sw <= SW_MAX_A; sw++)
    switch_set_callback(sw, _limit_switch_cb);
}


void exec_get_position(float p[AXES]) {
  memcpy(p, ex.position, sizeof(ex.position));
}


float exec_get_axis_position(int axis) {return ex.position[axis];}
void exec_set_velocity(float v) {ex.velocity = v;}
float exec_get_velocity() {return ex.velocity;}
void exec_set_acceleration(float a) {ex.accel = a;}
float exec_get_acceleration() {return ex.accel;}
void exec_set_jerk(float j) {ex.jerk = j;}
void exec_set_cb(exec_cb_t cb) {ex.cb = cb;}


stat_t exec_move_to_target(float time, const float target[]) {
  ASSERT(isfinite(time));
  ASSERT(isfinite(target[AXIS_X]) && isfinite(target[AXIS_Y]) &&
         isfinite(target[AXIS_Z]) && isfinite(target[AXIS_A]) &&
         isfinite(target[AXIS_B]) && isfinite(target[AXIS_C]));

  // Update position
  copy_vector(ex.position, target);

  // Call the stepper prep function
  st_prep_line(time, target);

  return STAT_OK;
}


stat_t exec_next() {
  // Hold if we've reached zero velocity between commands an stopping
  if (!ex.cb && !exec_get_velocity() && state_get() == STATE_STOPPING)
    state_holding();

  if (!ex.cb && !command_exec()) return STAT_NOP; // Queue empty
  if (!ex.cb) return STAT_AGAIN; // Non-exec command
  return ex.cb(); // Exec
}


// Variable callbacks
float get_axis_position(int axis) {return ex.position[axis];}
float get_velocity() {return ex.velocity / VELOCITY_MULTIPLIER;}
float get_acceleration() {return ex.accel / ACCEL_MULTIPLIER;}
float get_jerk() {return ex.jerk / JERK_MULTIPLIER;}
float get_feed_override() {return ex.feed_override;}
void set_feed_override(float value) {ex.feed_override = value;}
float get_speed_override() {return ex.spindle_override;}
void set_speed_override(float value) {ex.spindle_override = value;}


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
  command_set_axis_position(axis, position);

  // Queue
  set_axis_t set_axis = {axis, position};
  command_push(COMMAND_set_axis, &set_axis);

  return STAT_OK;
}


unsigned command_set_axis_size() {return sizeof(set_axis_t);}


void command_set_axis_exec(void *data) {
  set_axis_t *cmd = (set_axis_t *)data;

  // Update exec
  ex.position[cmd->axis] = cmd->position;

  // Update motors
  int motor = axis_get_motor(cmd->axis);
  if (0 <= motor) motor_set_position(motor, cmd->position);

  // Report
  report_request();
}
