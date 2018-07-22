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

#include "spindle.h"
#include "pwm_spindle.h"
#include "huanyang.h"
#include "vfd_spindle.h"
#include "config.h"
#include "pgmspace.h"

#include <math.h>


static struct {
  spindle_type_t type;
  float override;
  float speed;
  bool reversed;
  float min_rpm;
  float max_rpm;

  spindle_type_t next_type;
} spindle = {SPINDLE_TYPE_DISABLED, 1};


spindle_type_t spindle_get_type() {return spindle.type;}


void spindle_set_speed(float speed) {
  spindle.speed = speed;

  speed *= spindle.override;

  bool negative = speed < 0;
  if (spindle.max_rpm <= fabs(speed)) speed = negative ? -1 : 1;
  else if (fabs(speed) < spindle.min_rpm) speed = 0;
  else speed /= spindle.max_rpm;

  if (spindle.reversed) speed = -speed;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_spindle_set(speed); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_set(speed); break;
  default: vfd_spindle_set(speed); break;
  }
}


float spindle_get_speed() {
  float speed = 0;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: speed = pwm_spindle_get(); break;
  case SPINDLE_TYPE_HUANYANG: speed = huanyang_get(); break;
  default: speed = vfd_spindle_get(); break;
  }

  return speed * spindle.max_rpm;
}


void spindle_stop() {
  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_spindle_stop(); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_stop(); break;
  default: vfd_spindle_stop(); break;
  }
}


bool spindle_is_reversed() {return spindle.reversed;}
static void _update_speed() {spindle_set_speed(spindle.speed);}


// Var callbacks
uint8_t get_tool_type() {return spindle.type;}


static void _deinit_cb() {
  spindle.type = spindle.next_type;
  spindle.next_type = SPINDLE_TYPE_DISABLED;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_spindle_init(); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_init(); break;
  default: vfd_spindle_init(); break;
  }

  spindle_set_speed(spindle.speed);
}


void set_tool_type(uint8_t value) {
  if (value == spindle.type) return;

  spindle_type_t old_type = spindle.type;
  spindle.next_type = (spindle_type_t)value;
  spindle.type = SPINDLE_TYPE_DISABLED;

  switch (old_type) {
  case SPINDLE_TYPE_DISABLED: _deinit_cb(); break;
  case SPINDLE_TYPE_PWM: pwm_spindle_deinit(_deinit_cb); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_deinit(_deinit_cb); break;
  default: vfd_spindle_deinit(_deinit_cb); break;
  }
}


void set_speed(float speed) {spindle_set_speed(speed);}
float get_speed() {return spindle_get_speed();}
bool get_tool_reversed() {return spindle.reversed;}


void set_tool_reversed(bool reversed) {
  if (spindle.reversed != reversed) {
    spindle.reversed = reversed;
    _update_speed();
  }
}


float get_max_spin() {return spindle.max_rpm;}
void set_max_spin(float value) {spindle.max_rpm = value; _update_speed();}
float get_min_spin() {return spindle.min_rpm;}
void set_min_spin(float value) {spindle.min_rpm = value; _update_speed();}
uint16_t get_speed_override() {return spindle.override * 1000;}


void set_speed_override(uint16_t value) {
  spindle.override = value / 1000.0;
  _update_speed();
}
