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

#pragma once


#include "config.h"
#include "status.h"

#include <avr/interrupt.h>


#define PWM_1        0
#define PWM_2        1


typedef struct pwmConfigChannel {
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
} pwmConfigChannel_t;


typedef struct pwmChannel {
  uint8_t ctrla; // byte needed to active CTRLA (it's dynamic - rest are static)
  TC1_t *timer;  // assumes TC1 flavor timers used for PWM channels
} pwmChannel_t;


typedef struct pwmSingleton {
  pwmConfigChannel_t c[PWMS]; // array of channel configs
  pwmChannel_t p[PWMS];       // array of PWM channels
} pwmSingleton_t;

extern pwmSingleton_t pwm;


void pwm_init();
stat_t pwm_set_freq(uint8_t channel, float freq);
stat_t pwm_set_duty(uint8_t channel, float duty);
