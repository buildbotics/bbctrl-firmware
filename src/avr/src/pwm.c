/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#include "pwm.h"

#include "config.h"
#include "estop.h"
#include "io.h"

#include <math.h>

#if defined (__AVR_ATxmega256A3__)
#define TC_WGMODE_SINGLESLOPE_gc TC_WGMODE_SS_gc
#endif


typedef struct {
  bool initialized;
  float freq; // base frequency for PWM driver, in Hz
  float min_duty;
  float max_duty;
  float power;
} pwm_t;


static pwm_t pwm = {0};


static void _set_enable(bool enable) {
  io_set_output(OUTPUT_TOOL_ENABLE, enable);
}


static void _set_dir(bool clockwise) {
  io_set_output(OUTPUT_TOOL_DIRECTION, !clockwise);
}


static void _update_clock(uint16_t period) {
  if (estop_triggered()) period = 0;

  // Disable
  if (!period) {
    TIMER_PWM.CTRLB = 0; // Disable clock control of pin
    OUTCLR_PIN(PWM_PIN);
    _set_enable(false);
    return;
  }
  _set_enable(true);

  // 100% duty
  if (period == 0xffff) {
    TIMER_PWM.CTRLB = 0; // Disable clock control of pin
    OUTSET_PIN(PWM_PIN);
    return;
  }

  // Configure clock
  TIMER_PWM.CTRLB = TC1_CCAEN_bm | TC_WGMODE_SINGLESLOPE_gc;
  TIMER_PWM.CCA = period;
}


static float _compute_duty(float power) {
  power = fabsf(power);
  if (!power) return 0; // 0% duty
  if (power == 1 && pwm.max_duty == 1) return 1; // 100% duty
  return power * (pwm.max_duty - pwm.min_duty) + pwm.min_duty;
}


static uint16_t _compute_period(float duty) {return TIMER_PWM.PER * duty;}


static void _update_pwm() {
  if (pwm.initialized)
    _update_clock(_compute_period(_compute_duty(pwm.power)));
}


static void _update_freq() {
  // Set clock period and optimal prescaler value
  float prescale = (F_CPU >> 16) / pwm.freq;

  if (prescale <= 1) {
    TIMER_PWM.PER = F_CPU / pwm.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV1_gc;

  } else if (prescale <= 2) {
    TIMER_PWM.PER = F_CPU / 2 / pwm.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV2_gc;

  } else if (prescale <= 4) {
    TIMER_PWM.PER = F_CPU / 4 / pwm.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV4_gc;

  } else if (prescale <= 8) {
    TIMER_PWM.PER = F_CPU / 8 / pwm.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV8_gc;

  } else if (prescale <= 64) {
    TIMER_PWM.PER = F_CPU / 64 / pwm.freq;
    TIMER_PWM.CTRLA = TC_CLKSEL_DIV64_gc;

  } else TIMER_PWM.CTRLA = 0;

  _update_pwm();
}


void pwm_init() {
  pwm.initialized = true;
  pwm.power = 0;

  // Configure IO
  _set_dir(true);
  _set_enable(false);
  _update_freq();

  // PWM output
  OUTCLR_PIN(PWM_PIN);
  DIRSET_PIN(PWM_PIN);
}


float pwm_get() {return pwm.power;}


void pwm_deinit(deinit_cb_t cb) {
  pwm.initialized = false;

  _set_enable(false);

  // Float PWM output pin
  DIRCLR_PIN(PWM_PIN);

  // Disable clock
  TIMER_PWM.CTRLA = 0;

  cb();
}


power_update_t pwm_get_update(float power) {
  power_update_t update = {
    0 <= power ? POWER_FORWARD : POWER_REVERSE,
    power,
    _compute_period(_compute_duty(power))
  };

  return update;
}


// Called from hi-priority stepper interrupt, must be very fast
void pwm_update(const power_update_t &update) {
  if (!pwm.initialized || update.state == POWER_IGNORE) return;
  _update_clock(update.period);
  if (update.period) _set_dir(update.state == POWER_FORWARD);
  pwm.power = update.power;
}


// Var callbacks
float get_pwm_min_duty() {return pwm.min_duty * 100;}


void set_pwm_min_duty(float value) {
  pwm.min_duty = value * 0.01;
  _update_pwm();
}


float get_pwm_max_duty() {return pwm.max_duty * 100;}


void set_pwm_max_duty(float value) {
  pwm.max_duty = value * 0.01;
  _update_pwm();
}


float get_pwm_duty() {return _compute_duty(pwm.power);}
float get_pwm_freq() {return pwm.freq;}


void set_pwm_freq(float value) {
  if (value < 8) value = 8;
  if (320000 < value) value = 320000;
  pwm.freq = value;
  _update_freq();
}


bool get_pwm_invert() {return PINCTRL_PIN(PWM_PIN) & PORT_INVEN_bm;}


void set_pwm_invert(bool invert) {
  if (invert) PINCTRL_PIN(PWM_PIN) |= PORT_INVEN_bm;
  else PINCTRL_PIN(PWM_PIN) &= ~PORT_INVEN_bm;
}
