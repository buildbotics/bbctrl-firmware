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

  int seg;
  bool stop_seg;
  float current_t;

  float iD; // Initial segment distance
  float iV; // Initial segment velocity
  float iA; // Initial segment acceleration
  float jerk;
  float dist;
} l = {.current_t = SEGMENT_TIME};


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


static bool _segment_next() {
  while (++l.seg < 7) {
    float seg_t = l.line.times[l.seg];
    if (!seg_t) continue;

    // Check if last seg
    bool last_seg = true;
    for (int i = l.seg + 1; i < 7; i++)
      if (l.line.times[i]) last_seg = false;

    // Check if stop seg
    l.stop_seg = last_seg && !l.line.target_vel;

    // Jerk
    switch (l.seg) {
    case 0: case 6: l.jerk = l.line.max_jerk; break;
    case 2: case 4: l.jerk = -l.line.max_jerk; break;
    default: l.jerk = 0;
    }
    exec_set_jerk(l.jerk);

    // Acceleration
    switch (l.seg) {
    case 1: case 2: l.iA = l.line.max_jerk * l.line.times[0]; break;
    case 5: case 6: l.iA = -l.line.max_jerk * l.line.times[4]; break;
    default: l.iA = 0;
    }
    exec_set_acceleration(l.iA);

    // Skip segs which do not land on a SEGMENT_TIME
    if (seg_t < l.current_t && !l.stop_seg) {
      l.current_t -= l.line.times[l.seg];
      l.iD = _segment_distance(seg_t);
      l.iV = _segment_velocity(seg_t);
      continue;
    }

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
  float t = SEGMENT_TIME - l.current_t;
  float v = exec_get_velocity();
  float a = exec_get_acceleration();
  float j = l.line.max_jerk;

  if (v < MIN_VELOCITY) {
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
    // Compute time left in current segment
    l.current_t = t - (l.dist - l.line.length) / v;

    exec_set_velocity(l.line.target_vel);
    exec_set_acceleration(0);
    exec_set_jerk(0);
    _done();

    return STAT_AGAIN;
  }

  // Compute target position from distance
  float target[AXES];
  _segment_target(target, l.dist);

  l.current_t = 0;
  exec_set_jerk(j);

  return _move(SEGMENT_TIME, target, v, a);
}


static stat_t _line_exec() {
  if (state_get() == STATE_STOPPING && (l.seg < 4 || l.line.target_vel)) {
    if (SEGMENT_TIME < l.current_t) l.current_t = 0;
    exec_set_cb(_pause);
    return _pause();
  }

  float seg_t = l.line.times[l.seg];

  // Adjust time if near a stopping point
  float delta = SEGMENT_TIME;
  if (l.stop_seg && seg_t - l.current_t < SEGMENT_TIME) {
    delta += seg_t - l.current_t;
    l.current_t = seg_t;
  }

  // Compute distance and velocity (Must be computed before changing time)
  float d = _segment_distance(l.current_t);
  float v = _segment_velocity(l.current_t);
  float a = _segment_accel(l.current_t);

  // Advance time
  l.current_t += SEGMENT_TIME;

  // Check if segment complete
  bool lastSeg = false;
  if (seg_t < l.current_t) {
    l.current_t -= seg_t;
    l.iD = _segment_distance(seg_t);
    l.iV = _segment_velocity(seg_t);

    // Next segment
    lastSeg = !_segment_next();
  }

  // Do move & update exec
  stat_t status;

  if (lastSeg && l.stop_seg)
    // Stop exactly on target to correct for floating-point errors
    status = _move(delta, l.line.target, 0, 0);

  else {
    // Compute target position from distance
    float target[AXES];
    _segment_target(target, d);

    status = _move(delta, target, v, a);
    l.dist = d;
  }

  // Release exec if we are done
  if (lastSeg) _done();

  return status;
}


void _print_vector(const char *name, float v[AXES]) {
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

  // Zero disabled axes (TODO should moving a disable axis cause an error?)
  for (int i = 0; i < AXES; i++)
    if (!axis_is_enabled(i)) line.target[i] = 0;

  // Get times
  bool has_time = false;
  while (*cmd) {
    if (*cmd < '0' || '6' < *cmd) break;
    int seg = *cmd - '0';
    cmd++;

    float time;
    if (!decode_float(&cmd, &time)) return STAT_BAD_FLOAT;

    if (time < 0 || 0x10000 * SEGMENT_TIME <= time) return STAT_BAD_SEG_TIME;
    line.times[seg] = time;
    if (time) has_time = true;
   }

  if (!has_time) return STAT_BAD_SEG_TIME;

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

  // Find first segment
  l.seg = -1;
  if (!_segment_next()) return;

  // Set callback
  exec_set_cb(_line_exec);
}
