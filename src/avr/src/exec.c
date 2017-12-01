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
#include "queue.h"
#include "jog.h"
#include "stepper.h"
#include "axis.h"
#include "coolant.h"
#include "spindle.h"
#include "rtc.h"
#include "util.h"
#include "config.h"

#include <string.h>
#include <float.h>


typedef struct {
  bool new;

  float position[AXES];
  float target[AXES];
  float unit[AXES];

  uint16_t steps;
  uint16_t step;
  float delta;

  float dist;
  float vel;
  float jerk;
} segment_t;


static struct {
  bool busy;
  bool new;

  float position[AXES];
  float velocity;
  float accel;
  float jerk;

  int tool;
  int line;

  float feed_override;
  float spindle_override;

  int scurve;
  float time;
  float leftover_time;

  bool seek_error;
  bool seek_open;
  int seek_switch;

  uint32_t last_empty;

  segment_t seg;
} ex;


void exec_init() {
  memset(&ex, 0, sizeof(ex));
  ex.feed_override = 1;
  ex.spindle_override = 1;
  ex.seek_switch = -1;
  // TODO implement seek
  // TODO implement feedhold
  // TODO implement move stepping
  // TODO implement overrides
}


// Var callbacks
int32_t get_line() {return ex.line;}
uint8_t get_tool() {return ex.tool;}
float get_velocity() {return ex.velocity;}
float get_acceleration() {return ex.accel;}
float get_jerk() {return ex.jerk;}
float get_feed_override() {return ex.feed_override;}
float get_speed_override() {return ex.spindle_override;}
float get_axis_mach_coord(int axis) {return exec_get_axis_position(axis);}

void set_tool(uint8_t tool) {ex.tool = tool;}
void set_feed_override(float value) {ex.feed_override = value;}
void set_speed_override(float value) {ex.spindle_override = value;}
void set_axis_mach_coord(int axis, float position) {
  exec_set_axis_position(axis, position);
}


bool exec_is_busy() {return ex.busy;}
float exec_get_axis_position(int axis) {return ex.position[axis];}
void exec_set_axis_position(int axis, float p) {ex.position[axis] = p;}
void exec_set_velocity(float v) {ex.velocity = v;}
void exec_set_line(int line) {ex.line = line;}


stat_t exec_move_to_target(float time, const float target[]) {
  ASSERT(isfinite(target[AXIS_X]) && isfinite(target[AXIS_Y]) &&
         isfinite(target[AXIS_Z]) && isfinite(target[AXIS_A]) &&
         isfinite(target[AXIS_B]) && isfinite(target[AXIS_C]));

  // Update position
  copy_vector(ex.position, target);

  // No move if time is too short
  if (time < 0.5 * SEGMENT_TIME) {
    ex.leftover_time += time;
    return STAT_NOOP;
  }

  time += ex.leftover_time;
  ex.leftover_time = 0;

  // Call the stepper prep function
  st_prep_line(time, target);

  return STAT_OK;
}


void exec_reset_encoder_counts() {st_set_position(ex.position);}


static float _compute_distance(double t, double v, double a, double j) {
  // v * t + 1/2 * a * t^2 + 1/6 * j * t^3
  return t * (v + t * (0.5 * a + 1.0 / 6.0 * j * t));
}


static float _compute_velocity(double t, double a, double j) {
  // a * t + 1/2 * j * t^2
  return t * (a + 0.5 * j * t);
}


static void _segment_init(float time) {
  ASSERT(isfinite(time) && 0 < time && time < 0x10000 * SEGMENT_TIME);

  // Init segment
  ex.seg.new = false;
  ex.time = time;
  ex.seg.step = 0;
  ex.seg.steps = ceil(ex.time / SEGMENT_TIME);
  ex.seg.delta = time / ex.seg.steps;
  if (ex.scurve == 0) ex.seg.dist = 0;

  // Record starting position and compute unit vector
  float length = 0;
  for (int i = 0; i < AXES; i++) {
    ex.seg.position[i] = ex.position[i];
    ex.seg.unit[i] = ex.seg.target[i] - ex.position[i];
    length = ex.seg.unit[i] * ex.seg.unit[i];
  }
  length = sqrt(length);
  for (int i = 0; i < AXES; i++) ex.seg.unit[i] /= length;

  // Compute axis limited jerk
  if (ex.scurve == 0) {
    ex.seg.jerk = FLT_MAX;

    for (int i = 0; i < AXES; i++)
      if (ex.seg.unit[i]) {
        double j = fabs(axis_get_jerk_max(i) / ex.seg.unit[i]);
        if (j < ex.seg.jerk) ex.seg.jerk = j;
      }
  }

  // Jerk
  switch (ex.scurve) {
  case 0: case 6: ex.jerk = ex.seg.jerk; break;
  case 2: case 4: ex.jerk = -ex.seg.jerk; break;
  default: ex.jerk = 0;
  }

  // Acceleration
  switch (ex.scurve) {
  case 1: ex.accel = ex.seg.jerk * ex.time; break;
  case 3: ex.accel = 0; break;
  case 5: ex.accel = -ex.seg.jerk * ex.time; break;
  }
}


static stat_t _move_distance(float dist, bool end) {
  // Compute target position from distance
  float target[AXES];
  for (int i = 0; i < AXES; i++)
    target[i] = ex.seg.position[i] + ex.seg.unit[i] * dist;

  // Check if we have reached the end of the segment
  for (int i = 0; end && i < AXES; i++)
    if (0.000001 < fabs(ex.seg.target[i] - target[i])) end = false;

  // Do move
  return exec_move_to_target(ex.seg.delta, end ? ex.seg.target : target);
}


static stat_t _segment_body() {
  // Compute time, distance and velocity offsets
  float t = ex.seg.delta * (ex.seg.step + 1);
  float d = _compute_distance(t, ex.seg.vel, ex.accel, ex.jerk);
  float v = _compute_velocity(t, ex.accel, ex.jerk);

  // Update velocity
  exec_set_velocity(ex.seg.vel + v);

  return _move_distance(ex.seg.dist + d, false);
}


static void _set_scurve(int scurve) {
  ex.scurve = scurve;
  ex.seg.new = true;
}


static stat_t _segment_end() {
  // Update distance and velocity
  ex.seg.dist += _compute_distance(ex.time, ex.seg.vel, ex.accel, ex.jerk);
  ex.seg.vel += _compute_velocity(ex.time, ex.accel, ex.jerk);

  // Set final segment velocity
  exec_set_velocity(ex.seg.vel);

  // Automatically advance S-curve segment
  if (++ex.scurve == 7) ex.scurve = 0;
  _set_scurve(ex.scurve);

  return _move_distance(ex.seg.dist, true);
}


static stat_t _scurve_action(float time) {
  if (time <= 0) return STAT_NOOP; // Skip invalid curves
  if (ex.seg.new) _segment_init(time);
  return ++ex.seg.step == ex.seg.steps ? _segment_end() : _segment_body();
}


static void _set_output() {
  int output = queue_head_left();
  bool enable = queue_head_right();

  switch (output) {
  case 0: coolant_set_mist(enable); break;
  case 1: coolant_set_flood(enable); break;
  }
}


static void _seek() {
  ex.seek_switch = queue_head_left();
  ex.seek_open = queue_head_right() & (1 << 0);
  ex.seek_error = queue_head_right() & (1 << 1);
}


static void _set_home() {
  axis_set_homed(queue_head_left(), queue_head_right());
}


static void _pause(bool optional) {
  // TODO Initial immediate feedhold
}


stat_t exec_next_action() {
  if (queue_is_empty()) {
    ex.busy = false;
    ex.last_empty = rtc_get_time();
    return STAT_NOOP;
  }

  // On restart wait a bit to give queue a chance to fill
  if (!ex.busy && queue_get_fill() < EXEC_FILL_TARGET &&
      !rtc_expired(ex.last_empty + EXEC_DELAY)) return STAT_NOOP;

  ex.busy = true;
  stat_t status = STAT_NOOP;

  switch (queue_head()) {
  case ACTION_SCURVE: _set_scurve(queue_head_int()); break;
  case ACTION_DATA: status = _scurve_action(queue_head_float()); break; // TODO
  case ACTION_VELOCITY: ex.seg.vel = queue_head_float(); break;
  case ACTION_X: ex.seg.target[AXIS_X] = queue_head_float(); break;
  case ACTION_Y: ex.seg.target[AXIS_Y] = queue_head_float(); break;
  case ACTION_Z: ex.seg.target[AXIS_Z] = queue_head_float(); break;
  case ACTION_A: ex.seg.target[AXIS_A] = queue_head_float(); break;
  case ACTION_B: ex.seg.target[AXIS_B] = queue_head_float(); break;
  case ACTION_C: ex.seg.target[AXIS_C] = queue_head_float(); break;
  case ACTION_SEEK: _seek(); break;
  case ACTION_OUTPUT: _set_output(); break;
  case ACTION_DWELL: st_prep_dwell(queue_head_float()); status = STAT_OK; break;
  case ACTION_PAUSE: _pause(queue_head_bool()); status = STAT_PAUSE; break;
  case ACTION_TOOL: set_tool(queue_head_int()); break;
  case ACTION_SPEED: spindle_set_speed(queue_head_float()); break;
  case ACTION_JOG: status = jog_exec(); break;
  case ACTION_LINE_NUM: exec_set_line(queue_head_int()); break;
  case ACTION_SET_HOME: _set_home(); break;
  default: status = STAT_INTERNAL_ERROR; break;
  }

  if (status != STAT_EAGAIN) queue_pop();

  switch (status) {
  case STAT_NOOP: return STAT_EAGAIN;
  case STAT_PAUSE: return STAT_NOOP;
  case STAT_EAGAIN: return STAT_OK;
  default: break;
  }
  return status;
}
