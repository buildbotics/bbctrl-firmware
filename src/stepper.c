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

#include "motor.h"
#include "config.h"
#include "hardware.h"
#include "util.h"
#include "cpp_magic.h"

#include <string.h>


typedef struct {
  // Runtime
  bool busy;
  moveType_t run_move;
  uint16_t run_dwell;

  // Move prep
  bool move_ready;      // preped move ready for loader
  moveType_t prep_move;
  uint32_t prep_dwell;
  struct mpBuffer *bf;  // used for command moves
  uint16_t seg_period;
} stepper_t;


static stepper_t st;


void stepper_init() {
  /// clear all values, pointers and status
  memset(&st, 0, sizeof(st));

  // Setup step timer
  TIMER_STEP.CTRLA = STEP_TIMER_DISABLE;   // turn timer off
  TIMER_STEP.CTRLB = STEP_TIMER_WGMODE;    // waveform mode
  TIMER_STEP.INTCTRLA = STEP_TIMER_INTLVL; // interrupt mode
}


/// Return true if motors or dwell are running
uint8_t st_runtime_isbusy() {return st.busy;}


/// "Software" interrupt to request move execution
void st_request_exec_move() {
  if (!st.move_ready) {
    ADCB_CH0_INTCTRL = ADC_CH_INTLVL_LO_gc; // trigger LO interrupt
    ADCB_CTRLA = ADC_ENABLE_bm | ADC_CH0START_bm;
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

  // If there are no more moves
  if (!st.move_ready) return;

  for (int motor = 0; motor < MOTORS; motor++)
    motor_begin_move(motor);

  // Wait until motors have energized
  if (motor_energizing()) return;

  st.run_move = st.prep_move;

  switch (st.run_move) {
  case MOVE_TYPE_DWELL:
    st.run_dwell = st.prep_dwell;
    // Fall through

  case MOVE_TYPE_ALINE:
    for (int motor = 0; motor < MOTORS; motor++)
      if (st.run_move == MOVE_TYPE_ALINE)
        motor_load_move(motor);

    st.busy = true;
    TIMER_STEP.PER = st.seg_period;
    TIMER_STEP.CTRLA = STEP_TIMER_ENABLE; // enable step timer, if not enabled
    break;

  case MOVE_TYPE_COMMAND: // handle synchronous commands
    mp_runtime_command(st.bf);
    // Fall through

  default:
    TIMER_STEP.CTRLA = STEP_TIMER_DISABLE;
    break;
  }

  // we are done with the prep buffer - flip the flag back
  st.prep_move = MOVE_TYPE_NULL;
  st.move_ready = false;
  st_request_exec_move(); // exec and prep next move
}


/// Step timer interrupt routine
ISR(STEP_TIMER_ISR) {
  if (st.run_move == MOVE_TYPE_DWELL && --st.run_dwell) return;

  for (int motor = 0; motor < MOTORS; motor++)
    motor_end_move(motor);

  st.busy = false;
  _load_move();
}


/// Interrupt handler for running the loader.
/// ADC channel 1 triggered by st_request_load_move() as a "software" interrupt.
ISR(ADCB_CH1_vect) {
  _load_move();
}


/// Fires a "software" interrupt to request a move load.
void st_request_load_move() {
  if (st_runtime_isbusy()) return; // don't request a load if runtime is busy

  if (st.move_ready) {
    ADCB_CH1_INTCTRL = ADC_CH_INTLVL_HI_gc; // trigger HI interrupt
    ADCB_CTRLA = ADC_ENABLE_bm | ADC_CH1START_bm;
  }
}


/// Interrupt handler for calling move exec function.
/// ADC channel 0 triggered by st_request_exec_move() as a "software" interrupt.
ISR(ADCB_CH0_vect) {
  if (!st.move_ready && mp_exec_move() != STAT_NOOP) {
    st.move_ready = true; // flip it back - TODO is this really needed?
    st_request_load_move();
  }
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
 *   - seg_time - how many minutes the segment should run. If timing is not
 *     100% accurate this will affect the move velocity, but not the distance
 *     traveled.
 */
stat_t st_prep_line(float travel_steps[], float error[], float seg_time) {
  // Trap conditions that would prevent queueing the line
  if (st.move_ready) return cm_hard_alarm(STAT_INTERNAL_ERROR);
  if (isinf(seg_time))
    return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_INFINITE);
  if (isnan(seg_time)) return cm_hard_alarm(STAT_PREP_LINE_MOVE_TIME_IS_NAN);
  if (seg_time < EPSILON) return STAT_MINIMUM_TIME_MOVE;

  // Setup segment parameters
  st.prep_move = MOVE_TYPE_ALINE;
  st.seg_period = seg_time * 60 * F_CPU / STEP_TIMER_DIV; // Must fit 16-bit
  uint32_t seg_clocks = (uint32_t)st.seg_period * STEP_TIMER_DIV;

  // Prepare motor moves
  for (uint8_t motor = 0; motor < MOTORS; motor++)
    motor_prep_move(motor, seg_clocks, travel_steps[motor], error[motor]);

  st.move_ready = true; // signal prep buffer ready(do this last)

  return STAT_OK;
}


/// Keeps the loader happy. Otherwise performs no action
void st_prep_null() {
  if (st.move_ready) cm_hard_alarm(STAT_INTERNAL_ERROR);
  st.prep_move = MOVE_TYPE_NULL;
  st.move_ready = false; // signal prep buffer empty
}


/// Stage command to execution
void st_prep_command(void *bf) {
  if (st.move_ready) cm_hard_alarm(STAT_INTERNAL_ERROR);
  st.prep_move = MOVE_TYPE_COMMAND;
  st.bf = (mpBuf_t *)bf;
  st.move_ready = true; // signal prep buffer ready
}


/// Add a dwell to the move buffer
void st_prep_dwell(float seconds) {
  if (st.move_ready) cm_hard_alarm(STAT_INTERNAL_ERROR);
  st.prep_move = MOVE_TYPE_DWELL;
  st.seg_period = F_CPU / STEP_TIMER_DIV / 1000; // 1 ms
  st.prep_dwell = seconds * 1000; // convert to ms
  st.move_ready = true; // signal prep buffer ready
}
