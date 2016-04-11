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


#include "calibrate.h"

#include "buffer.h"
#include "motor.h"
#include "canonical_machine.h"
#include "planner.h"
#include "stepper.h"
#include "rtc.h"
#include "tmc2660.h"
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define CAL_THRESHOLDS 32
#define CAL_VELOCITIES 256
#define CAL_MIN_THRESH -10
#define CAL_MAX_THRESH 63
#define CAL_WAIT_TIME 3 // ms


enum {
  CAL_START,
  CAL_ACCEL,
  CAL_MEASURE,
  CAL_DECEL,
};


typedef struct {
  bool busy;

  uint32_t wait;
  int state;
  int motor;
  int axis;
  int vstep;

  float current_velocity;

  uint16_t stallguard[CAL_VELOCITIES];
  uint16_t velocities[CAL_VELOCITIES];
} calibrate_t;

static calibrate_t cal = {};


static stat_t _exec_calibrate(mpBuf_t *bf) {
  if (bf->move_state == MOVE_NEW) bf->move_state = MOVE_RUN;

  const float time = MIN_SEGMENT_TIME; // In minutes
  const float maxDeltaV = JOG_ACCELERATION * time;

  if (cal.wait <= rtc_get_time())
    switch (cal.state) {
    case CAL_START: {
      cal.axis = motor_get_axis(cal.motor);
      cal.state = CAL_ACCEL;

      cal.current_velocity = 0;
      float max_velocity = cm.a[cal.axis].velocity_max / 2;
      for (int i = 0; i < CAL_VELOCITIES; i++)
        cal.velocities[i] = (1 + i) * max_velocity / CAL_VELOCITIES;

      memset(cal.stallguard, 0, sizeof(cal.stallguard));

      tmc2660_set_stallguard_threshold(cal.motor, 8);
      cal.wait = rtc_get_time() + CAL_WAIT_TIME;

      break;
    }

    case CAL_ACCEL:
      if (cal.velocities[cal.vstep] == cal.current_velocity)
        cal.state = CAL_MEASURE;

      else {
        cal.current_velocity += maxDeltaV;

        if (cal.velocities[cal.vstep] <= cal.current_velocity)
          cal.current_velocity = cal.velocities[cal.vstep];
      }
      break;

    case CAL_MEASURE:
      if (++cal.vstep == CAL_VELOCITIES) cal.state = CAL_DECEL;
      else cal.state = CAL_ACCEL;
      break;

    case CAL_DECEL:
      cal.current_velocity -= maxDeltaV;
      if (cal.current_velocity < 0) cal.current_velocity = 0;

      if (!cal.current_velocity) {
        // Print results
        putchar('\n');
        for (int i = 0; i < CAL_VELOCITIES; i++)
          printf("%d,", cal.velocities[i]);
        putchar('\n');
        for (int i = 0; i < CAL_VELOCITIES; i++)
          printf("%d,", cal.stallguard[i]);
        putchar('\n');

        mp_free_run_buffer(); // Release buffer
        cal.busy = false;

        return STAT_OK;
      }
      break;
    }

  if (!cal.current_velocity) return STAT_OK;

  // Compute travel
  float travel[AXES] = {}; // In mm
  travel[cal.axis] = time * cal.current_velocity;

  // Convert to steps
  float steps[MOTORS] = {0};
  mp_kinematics(travel, steps);

  // Queue segment
  float error[MOTORS] = {0};
  st_prep_line(steps, error, time);

  return STAT_OK;
}


bool calibrate_busy() {return cal.busy;}


void calibrate_set_stallguard(int motor, uint16_t sg) {
  if (cal.vstep < CAL_VELOCITIES && cal.motor == motor)
    cal.stallguard[cal.vstep] = sg;
}


uint8_t command_calibrate(int argc, char *argv[]) {
  if (cal.busy) return 0;

  mpBuf_t *bf = mp_get_write_buffer();
  if (!bf) {
    cm_hard_alarm(STAT_BUFFER_FULL_FATAL);
    return 0;
  }

  // Start
  memset(&cal, 0, sizeof(cal));
  cal.busy = true;

  bf->bf_func = _exec_calibrate; // register callback
  mp_commit_write_buffer(MOVE_TYPE_COMMAND);

  return 0;
}
