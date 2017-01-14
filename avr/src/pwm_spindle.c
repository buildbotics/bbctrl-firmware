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


#include "pwm_spindle.h"

#include "config.h"


typedef struct {
  uint16_t freq;    // base frequency for PWM driver, in Hz
  float min_rpm;
  float max_rpm;
  float min_duty;
  float max_duty;
  bool reverse;
  bool enable_invert;
  bool estop;
} pwm_spindle_t;


static pwm_spindle_t spindle = {
  .freq          = SPINDLE_PWM_FREQUENCY,
  .min_rpm       = SPINDLE_MIN_RPM,
  .max_rpm       = SPINDLE_MAX_RPM,
  .min_duty      = SPINDLE_MIN_DUTY,
  .max_duty      = SPINDLE_MAX_DUTY,
  .reverse       = SPINDLE_REVERSE,
  .enable_invert = false,
  .estop         = false,
};


static void _spindle_set_pwm(spindle_mode_t mode, float speed) {
  if (mode == SPINDLE_OFF || speed < spindle.min_rpm || spindle.estop) {
    TIMER_PWM.CTRLA = 0;
    return;
  }

  // Clamp speed
  if (spindle.max_rpm < speed) speed = spindle.max_rpm;

  // Set clock period and optimal prescaler value
  float prescale = F_CPU / 65536.0 / spindle.freq;
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

  // Map RPM to duty cycle
  float duty = (speed - spindle.min_rpm) / (spindle.max_rpm - spindle.min_rpm) *
    (spindle.max_duty - spindle.min_duty) + spindle.min_duty;

  TIMER_PWM.CCB = TIMER_PWM.PER * duty;
}


static void _spindle_set_enable(bool enable) {
  if (enable ^ spindle.enable_invert)
    PORT(SPIN_ENABLE_PIN)->OUTSET = BM(SPIN_ENABLE_PIN);
  else PORT(SPIN_ENABLE_PIN)->OUTCLR = BM(SPIN_ENABLE_PIN);
}


static void _spindle_set_dir(bool forward) {
  if (forward ^ spindle.reverse) PORT(SPIN_DIR_PIN)->OUTCLR = BM(SPIN_DIR_PIN);
  else PORT(SPIN_DIR_PIN)->OUTSET = BM(SPIN_DIR_PIN);
}


void pwm_spindle_init() {
  // Configure IO
  PORT(SPIN_PWM_PIN)->DIRSET = BM(SPIN_PWM_PIN); // PWM Output
  _spindle_set_dir(true);
  PORT(SPIN_DIR_PIN)->DIRSET = BM(SPIN_DIR_PIN); // Dir Output
  _spindle_set_enable(false);
  PORT(SPIN_ENABLE_PIN)->DIRSET = BM(SPIN_ENABLE_PIN); // Enable output

  // Configure clock
  TIMER_PWM.CTRLB = TC1_CCBEN_bm | TC_WGMODE_SINGLESLOPE_gc;
}


void pwm_spindle_set(spindle_mode_t mode, float speed) {
  _spindle_set_dir(mode == SPINDLE_CW);
  _spindle_set_pwm(mode, speed);
  _spindle_set_enable(mode != SPINDLE_OFF && TIMER_PWM.CTRLA);
}


void pwm_spindle_estop() {
  spindle.estop = true;
  _spindle_set_pwm(SPINDLE_OFF, 0);
}


// TODO these need more effort and should work with the huanyang spindle too
float get_max_spin(int index) {return spindle.max_rpm;}
void set_max_spin(int axis, float value) {spindle.max_rpm = value;}
float get_min_spin(int index) {return spindle.min_rpm;}
void set_min_spin(int axis, float value) {spindle.min_rpm = value;}
float get_spin_min_pulse(int index) {return spindle.min_duty;}
void set_spin_min_pulse(int axis, float value) {spindle.min_duty = value;}
float get_spin_max_pulse(int index) {return spindle.max_duty;}
void set_spin_max_pulse(int axis, float value) {spindle.max_duty = value;}
uint8_t get_spin_reverse(int index) {return spindle.reverse;}
void set_spin_reverse(int axis, uint8_t value) {spindle.reverse = value;}
float get_spin_up(int index) {return 0;}
void set_spin_up(int axis, float value) {}
float get_spin_down(int index) {return 0;}
void set_spin_down(int axis, float value) {}
