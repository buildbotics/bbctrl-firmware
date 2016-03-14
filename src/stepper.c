/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2013 - 2015 Robert Giseburt
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

// This module provides the low-level stepper drivers and some related
// functions. See stepper.h for a detailed explanation of this module.

#include "stepper.h"

#include "config.h"
#include "encoder.h"
#include "hardware.h"
#include "util.h"
#include "rtc.h"
#include "report.h"
#include "cpp_magic.h"
#include "usart.h"

#include "plan/planner.h"

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>


#define TIMER_CC_BM(x) CAT3(TC1_, x, EN_bm)


enum {MOTOR_1, MOTOR_2, MOTOR_3, MOTOR_4};


stConfig_t st_cfg;
stPrepSingleton_t st_pre;
static stRunSingleton_t st_run;


static void _load_move();
static void _request_load_move();
static void _update_steps_per_unit(int motor);


/* Initialize stepper motor subsystem
 *
 * Notes:
 *   - This init requires sys_init() to be run beforehand
 *   - microsteps are setup during config_init()
 *   - motor polarity is setup during config_init()
 *   - high level interrupts must be enabled in main() once all inits are
 *     complete
 */
void stepper_init() {
  memset(&st_run, 0, sizeof(st_run)); // clear all values, pointers and status

  // Setup ports
  for (uint8_t i = 0; i < MOTORS; i++) {
    hw.st_port[i]->DIR = MOTOR_PORT_DIR_gm;
    hw.st_port[i]->OUTSET = MOTOR_ENABLE_BIT_bm; // disable motor
  }

  // Setup step timer
  TIMER_STEP.CTRLA = STEP_TIMER_DISABLE;        // turn timer off
  TIMER_STEP.CTRLB = STEP_TIMER_WGMODE;         // waveform mode
  TIMER_STEP.INTCTRLA = STEP_TIMER_INTLVL;       // interrupt mode

  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;

  // Defaults
  st_cfg.motor_power_timeout = MOTOR_IDLE_TIMEOUT;

  st_cfg.mot[MOTOR_1].motor_map  = M1_MOTOR_MAP;
  st_cfg.mot[MOTOR_1].step_angle = M1_STEP_ANGLE;
  st_cfg.mot[MOTOR_1].travel_rev = M1_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_1].microsteps = M1_MICROSTEPS;
  st_cfg.mot[MOTOR_1].polarity   = M1_POLARITY;
  st_cfg.mot[MOTOR_1].power_mode = M1_POWER_MODE;
  st_cfg.mot[MOTOR_1].timer      = (TC0_t *)&M1_TIMER;

  st_cfg.mot[MOTOR_2].motor_map  = M2_MOTOR_MAP;
  st_cfg.mot[MOTOR_2].step_angle = M2_STEP_ANGLE;
  st_cfg.mot[MOTOR_2].travel_rev = M2_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_2].microsteps = M2_MICROSTEPS;
  st_cfg.mot[MOTOR_2].polarity   = M2_POLARITY;
  st_cfg.mot[MOTOR_2].power_mode = M2_POWER_MODE;
  st_cfg.mot[MOTOR_2].timer      = &M2_TIMER;

  st_cfg.mot[MOTOR_3].motor_map  = M3_MOTOR_MAP;
  st_cfg.mot[MOTOR_3].step_angle = M3_STEP_ANGLE;
  st_cfg.mot[MOTOR_3].travel_rev = M3_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_3].microsteps = M3_MICROSTEPS;
  st_cfg.mot[MOTOR_3].polarity   = M3_POLARITY;
  st_cfg.mot[MOTOR_3].power_mode = M3_POWER_MODE;
  st_cfg.mot[MOTOR_3].timer      = &M3_TIMER;

  st_cfg.mot[MOTOR_4].motor_map  = M4_MOTOR_MAP;
  st_cfg.mot[MOTOR_4].step_angle = M4_STEP_ANGLE;
  st_cfg.mot[MOTOR_4].travel_rev = M4_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_4].microsteps = M4_MICROSTEPS;
  st_cfg.mot[MOTOR_4].polarity   = M4_POLARITY;
  st_cfg.mot[MOTOR_4].power_mode = M4_POWER_MODE;
  st_cfg.mot[MOTOR_4].timer      = &M4_TIMER;

  // Setup motor timers
  M1_TIMER.CTRLB = TC_WGMODE_FRQ_gc | TIMER_CC_BM(M1_TIMER_CC);
  M2_TIMER.CTRLB = TC_WGMODE_FRQ_gc | TIMER_CC_BM(M2_TIMER_CC);
  M3_TIMER.CTRLB = TC_WGMODE_FRQ_gc | TIMER_CC_BM(M3_TIMER_CC);
  M4_TIMER.CTRLB = TC_WGMODE_FRQ_gc | TIMER_CC_BM(M4_TIMER_CC);

  // Setup special interrupt for X-axis mapping
  M1_TIMER.INTCTRLB = TC_CCAINTLVL_HI_gc;

  // Init steps per unit
  for (int motor = 0; motor < MOTORS; motor++)
    _update_steps_per_unit(motor);

  st_reset(); // reset steppers to known state
}


/// Return true if motors or dwell are running
uint8_t st_runtime_isbusy() {return st_run.busy;}


/// Reset stepper internals
void st_reset() {
  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    st_pre.mot[motor].prev_direction = STEP_INITIAL_DIRECTION;
    st_pre.mot[motor].timer_clock = 0;
    st_pre.mot[motor].timer_period = 0;
    st_pre.mot[motor].steps = 0;
    st_pre.mot[motor].corrected_steps = 0; // diagnostic only - no effect
  }

  mp_set_steps_to_runtime_position();
}


/// returns 1 if motor is enabled (motor is actually active low)
static uint8_t _motor_is_enabled(uint8_t motor) {
  uint8_t port = motor < MOTORS ? hw.st_port[motor]->OUT : 0xff;
  return port & MOTOR_ENABLE_BIT_bm ? 0 : 1;
}


/// Remove power from a motor
static void _deenergize_motor(const uint8_t motor) {
  if (motor < MOTORS) hw.st_port[motor]->OUTSET = MOTOR_ENABLE_BIT_bm;
  st_run.mot[motor].power_state = MOTOR_OFF;
}


/// Apply power to a motor
static void _energize_motor(const uint8_t motor) {
  if (st_cfg.mot[motor].power_mode == MOTOR_DISABLED) _deenergize_motor(motor);

  else {
    if (motor < MOTORS) hw.st_port[motor]->OUTCLR = MOTOR_ENABLE_BIT_bm;
    st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;
  }
}


/// Apply power to all motors
void st_energize_motors() {
  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    _energize_motor(motor);
    st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;
  }
}


/// Remove power from all motors
void st_deenergize_motors() {
  for (uint8_t motor = 0; motor < MOTORS; motor++)
    _deenergize_motor(motor);
}



/// Callback to manage motor power sequencing
/// Handles motor power-down timing, low-power idle, and adaptive motor power
stat_t st_motor_power_callback() { // called by controller
  // Manage power for each motor individually
  for (int motor = 0; motor < MOTORS; motor++)
    switch (st_cfg.mot[motor].power_mode) {
    case MOTOR_ALWAYS_POWERED:
      if (!_motor_is_enabled(motor)) _energize_motor(motor);
      break;

    case MOTOR_POWERED_IN_CYCLE: case MOTOR_POWERED_ONLY_WHEN_MOVING:
      if (st_run.mot[motor].power_state == MOTOR_POWER_TIMEOUT_START) {
        // Start a countdown
        st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_COUNTDOWN;
        st_run.mot[motor].power_systick = rtc_get_time() +
          (st_cfg.motor_power_timeout * 1000);
      }

      // Run the countdown if not in feedhold and in a countdown
      if (cm_get_combined_state() != COMBINED_HOLD &&
          st_run.mot[motor].power_state == MOTOR_POWER_TIMEOUT_COUNTDOWN &&
          rtc_get_time() > st_run.mot[motor].power_systick) {
        st_run.mot[motor].power_state = MOTOR_IDLE;
        _deenergize_motor(motor);
        report_request(); // request a status report when motors shut down
      }
      break;

    default: _deenergize_motor(motor); break; // Motor disabled
    }

  return STAT_OK;
}


/// Special interrupt for X-axis
ISR(TCE1_CCA_vect) {
  PORT_MOTOR_1.OUTTGL = STEP_BIT_bm;
}


/// Step timer interrupt routine
ISR(STEP_TIMER_ISR) {
  if (st_run.move_type == MOVE_TYPE_DWELL && --st_run.dwell) return;
  st_run.busy = false;
  _load_move();
}


/// "Software" interrupt to request move execution
void st_request_exec_move() {
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {
    ADCB_CH0_INTCTRL = ADC_CH_INTLVL_LO_gc; // trigger LO interrupt
    ADCB_CTRLA = ADC_ENABLE_bm | ADC_CH0START_bm;
  }
}


/// Interrupt handler for calling move exec function.
/// Uses ADC channel 0 as software interrupt.
ISR(ADCB_CH0_vect) {
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC &&
      mp_exec_move() != STAT_NOOP) {
    st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // flip it back
    _request_load_move();
  }
}


/// Fires a "software" interrupt to request to load a move
static void _request_load_move() {
  if (st_runtime_isbusy()) return; // don't request a load if runtime is busy

  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_LOADER) {
    ADCB_CH1_INTCTRL = ADC_CH_INTLVL_HI_gc; // trigger HI interrupt
    ADCB_CTRLA = ADC_ENABLE_bm | ADC_CH1START_bm;
  }
}


/// Interrupt handler for running the loader
ISR(ADCB_CH1_vect) {
  // Use ADC channel 1 as software interrupt.
  // _load_move() can only be called be called from an ISR at the same or
  // higher level as the step timer ISR. A software interrupt is used to allow a
  // non-ISR to request a load. See st_request_load_move()
  _load_move();
}


static inline void _load_motor_move(int motor) {
  stRunMotor_t *run_mot = &st_run.mot[motor];
  stPrepMotor_t *pre_mot = &st_pre.mot[motor];
  const cfgMotor_t *cfg_mot = &st_cfg.mot[motor];

  // Set or zero runtime clock and period
  cfg_mot->timer->CTRLFCLR = TC0_DIR_bm; // Count up
  cfg_mot->timer->CNT = 0; // Start at zero
  cfg_mot->timer->CCA = pre_mot->timer_period;  // Set frequency
  cfg_mot->timer->CTRLA = pre_mot->timer_clock; // Start or stop

  // If motor has 0 steps the following is all skipped. This ensures that
  // state comparisons always operate on the last segment actually run by
  // this motor, regardless of how many segments it may have been inactive
  // in between.
  if (pre_mot->timer_clock) {
    // Detect direction change and set the direction bit in hardware.
    if (pre_mot->direction != pre_mot->prev_direction) {
      pre_mot->prev_direction = pre_mot->direction;

      if (pre_mot->direction == DIRECTION_CW)
        hw.st_port[motor]->OUTCLR = DIRECTION_BIT_bm;
      else hw.st_port[motor]->OUTSET = DIRECTION_BIT_bm;
    }

    // Accumulate encoder
    en[motor].encoder_steps += pre_mot->steps * pre_mot->step_sign;
    pre_mot->steps = 0;
  }

  // Energize motor and start power management
  if ((pre_mot->timer_clock && cfg_mot->power_mode != MOTOR_DISABLED) ||
      cfg_mot->power_mode == MOTOR_POWERED_IN_CYCLE) {
    hw.st_port[motor]->OUTCLR = MOTOR_ENABLE_BIT_bm;  // energize motor
    run_mot->power_state = MOTOR_POWER_TIMEOUT_START; // start power management
  }
}


/* Dequeue move and load into stepper struct
 *
 * This routine can only be called from an ISR at the same or
 * higher level as the step timer ISR. A software interrupt has been
 * provided to allow a non-ISR to request a load (see st_request_load_move())
 *
 * In ALINE code:
 *   - All axes must set steps and compensate for out-of-range pulse phasing.
 *   - If axis has 0 steps the direction setting can be omitted
 *   - If axis has 0 steps the motor must not be enabled to support power
 *     mode = 1
 */
static void _load_move() {
  if (st_runtime_isbusy()) return;

  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_LOADER) {
    // There are no more moves, disable motor clocks
    for (int motor = 0; motor < MOTORS; motor++)
      st_cfg.mot[motor].timer->CTRLA = 0;
    return;
  }

  st_run.move_type = st_pre.move_type;

  switch (st_pre.move_type) {
  case MOVE_TYPE_DWELL:
    st_run.dwell = st_pre.dwell;
    // Fall through

  case MOVE_TYPE_ALINE:
    for (int motor = 0; motor < MOTORS; motor++)
      if (st_pre.move_type == MOVE_TYPE_ALINE) _load_motor_move(motor);
      else st_pre.mot[motor].timer_clock = 0; // Off

    st_run.busy = true;
    TIMER_STEP.PER = st_pre.seg_period;
    TIMER_STEP.CTRLA = STEP_TIMER_ENABLE; // enable step timer, if not enabled
    break;

  case MOVE_TYPE_COMMAND: // handle synchronous commands
    mp_runtime_command(st_pre.bf);
    // Fall through

  default:
    TIMER_STEP.CTRLA = STEP_TIMER_DISABLE;
    break;
  }

  // we are done with the prep buffer - flip the flag back
  st_pre.move_type = MOVE_TYPE_0;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;
  st_request_exec_move(); // exec and prep next move
}


static void _prep_motor_line(int motor, float travel_steps, float error) {
  stPrepMotor_t *pre_mot = &st_pre.mot[motor];
  cfgMotor_t *cfg_mot = &st_cfg.mot[motor];

  // Disable this motor's clock if there are no new steps
  if (fp_ZERO(travel_steps)) {
    pre_mot->timer_clock = 0; // Off
    return;
  }

  // Setup the direction, compensating for polarity.
  // Set the step_sign which is used by the stepper ISR to accumulate step
  // position
  if (0 <= travel_steps) { // positive direction
    pre_mot->direction = DIRECTION_CW ^ cfg_mot->polarity;
    pre_mot->step_sign = 1;

  } else {
    pre_mot->direction = DIRECTION_CCW ^ cfg_mot->polarity;
    pre_mot->step_sign = -1;
  }

#ifdef __STEP_CORRECTION
  float correction;

  // 'Nudge' correction strategy. Inject a single, scaled correction value
  // then hold off
  if (--pre_mot->correction_holdoff < 0 &&
      STEP_CORRECTION_THRESHOLD < fabs(error)) {

    pre_mot->correction_holdoff = STEP_CORRECTION_HOLDOFF;
    correction = error * STEP_CORRECTION_FACTOR;

    if (0 < correction)
      correction = min3(correction, fabs(travel_steps), STEP_CORRECTION_MAX);
    else correction =
           max3(correction, -fabs(travel_steps), -STEP_CORRECTION_MAX);

    pre_mot->corrected_steps += correction;
    travel_steps -= correction;
  }
#endif

  // Compute motor timer clock and period. Rounding is performed to eliminate
  // a negative bias in the uint32_t conversion that results in long-term
  // negative drift.
  uint16_t steps = round(fabs(travel_steps));
  uint32_t seg_clocks = (uint32_t)st_pre.seg_period * STEP_TIMER_DIV;
  uint32_t ticks_per_step = seg_clocks / (steps + 0.5);

  // Find the right clock rate
  if (ticks_per_step & 0xffff0000UL) {
    ticks_per_step /= 2;
    seg_clocks /= 2;

    if (ticks_per_step & 0xffff0000UL) {
      ticks_per_step /= 2;
      seg_clocks /= 2;

      if (ticks_per_step & 0xffff0000UL) {
        ticks_per_step /= 2;
        seg_clocks /= 2;

        if (ticks_per_step & 0xffff0000UL) pre_mot->timer_clock = 0; // Off
        else pre_mot->timer_clock = TC_CLKSEL_DIV8_gc;
      } else pre_mot->timer_clock = TC_CLKSEL_DIV4_gc;
    } else pre_mot->timer_clock = TC_CLKSEL_DIV2_gc;
  } else pre_mot->timer_clock = TC_CLKSEL_DIV1_gc;

  pre_mot->timer_period = ticks_per_step * 2; // TODO why do we need *2 here?
  pre_mot->steps = seg_clocks / ticks_per_step;
}


/* Prepare the next move for the loader
 *
 * This function does the math on the next pulse segment and gets it ready for
 * the loader. It deals with all the optimizations and timer setups so that
 * loading can be performed as rapidly as possible. It works in joint space
 * (motors) and it works in steps, not length units. All args are provided as
 * floats and converted to their appropriate integer types for the loader.
 *
 * Args:
 *   - travel_steps[] are signed relative motion in steps for each motor. Steps
 *     are floats that typically have fractional values (fractional steps). The
 *     sign indicates direction. Motors that are not in the move should be 0
 *     steps on input.
 *
 *   - error[] is a vector of measured errors to the step count. Used for
 *     correction.
 *
 *   - segment_time - how many minutes the segment should run. If timing is not
 *     100% accurate this will affect the move velocity, but not the distance
 *     traveled.
 */
stat_t st_prep_line(float travel_steps[], float error[], float segment_time) {
  // Trap conditions that would prevent queueing the line
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC)
    return cm_hard_alarm(STAT_INTERNAL_ERROR);
  if (isinf(segment_time))
    return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_INFINITE);
  if (isnan(segment_time))
    return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_NAN);
  if (segment_time < EPSILON) return STAT_MINIMUM_TIME_MOVE;

  // Setup segment parameters (need st_pre.seg_period for motor calcs)
  st_pre.seg_period = segment_time * 60 * F_CPU / STEP_TIMER_DIV;
  st_pre.move_type = MOVE_TYPE_ALINE;

  // Setup motor parameters
  for (uint8_t motor = 0; motor < MOTORS; motor++)
    _prep_motor_line(motor, travel_steps[motor], error[motor]);

  // Signal prep buffer ready (do this last)
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;

  return STAT_OK;
}


/// Keeps the loader happy. Otherwise performs no action
void st_prep_null() {
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC)
    cm_hard_alarm(STAT_INTERNAL_ERROR);

  st_pre.move_type = MOVE_TYPE_0;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC; // signal prep buffer empty
}


/// Stage command to execution
void st_prep_command(void *bf) {
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC)
    cm_hard_alarm(STAT_INTERNAL_ERROR);

  st_pre.move_type = MOVE_TYPE_COMMAND;
  st_pre.bf = (mpBuf_t *)bf;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // signal prep buffer ready
}


/// Add a dwell to the move buffer
void st_prep_dwell(float seconds) {
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC)
    cm_hard_alarm(STAT_INTERNAL_ERROR);

  st_pre.move_type = MOVE_TYPE_DWELL;
  st_pre.seg_period = F_CPU / STEP_TIMER_DIV / 1000; // 1 ms
  st_pre.dwell = seconds * 1000; // convert to ms
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // signal prep buffer ready
}


float get_ang(int index) {return st_cfg.mot[index].step_angle;}
float get_trvl(int index) {return st_cfg.mot[index].travel_rev;}
uint16_t get_mstep(int index) {return st_cfg.mot[index].microsteps;}
bool get_pol(int index) {return st_cfg.mot[index].polarity;}
uint16_t get_mvel(int index) {return 0;}
uint16_t get_mjerk(int index) {return 0;}


static void _update_steps_per_unit(int motor) {
  st_cfg.mot[motor].steps_per_unit =
    (360 * st_cfg.mot[motor].microsteps) /
    (st_cfg.mot[motor].travel_rev * st_cfg.mot[motor].step_angle);
}


void set_ang(int index, float value) {
  st_cfg.mot[index].step_angle = value;
  _update_steps_per_unit(index);
}


void set_trvl(int index, float value) {
  st_cfg.mot[index].travel_rev = value;
  _update_steps_per_unit(index);
}


void set_mstep(int index, uint16_t value) {
  switch (value) {
  case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
    break;
  default: return;
  }

  st_cfg.mot[index].microsteps = value;
  _update_steps_per_unit(index);
}


void set_pol(int index, bool value) {
  st_cfg.mot[index].polarity = value;
}


void set_mvel(int index, uint16_t value) {
}


void set_mjerk(int index, uint16_t value) {
}
