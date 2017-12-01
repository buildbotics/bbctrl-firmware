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


#include "pwm_spindle.h"

#include "config.h"
#include "estop.h"
#include "outputs.h"

#include <math.h>


typedef struct {
  uint16_t freq;    // base frequency for PWM driver, in Hz
  float min_rpm;
  float max_rpm;
  float min_duty;
  float max_duty;
  bool pwm_invert;
} pwm_spindle_t;


static pwm_spindle_t spindle = {0};


static void _set_enable(bool enable) {
  outputs_set_active(TOOL_ENABLE_PIN, enable);
}


static void _set_dir(bool clockwise) {
  outputs_set_active(TOOL_DIR_PIN, !clockwise);
}


static void _set_pwm(float speed) {
  if (speed < spindle.min_rpm || estop_triggered()) {
    TIMER_PWM.CTRLA = 0;
    OUTCLR_PIN(SPIN_PWM_PIN);
    _set_enable(false);
    return;
  }
  _set_enable(true);

  // Invert PWM
  if (spindle.pwm_invert) PINCTRL_PIN(SPIN_PWM_PIN) |= PORT_INVEN_bm;
  else PINCTRL_PIN(SPIN_PWM_PIN) &= ~PORT_INVEN_bm;

  // 100% duty
  if (spindle.max_rpm <= speed && spindle.max_duty == 1) {
    TIMER_PWM.CTRLB = 0;
    OUTSET_PIN(SPIN_PWM_PIN);
    return;
  }

  // Clamp speed
  if (spindle.max_rpm < speed) speed = spindle.max_rpm;
  if (speed < spindle.min_rpm) speed = 0;

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

  // Map RPM to duty cycle
  float duty = (speed - spindle.min_rpm) / (spindle.max_rpm - spindle.min_rpm) *
    (spindle.max_duty - spindle.min_duty) + spindle.min_duty;

  // Configure clock
  TIMER_PWM.CTRLB = TC1_CCAEN_bm | TC_WGMODE_SINGLESLOPE_gc;
  TIMER_PWM.CCA = TIMER_PWM.PER * duty;
}


void pwm_spindle_init() {
  // Configure IO
  _set_dir(true);
  _set_enable(false);

  DIRSET_PIN(SPIN_PWM_PIN);    // Output
}


void pwm_spindle_set(float speed) {
  if (speed) _set_dir(0 < speed);
  _set_pwm(fabs(speed));
}


void pwm_spindle_stop() {pwm_spindle_set(0);}


// TODO these need more effort and should work with the huanyang spindle too
float get_max_spin() {return spindle.max_rpm;}
void set_max_spin(float value) {spindle.max_rpm = value;}
float get_min_spin() {return spindle.min_rpm;}
void set_min_spin(float value) {spindle.min_rpm = value;}
float get_spin_min_duty() {return spindle.min_duty * 100;}
void set_spin_min_duty(float value) {spindle.min_duty = value / 100;}
float get_spin_max_duty() {return spindle.max_duty * 100;}
void set_spin_max_duty(float value) {spindle.max_duty = value / 100;}
float get_spin_up() {return 0;}    // TODO
void set_spin_up(float value) {}   // TODO
float get_spin_down() {return 0;}  // TODO
void set_spin_down(float value) {} // TODO
uint16_t get_spin_freq() {return spindle.freq;}
void set_spin_freq(uint16_t value) {spindle.freq = value;}
bool get_pwm_invert() {return spindle.pwm_invert;}
void set_pwm_invert(bool value) {spindle.pwm_invert = value;}
