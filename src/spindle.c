/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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

#include "spindle.h"

#include "config.h"
#include "pwm_spindle.h"
#include "huanyang.h"

#include "plan/command.h"


typedef enum {
  SPINDLE_TYPE_PWM,
  SPINDLE_TYPE_HUANYANG,
} spindleType_t;

static spindleType_t spindle_type = SPINDLE_TYPE;


static void _spindle_set(machSpindleMode_t mode, float speed) {
  switch (spindle_type) {
  case SPINDLE_TYPE_PWM: pwm_spindle_set(mode, speed); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_set(mode, speed); break;
  }
}


/// execute the spindle command (called from planner)
static void _exec_spindle_control(float *value, float *flag) {
  machSpindleMode_t mode = value[0];
  mach_set_spindle_mode(mode);
  _spindle_set(mode, mach.gm.spindle_speed);
}


/// Spindle speed callback from planner queue
static void _exec_spindle_speed(float *value, float *flag) {
  float speed = value[0];
  mach_set_spindle_speed_parameter(speed);
  _spindle_set(mach.gm.spindle_mode, speed);
}


void mach_spindle_init() {
  pwm_spindle_init();
  huanyang_init();
}


/// Queue the spindle command to the planner buffer
void mach_spindle_control(machSpindleMode_t mode) {
  float value[AXES] = {mode};
  mp_queue_command(_exec_spindle_control, value, value);
}


void mach_spindle_estop() {
  switch (spindle_type) {
  case SPINDLE_TYPE_PWM: pwm_spindle_estop(); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_estop(); break;
  }
}


/// Queue the S parameter to the planner buffer
void mach_set_spindle_speed(float speed) {
  float value[AXES] = {speed};
  mp_queue_command(_exec_spindle_speed, value, value);
}


uint8_t get_spindle_type(int index) {
  return spindle_type;
}


void set_spindle_type(int index, uint8_t value) {
  if (value != spindle_type) {
    _spindle_set(SPINDLE_OFF, 0);
    spindle_type = value;
    _spindle_set(mach.gm.spindle_mode, mach.gm.spindle_speed);
  }
}
