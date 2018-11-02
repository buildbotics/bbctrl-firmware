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


#define SPEED_QUEUE_SIZE 32

typedef struct {
  int8_t time;
  float speed;
} speed_t;


#define RING_BUF_NAME speed_q
#define RING_BUF_TYPE speed_t
#define RING_BUF_INDEX_TYPE volatile uint8_t
#define RING_BUF_SIZE SPEED_QUEUE_SIZE
#include "ringbuf.def"


static struct {
  spindle_type_t type;
  float override;
  float speed;
  bool reversed;
  float min_rpm;
  float max_rpm;

  int8_t time;
  spindle_type_t next_type;

} spindle = {
  .type = SPINDLE_TYPE_DISABLED,
  .override = 1
};


void spindle_init() {
  speed_q_init();
  spindle_new_segment();
}


spindle_type_t spindle_get_type() {return spindle.type;}


void _set_speed(float speed) {
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


float _get_speed() {
  float speed = 0;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: speed = pwm_spindle_get(); break;
  case SPINDLE_TYPE_HUANYANG: speed = huanyang_get(); break;
  default: speed = vfd_spindle_get(); break;
  }

  return speed * spindle.max_rpm;
}


void spindle_stop() {_set_speed(0);}


bool spindle_is_reversed() {return spindle.reversed;}


void spindle_update() {
  while (!speed_q_empty()) {
    speed_t s = speed_q_peek();
    if (s.time == -1 || spindle.time < s.time) break;
    speed_q_pop();
    _set_speed(s.speed);
  }

  spindle.time++;
}


void spindle_next_segment() {
  while (!speed_q_empty()) {
    speed_t s = speed_q_next();
    if (s.time == -1) break;
    _set_speed(s.speed);
  }

  spindle.time = 0;
  spindle_update();
}


void spindle_new_segment() {
  speed_t s = {-1, 0};
  if (!speed_q_full()) speed_q_push(s);
}


void spindle_queue_speed(int8_t time, float speed) {
  if (speed_q_empty()) spindle_new_segment();

  speed_t s = {time, speed};

#if 1
  if (!speed_q_full()) speed_q_push(s);

#else
  speed_t *last = speed_q_back();

  if ((speed_q_empty() || last->time != time) && !speed_q_full())
    speed_q_push(s);
  else if (last->time == time) last->speed = speed;
#endif
}


static void _update_speed() {_set_speed(spindle.speed);}


static void _deinit_cb() {
  spindle.type = spindle.next_type;
  spindle.next_type = SPINDLE_TYPE_DISABLED;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_spindle_init(); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_init(); break;
  default: vfd_spindle_init(); break;
  }

  _set_speed(spindle.speed);
}


// Var callbacks
uint8_t get_tool_type() {return spindle.type;}


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


float get_speed() {return _get_speed();}
void set_speed(float speed) {spindle_queue_speed(0, speed);}
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
