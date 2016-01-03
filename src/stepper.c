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

#include "tinyg.h"
#include "config.h"
#include "stepper.h"
#include "encoder.h"
#include "planner.h"
#include "report.h"
#include "hardware.h"
#include "text_parser.h"
#include "util.h"


stConfig_t st_cfg;
stPrepSingleton_t st_pre;
static stRunSingleton_t st_run;

static void _load_move();
static void _request_load_move();


// handy macro
#define _f_to_period(f) (uint16_t)((float)F_CPU / (float)f)


/*
 * stepper_init() - initialize stepper motor subsystem
 *
 *    Notes:
 *      - This init requires sys_init() to be run beforehand
 *      - microsteps are setup during config_init()
 *      - motor polarity is setup during config_init()
 *      - high level interrupts must be enabled in main() once all inits are complete
 */

/*    NOTE: This is the bare code that the Motate timer calls replace.
 *    NB: requires: #include <component_tc.h>
 *
 *    REG_TC1_WPMR = 0x54494D00;            // enable write to registers
 *    TC_Configure(TC_BLOCK_DDA, TC_CHANNEL_DDA, TC_CMR_DDA);
 *    REG_RC_DDA = TC_RC_DDA;                // set frequency
 *    REG_IER_DDA = TC_IER_DDA;            // enable interrupts
 *    NVIC_EnableIRQ(TC_IRQn_DDA);
 *    pmc_enable_periph_clk(TC_ID_DDA);
 *    TC_Start(TC_BLOCK_DDA, TC_CHANNEL_DDA);
 */
void stepper_init() {
  memset(&st_run, 0, sizeof(st_run));            // clear all values, pointers and status
  stepper_init_assertions();

  // Configure virtual ports
  PORTCFG.VPCTRLA = PORTCFG_VP0MAP_PORT_MOTOR_1_gc | PORTCFG_VP1MAP_PORT_MOTOR_2_gc;
  PORTCFG.VPCTRLB = PORTCFG_VP2MAP_PORT_MOTOR_3_gc | PORTCFG_VP3MAP_PORT_MOTOR_4_gc;

  // setup ports and data structures
  for (uint8_t i = 0; i < MOTORS; i++) {
    hw.st_port[i]->DIR = MOTOR_PORT_DIR_gm;    // sets outputs for motors & GPIO1, and GPIO2 inputs
    hw.st_port[i]->OUT = MOTOR_ENABLE_BIT_bm;  // zero port bits AND disable motor
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
  st_reset();                                    // reset steppers to known state
}


/*
 * stepper_init_assertions() - test assertions, return error code if violation exists
 * stepper_test_assertions() - test assertions, return error code if violation exists
 */
void stepper_init_assertions() {
  st_run.magic_end = MAGICNUM;
  st_run.magic_start = MAGICNUM;
  st_pre.magic_end = MAGICNUM;
  st_pre.magic_start = MAGICNUM;
}


stat_t stepper_test_assertions() {
  if (st_run.magic_end   != MAGICNUM) return STAT_STEPPER_ASSERTION_FAILURE;
  if (st_run.magic_start != MAGICNUM) return STAT_STEPPER_ASSERTION_FAILURE;
  if (st_pre.magic_end   != MAGICNUM) return STAT_STEPPER_ASSERTION_FAILURE;
  if (st_pre.magic_start != MAGICNUM) return STAT_STEPPER_ASSERTION_FAILURE;

  return STAT_OK;
}


/*
 * st_runtime_isbusy() - return TRUE if runtime is busy:
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


/// Clear counters
stat_t st_clc(nvObj_t *nv) {
  // clear diagnostic counters, reset stepper prep
  st_reset();
  return STAT_OK;
}

/*
 * Motor power management functions
 *
 * _deenergize_motor()         - remove power from a motor
 * _energize_motor()         - apply power to a motor
 * _set_motor_power_level()     - set the actual Vref to a specified power level
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
  default: port = 0xff;    // defaults to disabled for bad motor input value
  }

  return port & MOTOR_ENABLE_BIT_bm ? 0 : 1;    // returns 1 if motor is enabled (motor is actually active low)
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
 * st_motor_power_callback() - callback to manage motor power sequencing
 *
 *    Handles motor power-down timing, low-power idle, and adaptive motor power
 */
stat_t st_motor_power_callback() {    // called by controller
  // manage power for each motor individually
  for (uint8_t m = MOTOR_1; m < MOTORS; m++) {

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
      st_run.mot[m].power_systick = SysTickTimer_getValue() +
        (st_cfg.motor_power_timeout * 1000);
    }

    // do not process countdown if in a feedhold
    if (cm_get_combined_state() == COMBINED_HOLD) {
      continue;
    }

    // do not process countdown if in a feedhold
    if (cm_get_combined_state() == COMBINED_HOLD) {
      continue;
    }

    // run the countdown if you are in a countdown
    if (st_run.mot[m].power_state == MOTOR_POWER_TIMEOUT_COUNTDOWN) {
      if (SysTickTimer_getValue() > st_run.mot[m].power_systick ) {
        st_run.mot[m].power_state = MOTOR_IDLE;
        _deenergize_motor(m);
        sr_request_status_report(SR_TIMED_REQUEST);        // request a status report when motors shut down
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
    PORT_MOTOR_1_VPORT.OUT |= STEP_BIT_bm;        // turn step bit on
    st_run.mot[MOTOR_1].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_1);
  }

  if ((st_run.mot[MOTOR_2].substep_accumulator += st_run.mot[MOTOR_2].substep_increment) > 0) {
    PORT_MOTOR_2_VPORT.OUT |= STEP_BIT_bm;
    st_run.mot[MOTOR_2].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_2);
  }

  if ((st_run.mot[MOTOR_3].substep_accumulator += st_run.mot[MOTOR_3].substep_increment) > 0) {
    PORT_MOTOR_3_VPORT.OUT |= STEP_BIT_bm;
    st_run.mot[MOTOR_3].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_3);
  }

  if ((st_run.mot[MOTOR_4].substep_accumulator += st_run.mot[MOTOR_4].substep_increment) > 0) {
    PORT_MOTOR_4_VPORT.OUT |= STEP_BIT_bm;
    st_run.mot[MOTOR_4].substep_accumulator -= st_run.dda_ticks_X_substeps;
    INCREMENT_ENCODER(MOTOR_4);
  }

  // pulse stretching for using external drivers.- turn step bits off
  PORT_MOTOR_1_VPORT.OUT &= ~STEP_BIT_bm;                // ~ 5 uSec pulse width
  PORT_MOTOR_2_VPORT.OUT &= ~STEP_BIT_bm;                // ~ 4 uSec
  PORT_MOTOR_3_VPORT.OUT &= ~STEP_BIT_bm;                // ~ 3 uSec
  PORT_MOTOR_4_VPORT.OUT &= ~STEP_BIT_bm;                // ~ 2 uSec

  if (--st_run.dda_ticks_downcount != 0) return;

  TIMER_DDA.CTRLA = STEP_TIMER_DISABLE;                // disable DDA timer
  _load_move();                                        // load the next move
}


/// DDA timer interrupt routine - service ticks from DDA timer
ISR(TIMER_DWELL_ISR_vect) {                                // DWELL timer interrupt
  if (--st_run.dda_ticks_downcount == 0) {
    TIMER_DWELL.CTRLA = STEP_TIMER_DISABLE;            // disable DWELL timer
    _load_move();
  }
}


/****************************************************************************************
 * Exec sequencing code        - computes and prepares next load segment
 * st_request_exec_move()    - SW interrupt to request to execute a move
 * exec_timer interrupt        - interrupt handler for calling exec function
 */
void st_request_exec_move() {
  if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {// bother interrupting
    TIMER_EXEC.PER = EXEC_TIMER_PERIOD;
    TIMER_EXEC.CTRLA = EXEC_TIMER_ENABLE;                // trigger a LO interrupt
  }
}


ISR(TIMER_EXEC_ISR_vect) {                                // exec move SW interrupt
  TIMER_EXEC.CTRLA = EXEC_TIMER_DISABLE;                // disable SW interrupt timer

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
 * load_move interrupt      - interrupt handler for running the loader
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


ISR(TIMER_LOAD_ISR_vect) {                                      // load steppers SW interrupt
  TIMER_LOAD.CTRLA = LOAD_TIMER_DISABLE;                        // disable SW interrupt timer
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
  float correction_steps;
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


/// Helper to return motor number as an index
static int8_t _get_motor(const nvObj_t *nv) {
  return nv->group[0] ? nv->group[0] : nv->token[0] - 0x31;
}


/// This function will need to be rethought if microstep morphing is implemented
static void _set_motor_steps_per_unit(nvObj_t *nv) {
  uint8_t m = _get_motor(nv);
  st_cfg.mot[m].steps_per_unit = (360 * st_cfg.mot[m].microsteps) / (st_cfg.mot[m].travel_rev * st_cfg.mot[m].step_angle);
  st_reset();
}


/// Set motor step angle
stat_t st_set_sa(nvObj_t *nv) {
  set_flt(nv);
  _set_motor_steps_per_unit(nv);

  return STAT_OK;
}


/// Set motor travel per revolution
stat_t st_set_tr(nvObj_t *nv) {
  set_flu(nv);
  _set_motor_steps_per_unit(nv);

  return STAT_OK;
}


// Set motor microsteps
stat_t st_set_mi(nvObj_t *nv) {
  set_int(nv);
  _set_motor_steps_per_unit(nv);

  return STAT_OK;
}


// Set motor power mode
stat_t st_set_pm(nvObj_t *nv) {
  // NOTE: The motor power callback makes these settings take effect immediately
  if ((uint8_t)nv->value >= MOTOR_POWER_MODE_MAX_VALUE)
    return STAT_INPUT_VALUE_RANGE_ERROR;

  set_ui8(nv);

  return STAT_OK;
}


/*
 * st_set_pl() - set motor power level
 *
 *    Input value may vary from 0.000 to 1.000 The setting is scaled to allowable PWM range.
 *    This function sets both the scaled and dynamic power levels, and applies the
 *    scaled value to the vref.
 */
stat_t st_set_pl(nvObj_t *nv) {
  return STAT_OK;
}


/// get motor enable power state
stat_t st_get_pwr(nvObj_t *nv) {
  nv->value = _motor_is_enabled(_get_motor(nv));
  nv->valuetype = TYPE_INTEGER;

  return STAT_OK;
}


/* GLOBAL FUNCTIONS (SYSTEM LEVEL)
 * Calling me or md with 0 will enable or disable all motors
 * Setting a value of 0 will enable or disable all motors
 * Setting a value from 1 to MOTORS will enable or disable that motor only
 */

/// Set motor timeout in seconds
stat_t st_set_mt(nvObj_t *nv) {
  st_cfg.motor_power_timeout = min(MOTOR_TIMEOUT_SECONDS_MAX, max(nv->value, MOTOR_TIMEOUT_SECONDS_MIN));
  return STAT_OK;
}


/// Disable motor power
stat_t st_set_md(nvObj_t *nv) {
  // Make sure this function is not part of initialization --> f00
  if (((uint8_t)nv->value == 0) || (nv->valuetype == TYPE_0))
    st_deenergize_motors();

  else {
    uint8_t motor = (uint8_t)nv->value;
    if (motor > MOTORS) return STAT_INPUT_VALUE_RANGE_ERROR;
    _deenergize_motor(motor - 1);     // adjust so that motor 1 is actually 0 (etc)
  }

  return STAT_OK;
}


/// enable motor power
stat_t st_set_me(nvObj_t *nv) {
  // Make sure this function is not part of initialization --> f00
  if (((uint8_t)nv->value == 0) || (nv->valuetype == TYPE_0))
    st_energize_motors();

  else {
    uint8_t motor = (uint8_t)nv->value;
    if (motor > MOTORS) return STAT_INPUT_VALUE_RANGE_ERROR;
    _energize_motor(motor - 1);     // adjust so that motor 1 is actually 0 (etc)
  }

  return STAT_OK;
}


#ifdef __TEXT_MODE

static const char msg_units0[] PROGMEM = " in";    // used by generic print functions
static const char msg_units1[] PROGMEM = " mm";
static const char msg_units2[] PROGMEM = " deg";
static const char *const msg_units[] PROGMEM = {msg_units0, msg_units1, msg_units2};
#define DEGREE_INDEX 2

static const char fmt_me[] PROGMEM = "motors energized\n";
static const char fmt_md[] PROGMEM = "motors de-energized\n";
static const char fmt_mt[] PROGMEM = "[mt]  motor idle timeout%14.2f Sec\n";
static const char fmt_0ma[] PROGMEM = "[%s%s] m%s map to axis%15d [0=X,1=Y,2=Z...]\n";
static const char fmt_0sa[] PROGMEM = "[%s%s] m%s step angle%20.3f%s\n";
static const char fmt_0tr[] PROGMEM = "[%s%s] m%s travel per revolution%10.4f%s\n";
static const char fmt_0mi[] PROGMEM = "[%s%s] m%s microsteps%16d [1,2,4,8,16,32,64,128,256]\n";
static const char fmt_0po[] PROGMEM = "[%s%s] m%s polarity%18d [0=normal,1=reverse]\n";
static const char fmt_0pm[] PROGMEM = "[%s%s] m%s power management%10d [0=disabled,1=always on,2=in cycle,3=when moving]\n";
static const char fmt_0pl[] PROGMEM = "[%s%s] m%s motor power level%13.3f [0.000=minimum, 1.000=maximum]\n";
static const char fmt_pwr[] PROGMEM = "Motor %c power enabled state:%2.0f\n";

void st_print_mt(nvObj_t *nv) {text_print_flt(nv, fmt_mt);}
void st_print_me(nvObj_t *nv) {text_print_nul(nv, fmt_me);}
void st_print_md(nvObj_t *nv) {text_print_nul(nv, fmt_md);}


static void _print_motor_ui8(nvObj_t *nv, const char *format) {
  fprintf_P(stderr, format, nv->group, nv->token, nv->group, (uint8_t)nv->value);
}


static void _print_motor_flt_units(nvObj_t *nv, const char *format, uint8_t units) {
  fprintf_P(stderr, format, nv->group, nv->token, nv->group, nv->value, GET_TEXT_ITEM(msg_units, units));
}


static void _print_motor_flt(nvObj_t *nv, const char *format) {
  fprintf_P(stderr, format, nv->group, nv->token, nv->group, nv->value);
}

static void _print_motor_pwr(nvObj_t *nv, const char *format) {
  fprintf_P(stderr, format, nv->token[0], nv->value);
}


void st_print_ma(nvObj_t *nv) {_print_motor_ui8(nv, fmt_0ma);}
void st_print_sa(nvObj_t *nv) {_print_motor_flt_units(nv, fmt_0sa, DEGREE_INDEX);}
void st_print_tr(nvObj_t *nv) {_print_motor_flt_units(nv, fmt_0tr, cm_get_units_mode(MODEL));}
void st_print_mi(nvObj_t *nv) {_print_motor_ui8(nv, fmt_0mi);}
void st_print_po(nvObj_t *nv) {_print_motor_ui8(nv, fmt_0po);}
void st_print_pm(nvObj_t *nv) {_print_motor_ui8(nv, fmt_0pm);}
void st_print_pl(nvObj_t *nv) {_print_motor_flt(nv, fmt_0pl);}
void st_print_pwr(nvObj_t *nv){_print_motor_pwr(nv, fmt_pwr);}

#endif // __TEXT_MODE
