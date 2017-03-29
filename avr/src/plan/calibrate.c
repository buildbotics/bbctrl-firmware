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


#include "calibrate.h"

#include "buffer.h"
#include "motor.h"
#include "machine.h"
#include "planner.h"
#include "stepper.h"
#include "rtc.h"
#include "state.h"
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>


#define CAL_VELOCITIES 256
#define CAL_MIN_VELOCITY 1000 // mm/sec
#define CAL_TARGET_SG 100
#define CAL_MAX_DELTA_SG 75
#define CAL_WAIT_TIME 3 // ms


enum {
  CAL_START,
  CAL_ACCEL,
  CAL_MEASURE,
  CAL_DECEL,
};


typedef struct {
  bool stall_valid;
  bool stalled;
  bool reverse;

  uint32_t wait;
  int state;
  int motor;
  int axis;

  float velocity;
  uint16_t stallguard;
} calibrate_t;

static calibrate_t cal = {0};


static stat_t _exec_calibrate(mp_buffer_t *bf) {
  const float time = SEGMENT_TIME; // In minutes
  const float max_delta_v = CAL_ACCELERATION * time;

  do {
    if (rtc_expired(cal.wait))
      switch (cal.state) {
      case CAL_START: {
        cal.axis = motor_get_axis(cal.motor);
        cal.state = CAL_ACCEL;
        cal.velocity = 0;
        cal.stall_valid = false;
        cal.stalled = false;
        cal.reverse = false;

        //tmc2660_set_stallguard_threshold(cal.motor, 8);
        cal.wait = rtc_get_time() + CAL_WAIT_TIME;

        break;
      }

      case CAL_ACCEL:
        if (CAL_MIN_VELOCITY < cal.velocity) cal.stall_valid = true;

        if (cal.velocity < CAL_MIN_VELOCITY || CAL_TARGET_SG < cal.stallguard)
          cal.velocity += max_delta_v;

        if (cal.stalled) {
          if (cal.reverse) {
            int32_t steps = -motor_get_position(cal.motor);
            float mm = (float)steps / motor_get_steps_per_unit(cal.motor);
            STATUS_DEBUG("%"PRIi32" steps %0.2f mm", steps, mm);

            //tmc2660_set_stallguard_threshold(cal.motor, 63);

            mp_set_cycle(CYCLE_MACHINING); // Default cycle

            return STAT_NOOP; // Done, no move queued

          } else {
            motor_set_position(cal.motor, 0);

            cal.reverse = true;
            cal.velocity = 0;
            cal.stall_valid = false;
            cal.stalled = false;
          }
        }
        break;
      }
  } while (fp_ZERO(cal.velocity)); // Repeat if computed velocity was zero

  // Compute travel
  float travel[AXES] = {0}; // In mm
  travel[cal.axis] = time * cal.velocity * (cal.reverse ? -1 : 1);

  // Convert to steps
  float steps[MOTORS] = {0};
  mp_kinematics(travel, steps);

  // Queue segment
  st_prep_line(steps);

  return STAT_EAGAIN;
}


bool calibrate_busy() {return mp_get_cycle() == CYCLE_CALIBRATING;}


void calibrate_set_stallguard(int motor, uint16_t sg) {
  if (cal.motor != motor) return;

  if (cal.stall_valid) {
    int16_t delta = sg - cal.stallguard;
    if (!sg || CAL_MAX_DELTA_SG < abs(delta)) {
      cal.stalled = true;
      //motor_end_move(cal.motor);
    }
  }

  cal.stallguard = sg;
}


uint8_t command_calibrate(int argc, char *argv[]) {
  if (mp_get_cycle() != CYCLE_MACHINING || mp_get_state() != STATE_READY)
    return 0;

  // Start
  memset(&cal, 0, sizeof(cal));
  mp_set_cycle(CYCLE_CALIBRATING);
  cal.motor = 1;

  mp_queue_push_nonstop(_exec_calibrate, -1);

  return 0;
}
