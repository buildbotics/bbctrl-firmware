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
#include "gpio.h"
#include "hardware.h"
#include "pwm.h"


typedef struct {
  float frequency;    // base frequency for PWM driver, in Hz
  float cw_speed_lo;  // minimum clockwise spindle speed [0..N]
  float cw_speed_hi;  // maximum clockwise spindle speed
  float cw_phase_lo;  // pwm phase at minimum CW spindle speed, clamped [0..1]
  float cw_phase_hi;  // pwm phase at maximum CW spindle speed, clamped [0..1]
  float ccw_speed_lo; // minimum counter-clockwise spindle speed [0..N]
  float ccw_speed_hi; // maximum counter-clockwise spindle speed
  float ccw_phase_lo; // pwm phase at minimum CCW spindle speed, clamped [0..1]
  float ccw_phase_hi; // pwm phase at maximum CCW spindle speed, clamped
  float phase_off;    // pwm phase when spindle is disabled
} spindle_t;


static spindle_t spindle = {
  .frequency    = SPINDLE_PWM_FREQUENCY,
  .cw_speed_lo  = SPINDLE_CW_SPEED_LO,
  .cw_speed_hi  = SPINDLE_CW_SPEED_HI,
  .cw_phase_lo  = SPINDLE_CW_PHASE_LO,
  .cw_phase_hi  = SPINDLE_CW_PHASE_HI,
  .ccw_speed_lo = SPINDLE_CCW_SPEED_LO,
  .ccw_speed_hi = SPINDLE_CCW_SPEED_HI,
  .ccw_phase_lo = SPINDLE_CCW_PHASE_LO,
  .ccw_phase_hi = SPINDLE_CCW_PHASE_HI,
  .phase_off    = SPINDLE_PWM_PHASE_OFF,
};


/// return PWM phase (duty cycle) for dir and speed
static float _get_spindle_pwm(cmSpindleMode_t mode, float speed) {
  float speed_lo;
  float speed_hi;
  float phase_lo;
  float phase_hi;

  switch (mode) {
  case SPINDLE_CW:
    speed_lo = spindle.cw_speed_lo;
    speed_hi = spindle.cw_speed_hi;
    phase_lo = spindle.cw_phase_lo;
    phase_hi = spindle.cw_phase_hi;
    break;

  case SPINDLE_CCW:
    speed_lo = spindle.ccw_speed_lo;
    speed_hi = spindle.ccw_speed_hi;
    phase_lo = spindle.ccw_phase_lo;
    phase_hi = spindle.ccw_phase_hi;
    break;

  default: return spindle.phase_off;
  }

  // Clamp spindle speed to lo/hi range
  if (speed < speed_lo) speed = speed_lo;
  if (speed > speed_hi) speed = speed_hi;

  // Normalize speed to [0..1]
  speed = (speed - speed_lo) / (speed_hi - speed_lo);
  return speed * (phase_hi - phase_lo) + phase_lo;
}


void pwm_spindle_init() {
  if (spindle.frequency < 0) spindle.frequency = 0;

  pwm_set_freq(PWM_1, spindle.frequency);
  pwm_set_duty(PWM_1, spindle.phase_off);
}


void pwm_spindle_set(cmSpindleMode_t mode, float speed) {
  switch (mode) {
  case SPINDLE_CW:
    gpio_set_bit_on(SPINDLE_BIT);
    gpio_set_bit_off(SPINDLE_DIR);
    break;

  case SPINDLE_CCW:
    gpio_set_bit_on(SPINDLE_BIT);
    gpio_set_bit_on(SPINDLE_DIR);
    break;

  default:
    gpio_set_bit_off(SPINDLE_BIT); // failsafe: any error causes stop
  }

  // PWM spindle control
  pwm_set_duty(PWM_1, _get_spindle_pwm(mode, speed));
}


// TODO implement these
float get_max_spin(int index) {
  return 0;
}


void set_max_spin(int axis, float value) {
}


float get_spin_min_pulse(int index) {
  return 0;
}


void set_spin_min_pulse(int axis, float value) {
}


float get_spin_max_pulse(int index) {
  return 0;
}


void set_spin_max_pulse(int axis, float value) {
}


uint8_t get_spin_polarity(int index) {
  return 0;
}


void set_spin_polarity(int axis, uint8_t value) {
}


float get_spin_up(int index) {
  return 0;
}


void set_spin_up(int axis, float value) {
}


float get_spin_down(int index) {
  return 0;
}


void set_spin_down(int axis, float value) {
}
