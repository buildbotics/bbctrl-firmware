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

#include "config.h"
#include "pwm_spindle.h"
#include "huanyang.h"
#include "pgmspace.h"


typedef enum {
  SPINDLE_TYPE_DISABLED,
  SPINDLE_TYPE_PWM,
  SPINDLE_TYPE_HUANYANG,
} spindle_type_t;


typedef struct {
  spindle_type_t type;
  float speed;
  bool reversed;
} spindle_t;


static spindle_t spindle = {SPINDLE_TYPE_DISABLED,};


void spindle_init() {}


void spindle_set_speed(float speed) {
  spindle.speed = speed;

  if (spindle.reversed) speed = -speed;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_spindle_set(speed); break;
  case SPINDLE_TYPE_HUANYANG: hy_set(speed); break;
  }
}


float spindle_get_speed() {return spindle.speed;}


void spindle_stop() {
  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_spindle_stop(); break;
  case SPINDLE_TYPE_HUANYANG: hy_stop(); break;
  }
}


uint8_t get_spindle_type() {return spindle.type;}


void set_spindle_type(uint8_t value) {
  if (value != spindle.type) {
    float speed = spindle.speed;

    switch (spindle.type) {
    case SPINDLE_TYPE_DISABLED: break;
    case SPINDLE_TYPE_PWM: pwm_spindle_deinit(); break;
    case SPINDLE_TYPE_HUANYANG: hy_deinit(); break;
    }

    spindle.type = (spindle_type_t)value;

    switch (spindle.type) {
    case SPINDLE_TYPE_DISABLED: break;
    case SPINDLE_TYPE_PWM: pwm_spindle_init(); break;
    case SPINDLE_TYPE_HUANYANG: hy_init(); break;
    }

    spindle_set_speed(speed);
  }
}


bool spindle_is_reversed() {return spindle.reversed;}
bool get_spin_reversed() {return spindle.reversed;}


void set_spin_reversed(bool reversed) {
  if (spindle.reversed != reversed) {
    spindle.reversed = reversed;
    spindle_set_speed(spindle.speed);
  }
}


// Var callbacks
void set_speed(float speed) {spindle_set_speed(speed);}
float get_speed() {return spindle_get_speed();}
