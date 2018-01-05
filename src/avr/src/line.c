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
#include "util.h"

#include <math.h>
#include <string.h>
#include <stdio.h>


typedef struct {
  float start[AXES];
  float target[AXES];
  float times[7];
  float target_vel;
  float max_jerk;

  float unit[AXES];
} line_t;


static struct {
  line_t line;

  int seg;
  bool stop_seg;
  float current_t;

  float dist;
  float vel;
  float accel;
  float jerk;
} l = {.current_t = SEGMENT_TIME};


static float _compute_distance(float t, float v, float a, float j) {
  // v * t + 1/2 * a * t^2 + 1/6 * j * t^3
  return t * (v + t * (0.5 * a + 1.0 / 6.0 * j * t));
}


static float _compute_velocity(float t, float a, float j) {
  // a * t + 1/2 * j * t^2
  return t * (a + 0.5 * j * t);
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
    case 1: case 2: l.accel = l.line.max_jerk * l.line.times[0]; break;
    case 5: case 6: l.accel = -l.line.max_jerk * l.line.times[4]; break;
    default: l.accel = 0;
    }
    exec_set_acceleration(l.accel);

    // Skip segs which do not land on a SEGMENT_TIME
    if (seg_t < l.current_t && !l.stop_seg) {
      l.current_t -= l.line.times[l.seg];
      l.dist += _compute_distance(seg_t, l.vel, l.accel, l.jerk);
      l.vel += _compute_velocity(seg_t, l.accel, l.jerk);
      continue;
    }

    return true;
  }

  return false;
}


static stat_t _line_exec() {
  float seg_t = l.line.times[l.seg];

  // Adjust time if near a stopping point
  float delta = SEGMENT_TIME;
  if (l.stop_seg && seg_t - l.current_t < SEGMENT_TIME) {
    delta += seg_t - l.current_t;
    l.current_t = seg_t;
  }

  // Compute distance and velocity offsets
  float d = l.dist + _compute_distance(l.current_t, l.vel, l.accel, l.jerk);
  float v = l.vel + _compute_velocity(l.current_t, l.accel, l.jerk);

  // Compute target position from distance
  float target[AXES];
  for (int i = 0; i < AXES; i++)
    target[i] = l.line.start[i] + l.line.unit[i] * d;

  // Advance time
  l.current_t += SEGMENT_TIME;

  // Check if segment complete
  bool lastSeg = false;
  if (seg_t < l.current_t) {
    l.current_t -= seg_t;
    l.dist += _compute_distance(seg_t, l.vel, l.accel, l.jerk);
    l.vel += _compute_velocity(seg_t, l.accel, l.jerk);

    // Next segment
    lastSeg = !_segment_next();
  }

  // Do move
  // Stop exactly on target to correct for floating-point errors
  stat_t status = exec_move_to_target
    (delta, (lastSeg && l.stop_seg) ? l.line.target : target);

  // Check if we're done
  if (lastSeg) {
    exec_set_velocity(l.vel = l.line.target_vel);
    exec_set_cb(0);

  } else exec_set_velocity(v);

  return status;
}


void _print_vector(const char *name, float v[AXES]) {
  printf("%s %f %f %f %f\n", name, v[0], v[1], v[2], v[3]);
}


stat_t command_line(char *cmd) {
  static float next_start[AXES];
  line_t line = {};

  cmd++; // Skip command code

  // Get start position
  copy_vector(line.start, next_start);

  // Get target velocity
  if (!decode_float(&cmd, &line.target_vel)) return STAT_BAD_FLOAT;
  if (line.target_vel < 0) return STAT_INVALID_ARGUMENTS;

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
  copy_vector(next_start, line.target);

  // Compute direction vector
  float length = 0;
  for (int i = 0; i < AXES; i++)
    if (axis_is_enabled(i)) {
      line.unit[i] = line.target[i] - line.start[i];
      length += line.unit[i] * line.unit[i];

    } else line.unit[i] = 0;

  length = sqrt(length);
  for (int i = 0; i < AXES; i++)
    if (line.unit[i]) line.unit[i] /= length;

  // Queue
  command_push(COMMAND_line, &line);

  return STAT_OK;
}


unsigned command_line_size() {return sizeof(line_t);}


void command_line_exec(void *data) {
  l.line = *(line_t *)data;

  // Init dist & velocity
  l.dist = 0;
  l.vel = exec_get_velocity();

  // Find first segment
  l.seg = -1;
  if (!_segment_next()) return;

  // Set callback
  exec_set_cb(_line_exec);
}
