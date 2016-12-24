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

#include "home.h"

#include "plan/runtime.h"
#include "plan/line.h"
#include "plan/state.h"
#include "plan/exec.h"
#include "axes.h"
#include "motor.h"
#include "util.h"
#include "config.h"

#include <stdint.h>
#include <string.h>


typedef enum {
  STATE_INIT,
  STATE_HOMING,
  STATE_STALLED,
  STATE_BACKOFF,
  STATE_DONE,
} home_state_t;


typedef struct {
  bool homed[AXES];
  unsigned axis;
  home_state_t state;
  float velocity;
  uint16_t microsteps;
} home_t;

static home_t home;


void _stall_callback(int motor) {
  if (home.velocity == mp_runtime_get_velocity()) {
    mp_exec_abort();
    home.state = STATE_STALLED;
  }
}


void _move_axis(float travel) {
  float target[AXES];
  float *position = mp_runtime_get_position();
  copy_vector(target, position);
  target[home.axis] += travel;
  mp_aline(target, false, false, false, home.velocity, 1, -1);
}


void home_callback() {
  if (mp_get_cycle() != CYCLE_HOMING || !mp_is_quiescent() ||
      !mp_queue_is_empty()) return;

  while (true) {
    axis_t *axis = &axes[home.axis];
    int motor = axes_get_motor(home.axis);
    int direction = (axis->homing_mode == HOMING_STALL_MIN ||
                     axis->homing_mode == HOMING_SWITCH_MIN) ? -1 : 1;

    switch (home.state) {
    case STATE_INIT: {
      if (motor == -1 || axis->mode == AXIS_DISABLED ||
          axis->homing_mode == HOMING_DISABLED || !motor_is_enabled(motor)) {
        home.state = STATE_DONE;
        break;
      }

      STATUS_INFO("Homing %c axis", axis_get_char(home.axis));

      // Set axis not homed
      home.homed[home.axis] = false;

      // Determine homing type: switch or stall

      // Configure driver, set stall callback and compute homing velocity
      home.microsteps = motor_get_microsteps(motor);
      motor_set_microsteps(motor, 8);
      motor_set_stall_callback(motor, _stall_callback);
      //home.velocity = motor_get_stall_homing_velocity(motor);
      home.velocity = axis->search_velocity;

      // Move in home direction
      float travel = axis->travel_max - axis->travel_min;
      _move_axis(travel * direction);

      home.state = STATE_HOMING;
      return;
    }

    case STATE_HOMING:
    case STATE_STALLED:
      // Restore motor driver config
      motor_set_microsteps(motor, home.microsteps);
      motor_set_stall_callback(motor, 0);

      if (home.state == STATE_HOMING) {
        STATUS_ERROR(STAT_HOMING_CYCLE_FAILED, "Failed to find %c axis home",
                     axis_get_char(home.axis));
        mp_set_cycle(CYCLE_MACHINING);

      } else {
        STATUS_INFO("Backing off %c axis", axis_get_char(home.axis));
        mach_set_axis_position(home.axis, direction < 0 ? axis->travel_min :
                               axis->travel_max);
        _move_axis(axis->zero_backoff * direction * -1);
        home.state = STATE_BACKOFF;
      }
      return;

    case STATE_BACKOFF:
      STATUS_INFO("Homed %c axis", axis_get_char(home.axis));

      // Set axis position & homed
      mach_set_axis_position(home.axis, axis->travel_min);
      home.homed[home.axis] = true;
      home.state = STATE_DONE;
      break;

    case STATE_DONE:
      if (home.axis == AXIS_X) {
        // Done
        mp_set_cycle(CYCLE_MACHINING);
        return;
      }

      // Next axis
      home.axis--;
      home.state = STATE_INIT;
      break;
    }
  }
}


uint8_t command_home(int argc, char *argv[]) {
  if (mp_get_cycle() != CYCLE_MACHINING || mp_get_state() != STATE_READY)
    return 0;

  // Init
  memset(&home, 0, sizeof(home));
  home.axis = AXIS_Z;

  mp_set_cycle(CYCLE_HOMING);

  return 0;
}
