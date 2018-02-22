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

#include "config.h"
#include "exec.h"
#include "axis.h"
#include "command.h"
#include "scurve.h"
#include "state.h"
#include "seek.h"
#include "util.h"

#include <math.h>
#include <string.h>
#include <stdio.h>


typedef struct {
  float start[AXES];
  float target[AXES];
  float times[7];
  float target_vel;
  float max_accel;
  float max_jerk;

  float unit[AXES];
  float length;
} line_t;


static struct {
  line_t line;

  int section;
  bool stop_section;
  float current_time;
  float offset_time;
  int seg;

  float iD; // Initial section distance
  float iV; // Initial section velocity
  float iA; // Initial section acceleration
  float jerk;
  float dist;
} l = {.current_time = SEGMENT_TIME};


static void _segment_target(float target[AXES], float d) {
  for (int i = 0; i < AXES; i++)
    target[i] = l.line.start[i] + l.line.unit[i] * d;
}


static float _segment_distance(float t) {
  return l.iD + scurve_distance(t, l.iV, l.iA, l.jerk);
}


static float _segment_velocity(float t) {
  return l.iV + scurve_velocity(t, l.iA, l.jerk);
}


static float _segment_accel(float t) {
  return l.iA + scurve_acceleration(t, l.jerk);
}


static bool _section_next() {
  while (++l.section < 7) {
    float section_time = l.line.times[l.section];
    if (!section_time) continue;

    // Check if last section
    bool last_section = true;
    for (int i = l.section + 1; i < 7; i++)
      if (l.line.times[i]) last_section = false;

    // Check if stop section
    l.stop_section = last_section && !l.line.target_vel;

    // Jerk
    switch (l.section) {
    case 0: case 6: l.jerk = l.line.max_jerk; break;
    case 2: case 4: l.jerk = -l.line.max_jerk; break;
    default: l.jerk = 0;
    }
    exec_set_jerk(l.jerk);

    // Acceleration
    switch (l.section) {
    case 1: case 2: l.iA = l.line.max_jerk * l.line.times[0]; break;
    case 5: case 6: l.iA = -l.line.max_jerk * l.line.times[4]; break;
    default: l.iA = 0;
    }
    exec_set_acceleration(l.iA);

    // Skip sections which do not land on a SEGMENT_TIME
    if (section_time < l.current_time && !l.stop_section) {
      l.current_time -= l.line.times[l.section];
      l.iD = _segment_distance(section_time);
      l.iV = _segment_velocity(section_time);
      continue;
    }

    // Setup section time offset and segment counter
    l.offset_time = l.current_time;
    l.seg = 0;

    return true;
  }

  return false;
}


static void _done() {
  seek_end();
  exec_set_cb(0);
}


static stat_t _move(float t, float target[AXES], float v, float a) {
  exec_set_velocity(v);
  exec_set_acceleration(a);

  if (seek_switch_found()) state_seek_hold();

  return exec_move_to_target(t, target);
}


static stat_t _pause() {
  float t = SEGMENT_TIME - l.current_time;
  float v = exec_get_velocity();
  float a = exec_get_acceleration();
  float j = l.line.max_jerk;

  if (v < MIN_VELOCITY) {
    command_reset_position();
    exec_set_velocity(0);
    exec_set_acceleration(0);
    exec_set_jerk(0);
    state_holding();
    _done();

    return STAT_NOP;
  }

  // Compute new velocity and acceleration
  a = scurve_next_accel(t, v, 0, a, j);
  if (l.line.max_accel < fabs(a)) {
    a = (a < 0 ? -1 : 1) * l.line.max_accel;
    j = 0;
  } else if (0 < a) j = -j;
  v += a * t;

  // Compute distance that will be traveled
  l.dist += v * t;

  if (l.line.length < l.dist) {
    // Compute time left in current section
    l.current_time = t - (l.dist - l.line.length) / v;

    exec_set_acceleration(0);
    exec_set_jerk(0);
    _done();

    return STAT_AGAIN;
  }

  // Compute target position from distance
  float target[AXES];
  _segment_target(target, l.dist);

  l.current_time = 0;
  exec_set_jerk(j);

  return _move(SEGMENT_TIME, target, v, a);
}


static stat_t _line_exec() {
  // Pause if requested.  If we are already stopping, just continue.
  if (state_get() == STATE_STOPPING && (l.section < 4 || l.line.target_vel)) {
    if (SEGMENT_TIME < l.current_time) l.current_time = 0;
    exec_set_cb(_pause);
    return _pause();
  }

  float section_time = l.line.times[l.section];

  // Adjust segment time if near a stopping point
  float seg_time = SEGMENT_TIME;
  if (l.stop_section && section_time - l.current_time < SEGMENT_TIME) {
    seg_time += section_time - l.current_time;
    l.offset_time += section_time - l.current_time;
    l.current_time = section_time;
  }

  // Compute distance and velocity.  Must be computed before advancing time.
  float d = _segment_distance(l.current_time);
  float v = _segment_velocity(l.current_time);
  float a = _segment_accel(l.current_time);

  // Advance time.  This is the start time for the next segment.
  l.current_time = l.offset_time + ++l.seg * SEGMENT_TIME;

  // Check if section complete
  bool lastSection = false;
  if (section_time < l.current_time) {
    l.current_time -= section_time;
    l.iD = _segment_distance(section_time);
    l.iV = _segment_velocity(section_time);

    // Next section
    lastSection = !_section_next();
  }

  // Do move & update exec
  stat_t status;

  if (lastSection && l.stop_section)
    // Stop exactly on target to correct for floating-point errors
    status = _move(seg_time, l.line.target, 0, 0);

  else {
    // Compute target position from distance
    float target[AXES];
    _segment_target(target, d);

    status = _move(seg_time, target, v, a);
    l.dist = d;
  }

  // Release exec if we are done
  if (lastSection) {
    if (state_get() == STATE_STOPPING) state_holding();
    exec_set_velocity(l.line.target_vel);
    _done();
  }

  return status;
}


void _print_vector(const char *name, float v[4]) {
  printf("%s %f %f %f %f\n", name, v[0], v[1], v[2], v[3]);
}


stat_t command_line(char *cmd) {
  line_t line = {};

  cmd++; // Skip command code

  // Get start position
  command_get_position(line.start);

  // Get target velocity
  if (!decode_float(&cmd, &line.target_vel)) return STAT_BAD_FLOAT;
  if (line.target_vel < 0) return STAT_INVALID_ARGUMENTS;

  // Get max accel
  if (!decode_float(&cmd, &line.max_accel)) return STAT_BAD_FLOAT;
  if (line.max_accel < 0) return STAT_INVALID_ARGUMENTS;

  // Get max jerk
  if (!decode_float(&cmd, &line.max_jerk)) return STAT_BAD_FLOAT;
  if (line.max_jerk < 0) return STAT_INVALID_ARGUMENTS;

  // Get target position
  copy_vector(line.target, line.start);
  stat_t status = decode_axes(&cmd, line.target);
  if (status) return status;

  // Get times
  bool has_time = false;
  while (*cmd) {
    if (*cmd < '0' || '6' < *cmd) break;
    int section = *cmd - '0';
    cmd++;

    float time;
    if (!decode_float(&cmd, &time)) return STAT_BAD_FLOAT;

    if (time < 0) return STAT_NEGATIVE_SCURVE_TIME;
    line.times[section] = time;
    if (time) has_time = true;
   }

  if (!has_time) return STAT_ALL_ZERO_SCURVE_TIMES;

  // Check for end of command
  if (*cmd) return STAT_INVALID_ARGUMENTS;

  // Set next start position
  command_set_position(line.target);

  // Compute direction vector
  for (int i = 0; i < AXES; i++)
    if (axis_is_enabled(i)) {
      line.unit[i] = line.target[i] - line.start[i];
      line.length += line.unit[i] * line.unit[i];

    } else line.unit[i] = 0;

  line.length = sqrt(line.length);
  for (int i = 0; i < AXES; i++)
    if (line.unit[i]) line.unit[i] /= line.length;

  // Queue
  command_push(COMMAND_line, &line);

  return STAT_OK;
}


unsigned command_line_size() {return sizeof(line_t);}


void command_line_exec(void *data) {
  l.line = *(line_t *)data;

  // Init dist & velocity
  l.iD = l.dist = 0;
  l.iV = exec_get_velocity();

  // Find first section
  l.section = -1;
  if (!_section_next()) return;

  // Compare start position to actual position
  float diff[AXES];
  bool report = false;
  exec_get_position(diff);
  for (int i = 0; i < AXES; i++) {
    diff[i] -= l.line.start[i];
    if (0.1 < fabs(diff[i])) report = true;
  }

  if (report)
    STATUS_DEBUG("diff: %.4f %.4f %.4f %.4f",
                 diff[0], diff[1], diff[2], diff[3]);

  // Set callback
  exec_set_cb(_line_exec);
}
