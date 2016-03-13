/*
 * stepper.c - stepper motor controls
 * This file is part of the TinyG project
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
 * Copyright (c) 2013 - 2015 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License, version 2 as published by the Free Software
 * Foundation. You should have received a copy of the GNU General
 * Public License, version 2 along with the software.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software
 * library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with  other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU General
 * Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT
 * WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* This module provides the low-level stepper drivers and some related
 * functions. See stepper.h for a detailed explanation of this module.
 */

#include "stepper.h"

#include "config.h"
#include "encoder.h"
#include "hardware.h"
#include "util.h"
#include "rtc.h"
#include "report.h"

#include "plan/planner.h"

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>


#define DDA_PERIOD (F_CPU / STEP_CLOCK_FREQ)

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

  // Setup DDA timer
  TIMER_DDA.CTRLA = STEP_TIMER_DISABLE;        // turn timer off
  TIMER_DDA.CTRLB = STEP_TIMER_WGMODE;         // waveform mode
  TIMER_DDA.INTCTRLA = TIMER_DDA_INTLVL;       // interrupt mode

  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;

  // Defaults
  st_cfg.motor_power_timeout = MOTOR_IDLE_TIMEOUT;

  st_cfg.mot[MOTOR_1].motor_map =  M1_MOTOR_MAP;
  st_cfg.mot[MOTOR_1].step_angle = M1_STEP_ANGLE;
  st_cfg.mot[MOTOR_1].travel_rev = M1_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_1].microsteps = M1_MICROSTEPS;
  st_cfg.mot[MOTOR_1].polarity =   M1_POLARITY;
  st_cfg.mot[MOTOR_1].power_mode = M1_POWER_MODE;

  st_cfg.mot[MOTOR_2].motor_map =  M2_MOTOR_MAP;
  st_cfg.mot[MOTOR_2].step_angle = M2_STEP_ANGLE;
  st_cfg.mot[MOTOR_2].travel_rev = M2_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_2].microsteps = M2_MICROSTEPS;
  st_cfg.mot[MOTOR_2].polarity =   M2_POLARITY;
  st_cfg.mot[MOTOR_2].power_mode = M2_POWER_MODE;

  st_cfg.mot[MOTOR_3].motor_map =  M3_MOTOR_MAP;
  st_cfg.mot[MOTOR_3].step_angle = M3_STEP_ANGLE;
  st_cfg.mot[MOTOR_3].travel_rev = M3_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_3].microsteps = M3_MICROSTEPS;
  st_cfg.mot[MOTOR_3].polarity =   M3_POLARITY;
  st_cfg.mot[MOTOR_3].power_mode = M3_POWER_MODE;

  st_cfg.mot[MOTOR_4].motor_map =  M4_MOTOR_MAP;
  st_cfg.mot[MOTOR_4].step_angle = M4_STEP_ANGLE;
  st_cfg.mot[MOTOR_4].travel_rev = M4_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_4].microsteps = M4_MICROSTEPS;
  st_cfg.mot[MOTOR_4].polarity =   M4_POLARITY;
  st_cfg.mot[MOTOR_4].power_mode = M4_POWER_MODE;

  // Init steps per unit
  for (int m = 0; m < MOTORS; m++) _update_steps_per_unit(m);

  st_reset(); // reset steppers to known state
}


/// Return true if motors or dwell are running
uint8_t st_runtime_isbusy() {return st_run.dda_ticks_downcount;}


/// Reset stepper internals
void st_reset() {
  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    st_pre.mot[motor].prev_direction = STEP_INITIAL_DIRECTION;
    // will become max negative during per-motor setup;
    st_run.mot[motor].substep_accumulator = 0;
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


static inline void _step_motor(int motor) {
  st_run.mot[motor].substep_accumulator += st_run.mot[motor].substep_increment;

  if (0 < st_run.mot[motor].substep_accumulator) {
    hw.st_port[motor]->OUTTGL = STEP_BIT_bm; // toggle step line
    st_run.mot[motor].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(motor);
  }
}


/// Stepper Interrupt Service Routine
/// DDA timer interrupt routine - service ticks from DDA timer
ISR(TIMER_DDA_ISR_vect) {
  if (st_run.move_type == MOVE_TYPE_ALINE) {
    _step_motor(MOTOR_1);
    _step_motor(MOTOR_2);
    _step_motor(MOTOR_3);
    _step_motor(MOTOR_4);
  }

  if (--st_run.dda_ticks_downcount) return;

  TIMER_DDA.CTRLA = STEP_TIMER_DISABLE; // disable DDA timer
  _load_move();                         // load the next move
}


/// SW interrupt to request to execute a move
void st_request_exec_move() {
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {
    ADCB_CH0_INTCTRL = ADC_CH_INTLVL_LO_gc; // trigger LO interrupt
    ADCB_CTRLA = ADC_ENABLE_bm | ADC_CH0START_bm;
  }
}


/// Interrupt handler for calling exec function
ISR(ADCB_CH0_vect) {
  // Use ADC channel 0 as software interrupt
  // Exec move
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC &&
      mp_exec_move() != STAT_NOOP) {
    st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // flip it back
    _request_load_move();
  }
}


/// Fires a software interrupt (timer) to request to load a move
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
  // higher level as the DDA ISR. A software interrupt is used to allow a
  // non-ISR to request a load. See st_request_load_move()
  _load_move();
}


static inline void _load_motor_move(int motor) {
  stRunMotor_t *run_mot = &st_run.mot[motor];
  stPrepMotor_t *prep_mot = &st_pre.mot[motor];
  cfgMotor_t *cfg_mot = &st_cfg.mot[motor];

  // Set or zero, runtime substep increment
  run_mot->substep_increment = prep_mot->substep_increment;

  if (run_mot->substep_increment) {
    // If motor has 0 steps the following is all skipped. This ensures that
    // state comparisons always operate on the last segment actually run by
    // this motor, regardless of how many segments it may have been inactive
    // in between.

    // Apply accumulator correction if the time base has changed since
    // previous segment
    if (prep_mot->accumulator_correction_flag) {
      prep_mot->accumulator_correction_flag = false;
      run_mot->substep_accumulator *= prep_mot->accumulator_correction;
    }

    // Detect direction change and if so:
    //  - Set the direction bit in hardware.
    //  - Compensate for direction change by flipping substep accumulator
    //    value about its midpoint.
    if (prep_mot->direction != prep_mot->prev_direction) {
      prep_mot->prev_direction = prep_mot->direction;
      run_mot->substep_accumulator =
        -st_run.dda_ticks_X_substeps - run_mot->substep_accumulator;

      if (prep_mot->direction == DIRECTION_CW)
        hw.st_port[motor]->OUTCLR = DIRECTION_BIT_bm;
      else hw.st_port[motor]->OUTSET = DIRECTION_BIT_bm;
    }

    SET_ENCODER_STEP_SIGN(motor, prep_mot->step_sign);
  }

  // Enable the stepper and start motor power management
  if ((run_mot->substep_increment && cfg_mot->power_mode != MOTOR_DISABLED) ||
      cfg_mot->power_mode == MOTOR_POWERED_IN_CYCLE) {
    hw.st_port[motor]->OUTCLR = MOTOR_ENABLE_BIT_bm; // energize motor
    run_mot->power_state = MOTOR_POWER_TIMEOUT_START; // start power management
  }

  // Accumulate counted steps to the step position and zero out counted steps
  // for the segment currently being loaded
  ACCUMULATE_ENCODER(motor);
}


/* Dequeue move and load into stepper struct
 *
 * This routine can only be called be called from an ISR at the same or
 * higher level as the DDA or dwell ISR. A software interrupt has been
 * provided to allow a non-ISR to request a load (see st_request_load_move())
 *
 * In aline() code:
 *   - All axes must set steps and compensate for out-of-range pulse phasing.
 *   - If axis has 0 steps the direction setting can be omitted
 *   - If axis has 0 steps the motor must not be enabled to support power
 *     mode = 1
 */
static void _load_move() {
  // Be aware that dda_ticks_downcount must equal zero for the loader to run.
  // So the initial load must also have this set to zero as part of
  // initialization
  if (st_runtime_isbusy() || st_pre.buffer_state != PREP_BUFFER_OWNED_BY_LOADER)
    return; // exit if the runtime is busy or there are no more moves

  st_run.move_type = st_pre.move_type;

  switch (st_pre.move_type) {
  case MOVE_TYPE_ALINE: // Setup the new segment
    st_run.dda_ticks_downcount = st_pre.dda_ticks;
    st_run.dda_ticks_X_substeps = st_pre.dda_ticks_X_substeps;

    _load_motor_move(MOTOR_1);
    _load_motor_move(MOTOR_2);
    _load_motor_move(MOTOR_3);
    _load_motor_move(MOTOR_4);

    // do this last
    TIMER_DDA.PER = DDA_PERIOD;
    TIMER_DDA.CTRLA = STEP_TIMER_ENABLE; // enable the DDA timer
    break;

  case MOVE_TYPE_DWELL: // handle dwells
    st_run.dda_ticks_downcount = st_pre.dda_ticks;
    TIMER_DDA.PER = DDA_PERIOD;          // load dwell timer period
    TIMER_DDA.CTRLA = STEP_TIMER_ENABLE; // enable the dwell timer
    break;

  case MOVE_TYPE_COMMAND: // handle synchronous commands
    mp_runtime_command(st_pre.bf);
    // Fall through

  default:
    TIMER_DDA.CTRLA = STEP_TIMER_DISABLE;
    break;
  }

  // all other cases skip to here (e.g. Null moves after Mcodes skip to here)
  st_pre.move_type = MOVE_TYPE_0;

  // we are done with the prep buffer - flip the flag back
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;
  st_request_exec_move(); // exec and prep next move
}


/* Prepare the next move for the loader
 *
 * This function does the math on the next pulse segment and gets it ready for
 * the loader. It deals with all the DDA optimizations and timer setups so that
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
 *   - following_error[] is a vector of measured errors to the step count. Used
 *     for correction.
 *
 *   - segment_time - how many minutes the segment should run. If timing is not
 *     100% accurate this will affect the move velocity, but not the distance
 *     traveled.
 *
 * NOTE: Many of the expressions are sensitive to casting and execution order to
 * avoid long-term accuracy errors due to floating point round off. One earlier
 * failed attempt was:
 *
 *   dda_ticks_X_substeps =
 *     (int32_t)((microseconds / 1000000) * f_dda * dda_substeps);
 */
stat_t st_prep_line(float travel_steps[], float following_error[],
                    float segment_time) {
  // Trap conditions that would prevent queueing the line
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC)
    return cm_hard_alarm(STAT_INTERNAL_ERROR);
  else if (isinf(segment_time)) // never supposed to happen
    return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_INFINITE);
  else if (isnan(segment_time)) // never supposed to happen
    return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_NAN);
  else if (segment_time < EPSILON) return STAT_MINIMUM_TIME_MOVE;

  // setup segment parameters
  // - dda_ticks is the integer number of DDA clock ticks needed to play out the
  //   segment
  // - ticks_X_substeps is the maximum depth of the DDA accumulator (as a
  //   negative number)
  // convert minutes to seconds
  st_pre.dda_ticks = (int32_t)(segment_time * 60 * STEP_CLOCK_FREQ);
  st_pre.dda_ticks_X_substeps = st_pre.dda_ticks * DDA_SUBSTEPS;

  // setup motor parameters
  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    // Skip this motor if there are no new steps. Leave all other values intact.
    if (fp_ZERO(travel_steps[motor])) {
      st_pre.mot[motor].substep_increment = 0;
      continue;
    }

    // Setup the direction, compensating for polarity.
    // Set the step_sign which is used by the stepper ISR to accumulate step
    // position
    if (0 <= travel_steps[motor]) { // positive direction
      st_pre.mot[motor].direction = DIRECTION_CW ^ st_cfg.mot[motor].polarity;
      st_pre.mot[motor].step_sign = 1;

    } else {
      st_pre.mot[motor].direction = DIRECTION_CCW ^ st_cfg.mot[motor].polarity;
      st_pre.mot[motor].step_sign = -1;
    }

    // Detect segment time changes and setup the accumulator correction factor
    // and flag. Putting this here computes the correct factor even if the motor
    // was dormant for some number of previous moves. Correction is computed
    // based on the last segment time actually used.
    if (0.0000001 < fabs(segment_time - st_pre.mot[motor].prev_segment_time)) {
      // special case to skip first move
      if (fp_NOT_ZERO(st_pre.mot[motor].prev_segment_time)) {
        st_pre.mot[motor].accumulator_correction_flag = true;
        st_pre.mot[motor].accumulator_correction =
          segment_time / st_pre.mot[motor].prev_segment_time;
      }

      st_pre.mot[motor].prev_segment_time = segment_time;
    }

#ifdef __STEP_CORRECTION
    float correction_steps;

    // 'Nudge' correction strategy. Inject a single, scaled correction value
    // then hold off
    if (--st_pre.mot[motor].correction_holdoff < 0 &&
        fabs(following_error[motor]) > STEP_CORRECTION_THRESHOLD) {

      st_pre.mot[motor].correction_holdoff = STEP_CORRECTION_HOLDOFF;
      correction_steps = following_error[motor] * STEP_CORRECTION_FACTOR;

      if (0 < correction_steps)
        correction_steps = min3(correction_steps, fabs(travel_steps[motor]),
                                STEP_CORRECTION_MAX);
      else correction_steps = max3(correction_steps, -fabs(travel_steps[motor]),
                                   -STEP_CORRECTION_MAX);

      st_pre.mot[motor].corrected_steps += correction_steps;
      travel_steps[motor] -= correction_steps;
    }
#endif

    // Compute substep increment. The accumulator must be *exactly* the
    // incoming fractional steps times the substep multiplier or positional
    // drift will occur. Rounding is performed to eliminate a negative bias
    // in the uint32 conversion that results in long-term negative drift.
    // (fabs/round order doesn't matter)
    st_pre.mot[motor].substep_increment =
      round(fabs(travel_steps[motor] * DDA_SUBSTEPS));
  }

  st_pre.move_type = MOVE_TYPE_ALINE;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // signal prep buffer ready

  return STAT_OK;
}


/// Keeps the loader happy. Otherwise performs no action
void st_prep_null() {
  st_pre.move_type = MOVE_TYPE_0;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC; // signal prep buffer empty
}


/// Stage command to execution
void st_prep_command(void *bf) {
  st_pre.move_type = MOVE_TYPE_COMMAND;
  st_pre.bf = (mpBuf_t *)bf;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // signal prep buffer ready
}


/// Add a dwell to the move buffer
void st_prep_dwell(float microseconds) {
  st_pre.move_type = MOVE_TYPE_DWELL;
  st_pre.dda_ticks = microseconds / 1000000 * STEP_CLOCK_FREQ;
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
