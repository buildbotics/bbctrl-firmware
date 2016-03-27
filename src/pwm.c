/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2012 - 2015 Alden S. Hart, Jr.
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

#include "pwm.h"

#include "config.h"
#include "hardware.h"
#include "gpio.h"

#include <string.h>


typedef struct {
  TC1_t *timer;  // assumes TC1 flavor timers used for PWM channels
} pwm_t;


pwm_t pwm[PWMS] = {
  {.timer = &TIMER_PWM1},
  {.timer = &TIMER_PWM2}
};


/* Initialize pwm channels
 *
 * Notes:
 *   - Whatever level interrupts you use must be enabled in main()
 *   - init assumes PWM1 output bit (D5) has been set to output previously
 *     (stepper.c)
 */
void pwm_init() {
  return; // Don't init PWM for now.  TODO fix this

  gpio_set_bit_off(SPINDLE_PWM);

  pwm[PWM_1].timer->CTRLB = PWM1_CTRLB;
  pwm[PWM_2].timer->CTRLB = PWM2_CTRLB;
}


/* Set PWM channel frequency
 *
 * @param channel PWM channel
 * @param freq PWM frequency in Khz as a float
 *
 * Assumes 32MHz clock.
 * Doesn't turn time on until duty cycle is set
 */
stat_t pwm_set_freq(uint8_t chan, float freq) {
  if (PWMS <= chan) return STAT_NO_SUCH_DEVICE;
  if (PWM_MAX_FREQ < freq) return STAT_INPUT_EXCEEDS_MAX_VALUE;
  if (freq < PWM_MIN_FREQ) return STAT_INPUT_LESS_THAN_MIN_VALUE;

  // Set period and optimal prescaler value
  float prescale = F_CPU / 65536 / freq;
  if (prescale <= 1) {
    pwm[chan].timer->PER = F_CPU / freq;
    pwm[chan].timer->CTRLA = TC_CLKSEL_DIV1_gc;

  } else if (prescale <= 2) {
    pwm[chan].timer->PER = F_CPU / 2 / freq;
    pwm[chan].timer->CTRLA = TC_CLKSEL_DIV2_gc;

  } else if (prescale <= 4) {
    pwm[chan].timer->PER = F_CPU / 4 / freq;
    pwm[chan].timer->CTRLA = TC_CLKSEL_DIV4_gc;

  } else if (prescale <= 8) {
    pwm[chan].timer->PER = F_CPU / 8 / freq;
    pwm[chan].timer->CTRLA = TC_CLKSEL_DIV8_gc;

  } else {
    pwm[chan].timer->PER = F_CPU / 64 / freq;
    pwm[chan].timer->CTRLA = TC_CLKSEL_DIV64_gc;
  }

  return STAT_OK;
}


/*
 * Set PWM channel duty cycle
 *
 * @param channel PWM channel
 * @param duty PWM duty cycle from 0% to 100%
 *
 * Setting duty cycle to 0 disables the PWM channel with output low
 * Setting duty cycle to 100 disables the PWM channel with output high
 * Setting duty cycle between 0 and 100 enables PWM channel
 *
 * The frequency must have been set previously
 */
stat_t pwm_set_duty(uint8_t chan, float duty) {
  if (duty < 0) return STAT_INPUT_LESS_THAN_MIN_VALUE;
  if (duty > 1) return STAT_INPUT_EXCEEDS_MAX_VALUE;

  //  Ffrq = Fper / 2N(CCA + 1)
  //  Fpwm = Fper / N(PER + 1)
  float period_scalar = pwm[chan].timer->PER;
  pwm[chan].timer->CCB = period_scalar * duty + 1;

  return STAT_OK;
}
