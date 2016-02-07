/*
 * stepper.c - stepper motor controls
 * This file is part of the TinyG project
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
 * Copyright (c) 2013 - 2015 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software library without
 * restriction. Specifically, if other files instantiate templates or use macros or
 * inline functions from this file, or you compile this file and link it with  other
 * files to produce an executable, this file does not by itself cause the resulting
 * executable to be covered by the GNU General Public License. This exception does not
 * however invalidate any other reasons why the executable file might be covered by the
 * GNU General Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* This module provides the low-level stepper drivers and some related functions.
 * See stepper.h for a detailed explanation of this module.
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

stConfig_t st_cfg;
stPrepSingleton_t st_pre;
static stRunSingleton_t st_run;

static void _load_move();
static void _request_load_move();


// handy macro
#define _f_to_period(f) (uint16_t)((float)F_CPU / (float)f)

#define MOTOR_1        0
#define MOTOR_2        1
#define MOTOR_3        2
#define MOTOR_4        3
#define MOTOR_5        4
#define MOTOR_6        5


/*
 * Initialize stepper motor subsystem
 *
 *    Notes:
 *      - This init requires sys_init() to be run beforehand
 *      - microsteps are setup during config_init()
 *      - motor polarity is setup during config_init()
 *      - high level interrupts must be enabled in main() once all inits are complete
 */
void stepper_init() {
  memset(&st_run, 0, sizeof(st_run));          // clear all values, pointers and status

  // Configure virtual ports
  PORTCFG.VPCTRLA = PORTCFG_VP0MAP_PORT_MOTOR_1_gc | PORTCFG_VP1MAP_PORT_MOTOR_2_gc;
  PORTCFG.VPCTRLB = PORTCFG_VP2MAP_PORT_MOTOR_3_gc | PORTCFG_VP3MAP_PORT_MOTOR_4_gc;

  // setup ports and data structures
  for (uint8_t i = 0; i < MOTORS; i++) {
    hw.st_port[i]->DIR = MOTOR_PORT_DIR_gm;      // sets outputs for motors & GPIO1, and GPIO2 inputs
    hw.st_port[i]->OUTSET = MOTOR_ENABLE_BIT_bm; // disable motor
  }

  // setup DDA timer
  TIMER_DDA.CTRLA = STEP_TIMER_DISABLE;        // turn timer off
  TIMER_DDA.CTRLB = STEP_TIMER_WGMODE;         // waveform mode
  TIMER_DDA.INTCTRLA = TIMER_DDA_INTLVL;       // interrupt mode

  // setup DWELL timer
  TIMER_DWELL.CTRLA = STEP_TIMER_DISABLE;      // turn timer off
  TIMER_DWELL.CTRLB = STEP_TIMER_WGMODE;       // waveform mode
  TIMER_DWELL.INTCTRLA = TIMER_DWELL_INTLVL;   // interrupt mode

  // setup software interrupt load timer
  TIMER_LOAD.CTRLA = LOAD_TIMER_DISABLE;       // turn timer off
  TIMER_LOAD.CTRLB = LOAD_TIMER_WGMODE;        // waveform mode
  TIMER_LOAD.INTCTRLA = TIMER_LOAD_INTLVL;     // interrupt mode
  TIMER_LOAD.PER = LOAD_TIMER_PERIOD;          // set period

  // setup software interrupt exec timer
  TIMER_EXEC.CTRLA = EXEC_TIMER_DISABLE;       // turn timer off
  TIMER_EXEC.CTRLB = EXEC_TIMER_WGMODE;        // waveform mode
  TIMER_EXEC.INTCTRLA = TIMER_EXEC_INTLVL;     // interrupt mode
  TIMER_EXEC.PER = EXEC_TIMER_PERIOD;          // set period

  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;

  // defaults
  st_cfg.motor_power_timeout = MOTOR_IDLE_TIMEOUT;

  st_cfg.mot[MOTOR_1].motor_map =  M1_MOTOR_MAP;
  st_cfg.mot[MOTOR_1].step_angle = M1_STEP_ANGLE;
  st_cfg.mot[MOTOR_1].travel_rev = M1_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_1].microsteps = M1_MICROSTEPS;
  st_cfg.mot[MOTOR_1].polarity =   M1_POLARITY;
  st_cfg.mot[MOTOR_1].power_mode = M1_POWER_MODE;
#if (MOTORS >= 2)
  st_cfg.mot[MOTOR_2].motor_map =  M2_MOTOR_MAP;
  st_cfg.mot[MOTOR_2].step_angle = M2_STEP_ANGLE;
  st_cfg.mot[MOTOR_2].travel_rev = M2_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_2].microsteps = M2_MICROSTEPS;
  st_cfg.mot[MOTOR_2].polarity =   M2_POLARITY;
  st_cfg.mot[MOTOR_2].power_mode = M2_POWER_MODE;
#endif
#if (MOTORS >= 3)
  st_cfg.mot[MOTOR_3].motor_map =  M3_MOTOR_MAP;
  st_cfg.mot[MOTOR_3].step_angle = M3_STEP_ANGLE;
  st_cfg.mot[MOTOR_3].travel_rev = M3_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_3].microsteps = M3_MICROSTEPS;
  st_cfg.mot[MOTOR_3].polarity =   M3_POLARITY;
  st_cfg.mot[MOTOR_3].power_mode = M3_POWER_MODE;
#endif
#if (MOTORS >= 4)
  st_cfg.mot[MOTOR_4].motor_map =  M4_MOTOR_MAP;
  st_cfg.mot[MOTOR_4].step_angle = M4_STEP_ANGLE;
  st_cfg.mot[MOTOR_4].travel_rev = M4_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_4].microsteps = M4_MICROSTEPS;
  st_cfg.mot[MOTOR_4].polarity =   M4_POLARITY;
  st_cfg.mot[MOTOR_4].power_mode = M4_POWER_MODE;
#endif
#if (MOTORS >= 5)
  st_cfg.mot[MOTOR_5].motor_map =  M5_MOTOR_MAP;
  st_cfg.mot[MOTOR_5].step_angle = M5_STEP_ANGLE;
  st_cfg.mot[MOTOR_5].travel_rev = M5_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_5].microsteps = M5_MICROSTEPS;
  st_cfg.mot[MOTOR_5].polarity =   M5_POLARITY;
  st_cfg.mot[MOTOR_5].power_mode = M5_POWER_MODE;
#endif
#if (MOTORS >= 6)
  st_cfg.mot[MOTOR_6].motor_map =  M6_MOTOR_MAP;
  st_cfg.mot[MOTOR_6].step_angle = M6_STEP_ANGLE;
  st_cfg.mot[MOTOR_6].travel_rev = M6_TRAVEL_PER_REV;
  st_cfg.mot[MOTOR_6].microsteps = M6_MICROSTEPS;
  st_cfg.mot[MOTOR_6].polarity =   M6_POLARITY;
  st_cfg.mot[MOTOR_6].power_mode = M6_POWER_MODE;
#endif

  // Init steps per unit
  for (int m = 0; m < MOTORS; m++)
    st_cfg.mot[m].steps_per_unit =
      (360 * st_cfg.mot[m].microsteps) / (st_cfg.mot[m].travel_rev * st_cfg.mot[m].step_angle);

  st_reset();                                  // reset steppers to known state
}


/*
 * return TRUE if runtime is busy:
 *
 *    Busy conditions:
 *    - motors are running
 *    - dwell is running
 */
uint8_t st_runtime_isbusy() {
  return st_run.dda_ticks_downcount != 0;
}


/// Reset stepper internals
void st_reset() {
  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    st_pre.mot[motor].prev_direction = STEP_INITIAL_DIRECTION;
    st_run.mot[motor].substep_accumulator = 0;    // will become max negative during per-motor setup;
    st_pre.mot[motor].corrected_steps = 0;        // diagnostic only - no action effect
  }

  mp_set_steps_to_runtime_position();
}


/*
 * Motor power management functions
 *
 * _deenergize_motor()         - remove power from a motor
 * _energize_motor()         - apply power to a motor
 *
 * st_energize_motors()         - apply power to all motors
 * st_deenergize_motors()     - remove power from all motors
 * st_motor_power_callback() - callback to manage motor power sequencing
 */
static uint8_t _motor_is_enabled(uint8_t motor) {
  uint8_t port;
  switch(motor) {
  case (MOTOR_1): port = PORT_MOTOR_1_VPORT.OUT; break;
  case (MOTOR_2): port = PORT_MOTOR_2_VPORT.OUT; break;
  case (MOTOR_3): port = PORT_MOTOR_3_VPORT.OUT; break;
  case (MOTOR_4): port = PORT_MOTOR_4_VPORT.OUT; break;
  default: port = 0xff; // defaults to disabled for bad motor input value
  }

  // returns 1 if motor is enabled (motor is actually active low)
  return port & MOTOR_ENABLE_BIT_bm ? 0 : 1;
}


static void _deenergize_motor(const uint8_t motor) {
  switch (motor) {
  case (MOTOR_1): PORT_MOTOR_1_VPORT.OUT |= MOTOR_ENABLE_BIT_bm; break;
  case (MOTOR_2): PORT_MOTOR_2_VPORT.OUT |= MOTOR_ENABLE_BIT_bm; break;
  case (MOTOR_3): PORT_MOTOR_3_VPORT.OUT |= MOTOR_ENABLE_BIT_bm; break;
  case (MOTOR_4): PORT_MOTOR_4_VPORT.OUT |= MOTOR_ENABLE_BIT_bm; break;
  }

  st_run.mot[motor].power_state = MOTOR_OFF;
}


static void _energize_motor(const uint8_t motor) {
  if (st_cfg.mot[motor].power_mode == MOTOR_DISABLED) {
    _deenergize_motor(motor);
    return;
  }

  switch(motor) {
  case (MOTOR_1): PORT_MOTOR_1_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm; break;
  case (MOTOR_2): PORT_MOTOR_2_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm; break;
  case (MOTOR_3): PORT_MOTOR_3_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm; break;
  case (MOTOR_4): PORT_MOTOR_4_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm; break;
  }

  st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;
}


void st_energize_motors() {
  for (uint8_t motor = MOTOR_1; motor < MOTORS; motor++) {
    _energize_motor(motor);
    st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;
  }
}


void st_deenergize_motors() {
  for (uint8_t motor = MOTOR_1; motor < MOTORS; motor++)
    _deenergize_motor(motor);
}


/*
 * Callback to manage motor power sequencing
 * Handles motor power-down timing, low-power idle, and adaptive motor power
 */
stat_t st_motor_power_callback() { // called by controller
  // manage power for each motor individually
  for (uint8_t m = 0; m < MOTORS; m++) {

    // de-energize motor if it's set to MOTOR_DISABLED
    if (st_cfg.mot[m].power_mode == MOTOR_DISABLED) {
      _deenergize_motor(m);
      continue;
    }

    // energize motor if it's set to MOTOR_ALWAYS_POWERED
    if (st_cfg.mot[m].power_mode == MOTOR_ALWAYS_POWERED) {
      if (! _motor_is_enabled(m)) _energize_motor(m);
      continue;
    }

    // start a countdown if MOTOR_POWERED_IN_CYCLE or MOTOR_POWERED_ONLY_WHEN_MOVING
    if (st_run.mot[m].power_state == MOTOR_POWER_TIMEOUT_START) {
      st_run.mot[m].power_state = MOTOR_POWER_TIMEOUT_COUNTDOWN;
      st_run.mot[m].power_systick = rtc_get_time() +
        (st_cfg.motor_power_timeout * 1000);
    }

    // do not process countdown if in a feedhold
    if (cm_get_combined_state() == COMBINED_HOLD) continue;

    // do not process countdown if in a feedhold
    if (cm_get_combined_state() == COMBINED_HOLD) continue;

    // run the countdown if you are in a countdown
    if (st_run.mot[m].power_state == MOTOR_POWER_TIMEOUT_COUNTDOWN) {
      if (rtc_get_time() > st_run.mot[m].power_systick ) {
        st_run.mot[m].power_state = MOTOR_IDLE;
        _deenergize_motor(m);
        report_request(); // request a status report when motors shut down
      }
    }
  }

  return STAT_OK;
}


/***
    Stepper Interrupt Service Routine
    DDA timer interrupt routine - service ticks from DDA timer

    Uses direct struct addresses and literal values for hardware devices - it's faster than
    using indexed timer and port accesses. I checked. Even when -0s or -03 is used.
*/
ISR(TIMER_DDA_ISR_vect) {
  if ((st_run.mot[MOTOR_1].substep_accumulator += st_run.mot[MOTOR_1].substep_increment) > 0) {
    PORT_MOTOR_1_VPORT.OUT ^= STEP_BIT_bm; // toggle step line
    st_run.mot[MOTOR_1].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_1);
  }

  if ((st_run.mot[MOTOR_2].substep_accumulator += st_run.mot[MOTOR_2].substep_increment) > 0) {
    PORT_MOTOR_2_VPORT.OUT ^= STEP_BIT_bm;
    st_run.mot[MOTOR_2].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_2);
  }

  if ((st_run.mot[MOTOR_3].substep_accumulator += st_run.mot[MOTOR_3].substep_increment) > 0) {
    PORT_MOTOR_3_VPORT.OUT ^= STEP_BIT_bm;
    st_run.mot[MOTOR_3].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_3);
  }

  if ((st_run.mot[MOTOR_4].substep_accumulator += st_run.mot[MOTOR_4].substep_increment) > 0) {
    PORT_MOTOR_4_VPORT.OUT ^= STEP_BIT_bm;
    st_run.mot[MOTOR_4].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_4);
  }

  if (--st_run.dda_ticks_downcount != 0) return;

  TIMER_DDA.CTRLA = STEP_TIMER_DISABLE;                // disable DDA timer
  _load_move();                                        // load the next move
}


/// DDA timer interrupt routine - service ticks from DDA timer
ISR(TIMER_DWELL_ISR_vect) {                            // DWELL timer interrupt
  if (--st_run.dda_ticks_downcount == 0) {
    TIMER_DWELL.CTRLA = STEP_TIMER_DISABLE;            // disable DWELL timer
    _load_move();
  }
}


/****************************************************************************************
 * Exec sequencing code      - computes and prepares next load segment
 * st_request_exec_move()    - SW interrupt to request to execute a move
 * exec_timer interrupt      - interrupt handler for calling exec function
 */
void st_request_exec_move() {
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {// bother interrupting
    TIMER_EXEC.PER = EXEC_TIMER_PERIOD;
    TIMER_EXEC.CTRLA = EXEC_TIMER_ENABLE;                // trigger a LO interrupt
  }
}


ISR(TIMER_EXEC_ISR_vect) {                               // exec move SW interrupt
  TIMER_EXEC.CTRLA = EXEC_TIMER_DISABLE;                 // disable SW interrupt timer

  // exec_move
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {
    if (mp_exec_move() != STAT_NOOP) {
      st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // flip it back
      _request_load_move();
    }
  }
}

/****************************************************************************************
 * Loader sequencing code
 * st_request_load_move() - fires a software interrupt (timer) to request to load a move
 * load_move interrupt    - interrupt handler for running the loader
 *
 *    _load_move() can only be called be called from an ISR at the same or higher level as
 *    the DDA or dwell ISR. A software interrupt has been provided to allow a non-ISR to
 *    request a load (see st_request_load_move())
 */
static void _request_load_move() {
  if (st_runtime_isbusy()) return; // don't request a load if the runtime is busy

  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_LOADER) {  // bother interrupting
    TIMER_LOAD.PER = LOAD_TIMER_PERIOD;
    TIMER_LOAD.CTRLA = LOAD_TIMER_ENABLE;                    // trigger a HI interrupt
  }
}


ISR(TIMER_LOAD_ISR_vect) {                                   // load steppers SW interrupt
  TIMER_LOAD.CTRLA = LOAD_TIMER_DISABLE;                     // disable SW interrupt timer
  _load_move();
}


/****************************************************************************************
 * _load_move() - Dequeue move and load into stepper struct
 *
 *    This routine can only be called be called from an ISR at the same or
 *    higher level as the DDA or dwell ISR. A software interrupt has been
 *    provided to allow a non-ISR to request a load (see st_request_load_move())
 *
 *    In aline() code:
 *     - All axes must set steps and compensate for out-of-range pulse phasing.
 *     - If axis has 0 steps the direction setting can be omitted
 *     - If axis has 0 steps the motor must not be enabled to support power mode = 1
 */
static void _load_move() {
  // Be aware that dda_ticks_downcount must equal zero for the loader to run.
  // So the initial load must also have this set to zero as part of initialization
  if (st_runtime_isbusy()) return; // exit if the runtime is busy
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_LOADER) return; // if there are no moves to load...

  // handle aline loads first (most common case)
  if (st_pre.move_type == MOVE_TYPE_ALINE) {
    //**** setup the new segment ****
    st_run.dda_ticks_downcount = st_pre.dda_ticks;
    st_run.dda_ticks_X_substeps = st_pre.dda_ticks_X_substeps;

    //**** MOTOR_1 LOAD ****

    // These sections are somewhat optimized for execution speed. The whole load operation
    // is supposed to take < 10 uSec (Xmega). Be careful if you mess with this.

    // the following if() statement sets the runtime substep increment value or zeroes it
    if ((st_run.mot[MOTOR_1].substep_increment = st_pre.mot[MOTOR_1].substep_increment) != 0) {
      // NB: If motor has 0 steps the following is all skipped. This ensures that state comparisons
      //       always operate on the last segment actually run by this motor, regardless of how many
      //       segments it may have been inactive in between.

      // Apply accumulator correction if the time base has changed since previous segment
      if (st_pre.mot[MOTOR_1].accumulator_correction_flag == true) {
        st_pre.mot[MOTOR_1].accumulator_correction_flag = false;
        st_run.mot[MOTOR_1].substep_accumulator *= st_pre.mot[MOTOR_1].accumulator_correction;
      }

      // Detect direction change and if so:
      //    - Set the direction bit in hardware.
      //    - Compensate for direction change by flipping substep accumulator value about its midpoint.
      if (st_pre.mot[MOTOR_1].direction != st_pre.mot[MOTOR_1].prev_direction) {
        st_pre.mot[MOTOR_1].prev_direction = st_pre.mot[MOTOR_1].direction;
        st_run.mot[MOTOR_1].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[MOTOR_1].substep_accumulator);

        if (st_pre.mot[MOTOR_1].direction == DIRECTION_CW)
          PORT_MOTOR_1_VPORT.OUT &= ~DIRECTION_BIT_bm;
        else PORT_MOTOR_1_VPORT.OUT |= DIRECTION_BIT_bm;
      }

      SET_ENCODER_STEP_SIGN(MOTOR_1, st_pre.mot[MOTOR_1].step_sign);

      // Enable the stepper and start motor power management
      if (st_cfg.mot[MOTOR_1].power_mode != MOTOR_DISABLED) {
        PORT_MOTOR_1_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;             // energize motor
        st_run.mot[MOTOR_1].power_state = MOTOR_POWER_TIMEOUT_START;// set power management state
      }

    } else if (st_cfg.mot[MOTOR_1].power_mode == MOTOR_POWERED_IN_CYCLE) {
      // Motor has 0 steps; might need to energize motor for power mode processing
      PORT_MOTOR_1_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;             // energize motor
      st_run.mot[MOTOR_1].power_state = MOTOR_POWER_TIMEOUT_START;
    }

    // accumulate counted steps to the step position and zero out counted steps for the segment currently being loaded
    ACCUMULATE_ENCODER(MOTOR_1);

#if (MOTORS >= 2)    //**** MOTOR_2 LOAD ****
    if ((st_run.mot[MOTOR_2].substep_increment = st_pre.mot[MOTOR_2].substep_increment) != 0) {
      if (st_pre.mot[MOTOR_2].accumulator_correction_flag == true) {
        st_pre.mot[MOTOR_2].accumulator_correction_flag = false;
        st_run.mot[MOTOR_2].substep_accumulator *= st_pre.mot[MOTOR_2].accumulator_correction;
      }

      if (st_pre.mot[MOTOR_2].direction != st_pre.mot[MOTOR_2].prev_direction) {
        st_pre.mot[MOTOR_2].prev_direction = st_pre.mot[MOTOR_2].direction;
        st_run.mot[MOTOR_2].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[MOTOR_2].substep_accumulator);
        if (st_pre.mot[MOTOR_2].direction == DIRECTION_CW)
          PORT_MOTOR_2_VPORT.OUT &= ~DIRECTION_BIT_bm;
        else PORT_MOTOR_2_VPORT.OUT |= DIRECTION_BIT_bm;
      }

      SET_ENCODER_STEP_SIGN(MOTOR_2, st_pre.mot[MOTOR_2].step_sign);

      if (st_cfg.mot[MOTOR_2].power_mode != MOTOR_DISABLED) {
        PORT_MOTOR_2_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
        st_run.mot[MOTOR_2].power_state = MOTOR_POWER_TIMEOUT_START;
      }
    } else if (st_cfg.mot[MOTOR_2].power_mode == MOTOR_POWERED_IN_CYCLE) {
      PORT_MOTOR_2_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_2].power_state = MOTOR_POWER_TIMEOUT_START;
    }

    ACCUMULATE_ENCODER(MOTOR_2);
#endif

#if (MOTORS >= 3)    //**** MOTOR_3 LOAD ****
    if ((st_run.mot[MOTOR_3].substep_increment = st_pre.mot[MOTOR_3].substep_increment) != 0) {
      if (st_pre.mot[MOTOR_3].accumulator_correction_flag == true) {
        st_pre.mot[MOTOR_3].accumulator_correction_flag = false;
        st_run.mot[MOTOR_3].substep_accumulator *= st_pre.mot[MOTOR_3].accumulator_correction;
      }

      if (st_pre.mot[MOTOR_3].direction != st_pre.mot[MOTOR_3].prev_direction) {
        st_pre.mot[MOTOR_3].prev_direction = st_pre.mot[MOTOR_3].direction;
        st_run.mot[MOTOR_3].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[MOTOR_3].substep_accumulator);
        if (st_pre.mot[MOTOR_3].direction == DIRECTION_CW)
          PORT_MOTOR_3_VPORT.OUT &= ~DIRECTION_BIT_bm;
        else PORT_MOTOR_3_VPORT.OUT |= DIRECTION_BIT_bm;
      }

      SET_ENCODER_STEP_SIGN(MOTOR_3, st_pre.mot[MOTOR_3].step_sign);

      if (st_cfg.mot[MOTOR_3].power_mode != MOTOR_DISABLED) {
        PORT_MOTOR_3_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
        st_run.mot[MOTOR_3].power_state = MOTOR_POWER_TIMEOUT_START;
      }

    } else if (st_cfg.mot[MOTOR_3].power_mode == MOTOR_POWERED_IN_CYCLE) {
      PORT_MOTOR_3_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_3].power_state = MOTOR_POWER_TIMEOUT_START;
    }

    ACCUMULATE_ENCODER(MOTOR_3);
#endif

#if (MOTORS >= 4)  //**** MOTOR_4 LOAD ****
    if ((st_run.mot[MOTOR_4].substep_increment = st_pre.mot[MOTOR_4].substep_increment) != 0) {
      if (st_pre.mot[MOTOR_4].accumulator_correction_flag == true) {
        st_pre.mot[MOTOR_4].accumulator_correction_flag = false;
        st_run.mot[MOTOR_4].substep_accumulator *= st_pre.mot[MOTOR_4].accumulator_correction;
      }

      if (st_pre.mot[MOTOR_4].direction != st_pre.mot[MOTOR_4].prev_direction) {
        st_pre.mot[MOTOR_4].prev_direction = st_pre.mot[MOTOR_4].direction;
        st_run.mot[MOTOR_4].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[MOTOR_4].substep_accumulator);

        if (st_pre.mot[MOTOR_4].direction == DIRECTION_CW)
          PORT_MOTOR_4_VPORT.OUT &= ~DIRECTION_BIT_bm;
        else PORT_MOTOR_4_VPORT.OUT |= DIRECTION_BIT_bm;
      }

      SET_ENCODER_STEP_SIGN(MOTOR_4, st_pre.mot[MOTOR_4].step_sign);

      if (st_cfg.mot[MOTOR_4].power_mode != MOTOR_DISABLED) {
        PORT_MOTOR_4_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
        st_run.mot[MOTOR_4].power_state = MOTOR_POWER_TIMEOUT_START;
      }

    } else if (st_cfg.mot[MOTOR_4].power_mode == MOTOR_POWERED_IN_CYCLE) {
      PORT_MOTOR_4_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_4].power_state = MOTOR_POWER_TIMEOUT_START;
    }

    ACCUMULATE_ENCODER(MOTOR_4);
#endif

#if (MOTORS >= 5)    //**** MOTOR_5 LOAD ****
    if ((st_run.mot[MOTOR_5].substep_increment = st_pre.mot[MOTOR_5].substep_increment) != 0) {
      if (st_pre.mot[MOTOR_5].accumulator_correction_flag == true) {
        st_pre.mot[MOTOR_5].accumulator_correction_flag = false;
        st_run.mot[MOTOR_5].substep_accumulator *= st_pre.mot[MOTOR_5].accumulator_correction;
      }

      if (st_pre.mot[MOTOR_5].direction != st_pre.mot[MOTOR_5].prev_direction) {
        st_pre.mot[MOTOR_5].prev_direction = st_pre.mot[MOTOR_5].direction;
        st_run.mot[MOTOR_5].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[MOTOR_5].substep_accumulator);

        if (st_pre.mot[MOTOR_5].direction == DIRECTION_CW)
          PORT_MOTOR_5_VPORT.OUT &= ~DIRECTION_BIT_bm;
        else PORT_MOTOR_5_VPORT.OUT |= DIRECTION_BIT_bm;
      }

      PORT_MOTOR_5_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_5].power_state = MOTOR_POWER_TIMEOUT_START;
      SET_ENCODER_STEP_SIGN(MOTOR_5, st_pre.mot[MOTOR_5].step_sign);

    } else if (st_cfg.mot[MOTOR_5].power_mode == MOTOR_POWERED_IN_CYCLE) {
      PORT_MOTOR_5_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_5].power_state = MOTOR_POWER_TIMEOUT_START;
    }

    ACCUMULATE_ENCODER(MOTOR_5);
#endif

#if (MOTORS >= 6)    //**** MOTOR_6 LOAD ****
    if ((st_run.mot[MOTOR_6].substep_increment = st_pre.mot[MOTOR_6].substep_increment) != 0) {
      if (st_pre.mot[MOTOR_6].accumulator_correction_flag == true) {
        st_pre.mot[MOTOR_6].accumulator_correction_flag = false;
        st_run.mot[MOTOR_6].substep_accumulator *= st_pre.mot[MOTOR_6].accumulator_correction;
      }

      if (st_pre.mot[MOTOR_6].direction != st_pre.mot[MOTOR_6].prev_direction) {
        st_pre.mot[MOTOR_6].prev_direction = st_pre.mot[MOTOR_6].direction;
        st_run.mot[MOTOR_6].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[MOTOR_6].substep_accumulator);
        if (st_pre.mot[MOTOR_6].direction == DIRECTION_CW)
          PORT_MOTOR_6_VPORT.OUT &= ~DIRECTION_BIT_bm;
        else PORT_MOTOR_6_VPORT.OUT |= DIRECTION_BIT_bm;
      }

      PORT_MOTOR_6_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_6].power_state = MOTOR_POWER_TIMEOUT_START;
      SET_ENCODER_STEP_SIGN(MOTOR_6, st_pre.mot[MOTOR_6].step_sign);

    } else if (st_cfg.mot[MOTOR_6].power_mode == MOTOR_POWERED_IN_CYCLE) {
      PORT_MOTOR_6_VPORT.OUT &= ~MOTOR_ENABLE_BIT_bm;
      st_run.mot[MOTOR_6].power_state = MOTOR_POWER_TIMEOUT_START;
    }

    ACCUMULATE_ENCODER(MOTOR_6);
#endif

    //**** do this last ****
    TIMER_DDA.PER = st_pre.dda_period;
    TIMER_DDA.CTRLA = STEP_TIMER_ENABLE;            // enable the DDA timer

  } else if (st_pre.move_type == MOVE_TYPE_DWELL) {
    // handle dwells
    st_run.dda_ticks_downcount = st_pre.dda_ticks;
    TIMER_DWELL.PER = st_pre.dda_period;            // load dwell timer period
    TIMER_DWELL.CTRLA = STEP_TIMER_ENABLE;          // enable the dwell timer

  } else if (st_pre.move_type == MOVE_TYPE_COMMAND)
    // handle synchronous commands
    mp_runtime_command(st_pre.bf);

  // all other cases drop to here (e.g. Null moves after Mcodes skip to here)
  st_pre.move_type = MOVE_TYPE_0;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;    // we are done with the prep buffer - flip the flag back
  st_request_exec_move();                             // exec and prep next move
}


/***********************************************************************************
 * st_prep_line() - Prepare the next move for the loader
 *
 *    This function does the math on the next pulse segment and gets it ready for
 *    the loader. It deals with all the DDA optimizations and timer setups so that
 *    loading can be performed as rapidly as possible. It works in joint space
 *    (motors) and it works in steps, not length units. All args are provided as
 *    floats and converted to their appropriate integer types for the loader.
 *
 * Args:
 *      - travel_steps[] are signed relative motion in steps for each motor. Steps are
 *        floats that typically have fractional values (fractional steps). The sign
 *        indicates direction. Motors that are not in the move should be 0 steps on input.
 *
 *      - following_error[] is a vector of measured errors to the step count. Used for correction.
 *
 *      - segment_time - how many minutes the segment should run. If timing is not
 *        100% accurate this will affect the move velocity, but not the distance traveled.
 *
 * NOTE:  Many of the expressions are sensitive to casting and execution order to avoid long-term
 *        accuracy errors due to floating point round off. One earlier failed attempt was:
 *        dda_ticks_X_substeps = (int32_t)((microseconds / 1000000) * f_dda * dda_substeps);
 */
stat_t st_prep_line(float travel_steps[], float following_error[], float segment_time) {
  // trap conditions that would prevent queueing the line
  if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC) return cm_hard_alarm(STAT_INTERNAL_ERROR);
  else if (isinf(segment_time)) return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_INFINITE);    // never supposed to happen
  else if (isnan(segment_time)) return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_NAN);         // never supposed to happen
  else if (segment_time < EPSILON) return STAT_MINIMUM_TIME_MOVE;

  // setup segment parameters
  // - dda_ticks is the integer number of DDA clock ticks needed to play out the segment
  // - ticks_X_substeps is the maximum depth of the DDA accumulator (as a negative number)
  st_pre.dda_period = _f_to_period(FREQUENCY_DDA);
  st_pre.dda_ticks = (int32_t)(segment_time * 60 * FREQUENCY_DDA);// NB: converts minutes to seconds
  st_pre.dda_ticks_X_substeps = st_pre.dda_ticks * DDA_SUBSTEPS;

  // setup motor parameters
  for (uint8_t motor = 0; motor < MOTORS; motor++) {
    // Skip this motor if there are no new steps. Leave all other values intact.
    if (fp_ZERO(travel_steps[motor])) {
      st_pre.mot[motor].substep_increment = 0;
      continue;
    }

    // Setup the direction, compensating for polarity.
    // Set the step_sign which is used by the stepper ISR to accumulate step position
    if (travel_steps[motor] >= 0) {                    // positive direction
      st_pre.mot[motor].direction = DIRECTION_CW ^ st_cfg.mot[motor].polarity;
      st_pre.mot[motor].step_sign = 1;

    } else {
      st_pre.mot[motor].direction = DIRECTION_CCW ^ st_cfg.mot[motor].polarity;
      st_pre.mot[motor].step_sign = -1;
    }

    // Detect segment time changes and setup the accumulator correction factor and flag.
    // Putting this here computes the correct factor even if the motor was dormant for some
    // number of previous moves. Correction is computed based on the last segment time actually used.
    if (fabs(segment_time - st_pre.mot[motor].prev_segment_time) > 0.0000001) {  // highly tuned FP != compare
      if (fp_NOT_ZERO(st_pre.mot[motor].prev_segment_time)) {                    // special case to skip first move
        st_pre.mot[motor].accumulator_correction_flag = true;
        st_pre.mot[motor].accumulator_correction = segment_time / st_pre.mot[motor].prev_segment_time;
      }

      st_pre.mot[motor].prev_segment_time = segment_time;
    }

#ifdef __STEP_CORRECTION
    float correction_steps;

    // 'Nudge' correction strategy. Inject a single, scaled correction value then hold off
    if ((--st_pre.mot[motor].correction_holdoff < 0) &&
        (fabs(following_error[motor]) > STEP_CORRECTION_THRESHOLD)) {

      st_pre.mot[motor].correction_holdoff = STEP_CORRECTION_HOLDOFF;
      correction_steps = following_error[motor] * STEP_CORRECTION_FACTOR;

      if (correction_steps > 0)
        correction_steps = min3(correction_steps, fabs(travel_steps[motor]), STEP_CORRECTION_MAX);
      else correction_steps = max3(correction_steps, -fabs(travel_steps[motor]), -STEP_CORRECTION_MAX);

      st_pre.mot[motor].corrected_steps += correction_steps;
      travel_steps[motor] -= correction_steps;
    }
#endif

    // Compute substeb increment. The accumulator must be *exactly* the incoming
    // fractional steps times the substep multiplier or positional drift will occur.
    // Rounding is performed to eliminate a negative bias in the uint32 conversion
    // that results in long-term negative drift. (fabs/round order doesn't matter)
    st_pre.mot[motor].substep_increment = round(fabs(travel_steps[motor] * DDA_SUBSTEPS));
  }

  st_pre.move_type = MOVE_TYPE_ALINE;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;    // signal that prep buffer is ready

  return STAT_OK;
}


/// Keeps the loader happy. Otherwise performs no action
void st_prep_null() {
  st_pre.move_type = MOVE_TYPE_0;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;    // signal that prep buffer is empty
}


/// Stage command to execution
void st_prep_command(void *bf) {
  st_pre.move_type = MOVE_TYPE_COMMAND;
  st_pre.bf = (mpBuf_t *)bf;
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;    // signal that prep buffer is ready
}


/// Add a dwell to the move buffer
void st_prep_dwell(float microseconds) {
  st_pre.move_type = MOVE_TYPE_DWELL;
  st_pre.dda_period = _f_to_period(FREQUENCY_DWELL);
  st_pre.dda_ticks = (uint32_t)(microseconds / 1000000 * FREQUENCY_DWELL);
  st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;    // signal that prep buffer is ready
}


float get_ang(int index) {return st_cfg.mot[index].step_angle;}
float get_trvl(int index) {return st_cfg.mot[index].travel_rev;}
uint16_t get_mstep(int index) {return st_cfg.mot[index].microsteps;}
bool get_pol(int index) {return st_cfg.mot[index].polarity;}
uint16_t get_mvel(int index) {return 0;}
uint16_t get_mjerk(int index) {return 0;}


void update_steps_per_unit(int index) {
  st_cfg.mot[index].steps_per_unit =
    (360 * st_cfg.mot[index].microsteps) /
    (st_cfg.mot[index].travel_rev * st_cfg.mot[index].step_angle);
}


void set_ang(int index, float value) {
  st_cfg.mot[index].step_angle = value;
  update_steps_per_unit(index);
}


void set_trvl(int index, float value) {
  st_cfg.mot[index].travel_rev = value;
  update_steps_per_unit(index);
}


void set_mstep(int index, uint16_t value) {
  switch (value) {
  case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
    break;
  default: return;
  }

  st_cfg.mot[index].microsteps = value;
  update_steps_per_unit(index);
}


void set_pol(int index, bool value) {
  st_cfg.mot[index].polarity = value;
}


void set_mvel(int index, uint16_t value) {
}


void set_mjerk(int index, uint16_t value) {
}
