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
#include "pgmspace.h"

#include <math.h>


typedef enum {
  SPINDLE_TYPE_DISABLED,
#define SPINDLE(NAME) SPINDLE_TYPE_##NAME,
#include "spindle.def"
#undef SPINDLE
} spindle_type_t;


// Function decls
#define SPINDLE(NAME)                         \
  void NAME##_init();                         \
  void NAME##_deinit();                       \
  void NAME##_set(float speed);               \
  float NAME##_get();                         \
  void NAME##_stop();
#include "spindle.def"
#undef SPINDLE


typedef struct {
  spindle_type_t type;
  float speed;
  bool reversed;
  float min_rpm;
  float max_rpm;
} spindle_t;


static spindle_t spindle = {SPINDLE_TYPE_DISABLED,};


void spindle_set_speed(float speed) {
  spindle.speed = speed;

  bool negative = speed < 0;
  if (spindle.max_rpm <= fabs(speed)) speed = negative ? -1 : 1;
  else if (fabs(speed) < spindle.min_rpm) speed = 0;
  else speed /= spindle.max_rpm;

  if (spindle.reversed) speed = -speed;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
#define SPINDLE(NAME) case SPINDLE_TYPE_##NAME: NAME##_set(speed); break;
#include "spindle.def"
#undef SPINDLE
  }
}


float spindle_get_speed() {
  float speed = 0;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
#define SPINDLE(NAME) case SPINDLE_TYPE_##NAME: speed = NAME##_get(); break;
#include "spindle.def"
#undef SPINDLE
  }

  return speed * spindle.max_rpm;
}


void spindle_stop() {
  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
#define SPINDLE(NAME) case SPINDLE_TYPE_##NAME: NAME##_stop(); break;
#include "spindle.def"
#undef SPINDLE
  }
}


bool spindle_is_reversed() {return spindle.reversed;}
static void _update_speed() {spindle_set_speed(spindle.speed);}


// Var callbacks
uint8_t get_spindle_type() {return spindle.type;}


void set_spindle_type(uint8_t value) {
  if (value != spindle.type) {
    float speed = spindle.speed;

    switch (spindle.type) {
    case SPINDLE_TYPE_DISABLED: break;
#define SPINDLE(NAME) case SPINDLE_TYPE_##NAME: NAME##_deinit(); break;
#include "spindle.def"
#undef SPINDLE
    }

    spindle.type = (spindle_type_t)value;

    switch (spindle.type) {
    case SPINDLE_TYPE_DISABLED: break;
#define SPINDLE(NAME) case SPINDLE_TYPE_##NAME: NAME##_init(); break;
#include "spindle.def"
#undef SPINDLE
    }

    spindle_set_speed(speed);
  }
}


void set_speed(float speed) {spindle_set_speed(speed);}
float get_speed() {return spindle_get_speed();}
bool get_spin_reversed() {return spindle.reversed;}


void set_spin_reversed(bool reversed) {
  if (spindle.reversed != reversed) {
    spindle.reversed = reversed;
    spindle_set_speed(spindle.speed);
  }
}


float get_max_spin() {return spindle.max_rpm;}
void set_max_spin(float value) {spindle.max_rpm = value; _update_speed();}
float get_min_spin() {return spindle.min_rpm;}
void set_min_spin(float value) {spindle.min_rpm = value; _update_speed();}
