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

pwmSingleton_t pwm;


// defines common to all PWM channels
#define PWM_TIMER_t    TC1_t              // PWM uses TC1's
#define PWM_TIMER_DISABLE 0               // turn timer off (clock = 0 Hz)
#define PWM_MAX_FREQ (F_CPU / 256)        // with 8-bits duty cycle precision
#define PWM_MIN_FREQ (F_CPU / 64 / 65536) // min frequency for prescaling


/* CLKSEL is used to configure default PWM clock operating ranges
 * These can be changed by pwm_freq() depending on the PWM frequency selected
 *
 * The useful ranges (assuming a 32 Mhz system clock) are:
 *   TC_CLKSEL_DIV1_gc  - good for about 500 Hz to 125 Khz practical upper limit
 *   TC_CLKSEL_DIV2_gc  - good for about 250 Hz to  62 KHz
 *   TC_CLKSEL_DIV4_gc  - good for about 125 Hz to  31 KHz
 *   TC_CLKSEL_DIV8_gc  - good for about  62 Hz to  16 KHz
 *   TC_CLKSEL_DIV64_gc - good for about   8 Hz to   2 Khz
 */
#define PWM1_CTRLA_CLKSEL TC_CLKSEL_DIV1_gc  // starting clock select value


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

  // setup PWM channel 1
  // clear parent structure
  memset(&pwm.p[PWM_1], 0, sizeof(pwmChannel_t));

  // bind timer struct to PWM struct array
  pwm.p[PWM_1].timer = &TIMER_PWM1;
  // initialize starting clock operating range
  pwm.p[PWM_1].ctrla = PWM1_CTRLA_CLKSEL;
  pwm.p[PWM_1].timer->CTRLB = PWM1_CTRLB;

  // setup PWM channel 2
  // clear all values, pointers and status
  memset(&pwm.p[PWM_2], 0, sizeof(pwmChannel_t));

  pwm.p[PWM_2].timer = &TIMER_PWM2;
  pwm.p[PWM_2].ctrla = PWM2_CTRLA_CLKSEL;
  pwm.p[PWM_2].timer->CTRLB = PWM2_CTRLB;

  pwm.c[PWM_1].frequency =    P1_PWM_FREQUENCY;
  pwm.c[PWM_1].cw_speed_lo =  P1_CW_SPEED_LO;
  pwm.c[PWM_1].cw_speed_hi =  P1_CW_SPEED_HI;
  pwm.c[PWM_1].cw_phase_lo =  P1_CW_PHASE_LO;
  pwm.c[PWM_1].cw_phase_hi =  P1_CW_PHASE_HI;
  pwm.c[PWM_1].ccw_speed_lo = P1_CCW_SPEED_LO;
  pwm.c[PWM_1].ccw_speed_hi = P1_CCW_SPEED_HI;
  pwm.c[PWM_1].ccw_phase_lo = P1_CCW_PHASE_LO;
  pwm.c[PWM_1].ccw_phase_hi = P1_CCW_PHASE_HI;
  pwm.c[PWM_1].phase_off =    P1_PWM_PHASE_OFF;
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
  if (chan > PWMS) return STAT_NO_SUCH_DEVICE;
  if (freq > PWM_MAX_FREQ) return STAT_INPUT_EXCEEDS_MAX_VALUE;
  if (freq < PWM_MIN_FREQ) return STAT_INPUT_LESS_THAN_MIN_VALUE;

  // Set the period and the prescaler
  // optimal non-integer prescaler value
  float prescale = F_CPU / 65536 / freq;
  if (prescale <= 1) {
    pwm.p[chan].timer->PER = F_CPU / freq;
    pwm.p[chan].timer->CTRLA = TC_CLKSEL_DIV1_gc;

  } else if (prescale <= 2) {
    pwm.p[chan].timer->PER = F_CPU / 2 / freq;
    pwm.p[chan].timer->CTRLA = TC_CLKSEL_DIV2_gc;

  } else if (prescale <= 4) {
    pwm.p[chan].timer->PER = F_CPU / 4 / freq;
    pwm.p[chan].timer->CTRLA = TC_CLKSEL_DIV4_gc;

  } else if (prescale <= 8) {
    pwm.p[chan].timer->PER = F_CPU / 8 / freq;
    pwm.p[chan].timer->CTRLA = TC_CLKSEL_DIV8_gc;

  } else {
    pwm.p[chan].timer->PER = F_CPU / 64 / freq;
    pwm.p[chan].timer->CTRLA = TC_CLKSEL_DIV64_gc;
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
  if (duty < 0.0) return STAT_INPUT_LESS_THAN_MIN_VALUE;
  if (duty > 1.0) return STAT_INPUT_EXCEEDS_MAX_VALUE;

  //  Ffrq = Fper / 2N(CCA + 1)
  //  Fpwm = Fper / N(PER + 1)
  float period_scalar = pwm.p[chan].timer->PER;
  pwm.p[chan].timer->CCB = (uint16_t)(period_scalar * duty) + 1;

  return STAT_OK;
}
