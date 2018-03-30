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
  uint16_t freq; // base frequency for PWM driver, in Hz
  float min_duty;
  float max_duty;
  float duty;
  float speed;
} pwm_spindle_t;


static pwm_spindle_t spindle = {0};


static void _set_enable(bool enable) {
  outputs_set_active(TOOL_ENABLE_PIN, enable);
}


static void _set_dir(bool clockwise) {
  outputs_set_active(TOOL_DIR_PIN, !clockwise);
}


static void _update_pwm() {
  float speed = spindle.speed;

  // Disable
  if (!speed || estop_triggered()) {
    TIMER_PWM.CTRLA = 0;
    OUTCLR_PIN(SPIN_PWM_PIN);
    _set_enable(false);
    return;
  }
  _set_enable(true);

  // 100% duty
  if (speed == 1 && spindle.max_duty == 1) {
    TIMER_PWM.CTRLB = 0;
    OUTSET_PIN(SPIN_PWM_PIN);
    return;
  }

  // Set clock period and optimal prescaler value
  float prescale = (float)(F_CPU >> 16) / spindle.freq;
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

  // Compute duty cycle
  spindle.duty =
    speed * (spindle.max_duty - spindle.min_duty) + spindle.min_duty;

  // Configure clock
  TIMER_PWM.CTRLB = TC1_CCAEN_bm | TC_WGMODE_SINGLESLOPE_gc;
  TIMER_PWM.CCA = TIMER_PWM.PER * spindle.duty;
}


void pwm_spindle_init() {
  // Configure IO
  _set_dir(true);
  _set_enable(false);

  // PWM output
  OUTCLR_PIN(SPIN_PWM_PIN);
  DIRSET_PIN(SPIN_PWM_PIN);
}


void pwm_spindle_deinit() {
  _set_enable(false);

  // Float PWM output pin
  DIRCLR_PIN(SPIN_PWM_PIN);

  // Disable clock
  TIMER_PWM.CTRLA = 0;
}


void pwm_spindle_set(float speed) {
  if (speed) _set_dir(0 < speed);
  spindle.speed = fabs(speed);
  _update_pwm();
}


float pwm_spindle_get() {return spindle.speed;}
void pwm_spindle_stop() {pwm_spindle_set(0);}


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
uint16_t get_pwm_freq() {return spindle.freq;}
void set_pwm_freq(uint16_t value) {spindle.freq = value; _update_pwm();}
bool get_pwm_invert() {return PINCTRL_PIN(SPIN_PWM_PIN) & PORT_INVEN_bm;}


void set_pwm_invert(bool invert) {
  if (invert) PINCTRL_PIN(SPIN_PWM_PIN) |= PORT_INVEN_bm;
  else PINCTRL_PIN(SPIN_PWM_PIN) &= ~PORT_INVEN_bm;
}
