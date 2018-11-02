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

#include "pwm_spindle.h"

#include "config.h"
#include "estop.h"
#include "outputs.h"

#include <math.h>


typedef struct {
  uint8_t time;
  uint16_t period;
  bool clockwise;
} update_t;


#define RING_BUF_NAME update_q
#define RING_BUF_TYPE update_t
#define RING_BUF_INDEX_TYPE volatile uint8_t
#define RING_BUF_SIZE SPEED_QUEUE_SIZE
#include "ringbuf.def"


typedef struct {
  float freq; // base frequency for PWM driver, in Hz
  float min_duty;
  float max_duty;
  float duty;
  float speed;
  uint8_t time;
} pwm_spindle_t;


static pwm_spindle_t spindle = {0};


static void _set_enable(bool enable) {
  outputs_set_active(TOOL_ENABLE_PIN, enable);
}


static void _set_dir(bool clockwise) {
  outputs_set_active(TOOL_DIR_PIN, !clockwise);
}


static void _update_clock(uint16_t period) {
  if (estop_triggered()) period = 0;

  // Disable
  if (!period) {
    TIMER_PWM.CTRLB = 0; // Disable clock control of pin
    OUTCLR_PIN(SPIN_PWM_PIN);
    _set_enable(false);
    return;
  }
  _set_enable(true);

  // 100% duty
  if (period == 0xffff) {
    TIMER_PWM.CTRLB = 0; // Disable clock control of pin
    OUTSET_PIN(SPIN_PWM_PIN);
    return;
  }

  // Configure clock
  TIMER_PWM.CTRLB = TC1_CCAEN_bm | TC_WGMODE_SINGLESLOPE_gc;
  TIMER_PWM.CCA = period;
}


static float _compute_duty(float speed) {
  if (!speed) return 0; // 0% duty
  if (speed == 1 && spindle.max_duty == 1) return 1; // 100% duty
  return speed * (spindle.max_duty - spindle.min_duty) + spindle.min_duty;
}


static uint16_t _compute_period(float speed) {
  spindle.speed = speed;
  spindle.duty = _compute_duty(speed);

  if (!spindle.duty) return 0;
  if (spindle.duty == 1) return 0xffff;
  return TIMER_PWM.PER * spindle.duty;
}


static void _update_pwm() {_update_clock(_compute_period(spindle.speed));}


static void _update_freq() {
  // Set clock period and optimal prescaler value
  float prescale = (F_CPU >> 16) / spindle.freq;

  if (prescale <= 1) {
    TIMER_PWM.PER = F_CPU / spindle.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV1_gc;

  } else if (prescale <= 2) {
    TIMER_PWM.PER = F_CPU / 2 / spindle.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV2_gc;

  } else if (prescale <= 4) {
    TIMER_PWM.PER = F_CPU / 4 / spindle.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV4_gc;

  } else if (prescale <= 8) {
    TIMER_PWM.PER = F_CPU / 8 / spindle.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV8_gc;

  } else if (prescale <= 64) {
    TIMER_PWM.PER = F_CPU / 64 / spindle.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV64_gc;

  } else TIMER_PWM.CTRLA = 0;

  _update_pwm();
}


void pwm_spindle_init() {
  update_q_init();

  // Configure IO
  _set_dir(true);
  _set_enable(false);
  _update_freq();

  // PWM output
  OUTCLR_PIN(SPIN_PWM_PIN);
  DIRSET_PIN(SPIN_PWM_PIN);
}


void pwm_spindle_deinit(deinit_cb_t cb) {
  _set_enable(false);

  // Float PWM output pin
  DIRCLR_PIN(SPIN_PWM_PIN);

  // Disable clock
  TIMER_PWM.CTRLA = 0;

  cb();
}


void pwm_spindle_set(uint8_t time, float speed) {
  if (update_q_full()) update_q_init();

  update_t d = {
    (uint8_t)(spindle.time + time + SPEED_OFFSET),
    _compute_period(fabsf(speed)),
    0 < speed
  };

  update_q_push(d);
}


float pwm_spindle_get() {return spindle.speed;}


void pwm_spindle_update() {
  bool set = false;
  uint16_t period;
  bool clockwise;

  while (!update_q_empty()) {
    update_t d = update_q_peek();
    if (spindle.time != d.time) break;
    update_q_pop();
    set = true;
    period = d.period;
    clockwise = d.clockwise;
  }

  if (set) {
    _update_clock(period);
    if (period) _set_dir(clockwise);
  }

  spindle.time++;
}


// Var callbacks
float get_pwm_min_duty() {return spindle.min_duty * 100;}


void set_pwm_min_duty(float value) {
  spindle.min_duty = value / 100;
  _update_pwm();
}


float get_pwm_max_duty() {return spindle.max_duty * 100;}


void set_pwm_max_duty(float value) {
  spindle.max_duty = value / 100;
  _update_pwm();
}


float get_pwm_duty() {return spindle.duty;}
float get_pwm_freq() {return spindle.freq;}


void set_pwm_freq(float value) {
  if (value < 8) value = 8;
  if (320000 < value) value = 320000;
  spindle.freq = value; _update_freq();
}


bool get_pwm_invert() {return PINCTRL_PIN(SPIN_PWM_PIN) & PORT_INVEN_bm;}


void set_pwm_invert(bool invert) {
  if (invert) PINCTRL_PIN(SPIN_PWM_PIN) |= PORT_INVEN_bm;
  else PINCTRL_PIN(SPIN_PWM_PIN) &= ~PORT_INVEN_bm;
}
