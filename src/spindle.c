/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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
#include "gpio.h"
#include "hardware.h"
#include "pwm.h"

#include "plan/planner.h"


static void _exec_spindle_control(float *value, float *flag);
static void _exec_spindle_speed(float *value, float *flag);


void cm_spindle_init() {
  if( pwm.c[PWM_1].frequency < 0 )
    pwm.c[PWM_1].frequency = 0;

  pwm_set_freq(PWM_1, pwm.c[PWM_1].frequency);
  pwm_set_duty(PWM_1, pwm.c[PWM_1].phase_off);
}


/// return PWM phase (duty cycle) for dir and speed
float cm_get_spindle_pwm( uint8_t spindle_mode ) {
  float speed_lo=0, speed_hi=0, phase_lo=0, phase_hi=0;
  if (spindle_mode == SPINDLE_CW) {
    speed_lo = pwm.c[PWM_1].cw_speed_lo;
    speed_hi = pwm.c[PWM_1].cw_speed_hi;
    phase_lo = pwm.c[PWM_1].cw_phase_lo;
    phase_hi = pwm.c[PWM_1].cw_phase_hi;

  } else if (spindle_mode == SPINDLE_CCW) {
    speed_lo = pwm.c[PWM_1].ccw_speed_lo;
    speed_hi = pwm.c[PWM_1].ccw_speed_hi;
    phase_lo = pwm.c[PWM_1].ccw_phase_lo;
    phase_hi = pwm.c[PWM_1].ccw_phase_hi;
  }

  if (spindle_mode == SPINDLE_CW || spindle_mode == SPINDLE_CCW) {
    // clamp spindle speed to lo/hi range
    if (cm.gm.spindle_speed < speed_lo) cm.gm.spindle_speed = speed_lo;
    if (cm.gm.spindle_speed > speed_hi) cm.gm.spindle_speed = speed_hi;

    // normalize speed to [0..1]
    float speed = (cm.gm.spindle_speed - speed_lo) / (speed_hi - speed_lo);
    return speed * (phase_hi - phase_lo) + phase_lo;

  } else return pwm.c[PWM_1].phase_off;
}


/// queue the spindle command to the planner buffer
stat_t cm_spindle_control(uint8_t spindle_mode) {
  float value[AXES] = {spindle_mode, 0, 0, 0, 0, 0};

  mp_queue_command(_exec_spindle_control, value, value);

  return STAT_OK;
}

/// execute the spindle command (called from planner)
static void _exec_spindle_control(float *value, float *flag) {
  uint8_t spindle_mode = (uint8_t)value[0];

  cm_set_spindle_mode(MODEL, spindle_mode);

  if (spindle_mode == SPINDLE_CW) {
    gpio_set_bit_on(SPINDLE_BIT);
    gpio_set_bit_off(SPINDLE_DIR);

  } else if (spindle_mode == SPINDLE_CCW) {
    gpio_set_bit_on(SPINDLE_BIT);
    gpio_set_bit_on(SPINDLE_DIR);

  } else gpio_set_bit_off(SPINDLE_BIT); // failsafe: any error causes stop

  // PWM spindle control
  pwm_set_duty(PWM_1, cm_get_spindle_pwm(spindle_mode) );
}

/// Queue the S parameter to the planner buffer
stat_t cm_set_spindle_speed(float speed) {
  float value[AXES] = { speed, 0,0,0,0,0 };
  mp_queue_command(_exec_spindle_speed, value, value);
  return STAT_OK;
}


/// Execute the S command (called from the planner buffer)
void cm_exec_spindle_speed(float speed) {
  cm_set_spindle_speed(speed);
}


/// Spindle speed callback from planner queue
static void _exec_spindle_speed(float *value, float *flag) {
  cm_set_spindle_speed_parameter(MODEL, value[0]);
  // update spindle speed if we're running
  pwm_set_duty(PWM_1, cm_get_spindle_pwm(cm.gm.spindle_mode));
}


float get_max_spin(int index) {
  return 0;
}


void set_max_spin(int axis, float value) {
}


uint8_t get_spindle_type(int index) {
  return 0;
}


void set_spindle_type(int axis, uint8_t value) {
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
