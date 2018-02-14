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

#include "exec.h"
#include "jog.h"
#include "stepper.h"
#include "axis.h"
#include "spindle.h"
#include "util.h"
#include "command.h"
#include "config.h"

#include <string.h>
#include <float.h>


static struct {
  exec_cb_t cb;

  float position[AXES];
  float velocity;
  float accel;
  float jerk;

  int tool;

  float feed_override;
  float spindle_override;
} ex;


void exec_init() {
  memset(&ex, 0, sizeof(ex));
  ex.feed_override = 1;
  ex.spindle_override = 1;
  // TODO implement move stepping
  // TODO implement overrides
  // TODO implement optional pause
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


void exec_reset_encoder_counts() {st_set_position(ex.position);}


stat_t exec_next() {
  if (!ex.cb && !command_exec()) return STAT_NOP; // Queue empty
  if (!ex.cb) return STAT_AGAIN; // Non-exec command
  return ex.cb(); // Exec
}


// Variable callbacks
uint8_t get_tool() {return ex.tool;}
float get_velocity() {return ex.velocity / VELOCITY_MULTIPLIER;}
float get_acceleration() {return ex.accel / ACCEL_MULTIPLIER;}
float get_jerk() {return ex.jerk / JERK_MULTIPLIER;}
float get_feed_override() {return ex.feed_override;}
float get_speed_override() {return ex.spindle_override;}
float get_axis_position(int axis) {return ex.position[axis];}

void set_tool(uint8_t tool) {ex.tool = tool;}
void set_feed_override(float value) {ex.feed_override = value;}
void set_speed_override(float value) {ex.spindle_override = value;}
void set_axis_position(int axis, float p) {ex.position[axis] = p;}


// Command callbacks
stat_t command_opt_pause(char *cmd) {command_push(*cmd, 0); return STAT_OK;}
unsigned command_opt_pause_size() {return 0;}
void command_opt_pause_exec(void *data) {} // TODO pause if requested
