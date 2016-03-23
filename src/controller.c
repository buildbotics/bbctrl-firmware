/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2012 - 2015 Rob Giseburt
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

#include "controller.h"

#include "canonical_machine.h"
#include "stepper.h"
#include "motor.h"
#include "tmc2660.h"
#include "gpio.h"
#include "switch.h"
#include "encoder.h"
#include "hardware.h"
#include "usart.h"
#include "util.h"
#include "rtc.h"
#include "command.h"
#include "report.h"

#include "plan/buffer.h"
#include "plan/arc.h"
#include "plan/feedhold.h"

#include <string.h>
#include <stdio.h>


controller_t cs;        // controller state structure


/// Blink rapidly and prevent further activity from occurring Shutdown
/// idler flashes indicator LED rapidly to show everything is not OK.
/// Shutdown idler returns EAGAIN causing the control loop to never
/// advance beyond this point. It's important that the reset handler
/// is still called so a SW reset (ctrl-x) or bootloader request can
/// be processed.
static stat_t _shutdown_idler() {
  if (cm_get_machine_state() != MACHINE_SHUTDOWN) return STAT_OK;

  if (rtc_get_time() > cs.led_timer) {
    cs.led_timer = rtc_get_time() + LED_ALARM_TIMER;
    indicator_led_toggle();
  }

  return STAT_EAGAIN; // EAGAIN prevents any lower-priority actions from running
}


/// Return eagain if TX queue is backed up
static stat_t _sync_to_tx_buffer() {
  if (usart_tx_full()) return STAT_EAGAIN;

  return STAT_OK;
}


/// Return eagain if planner is not ready for a new command
static stat_t _sync_to_planner() {
  // allow up to N planner buffers for this line
  if (mp_get_planner_buffers_available() < PLANNER_BUFFER_HEADROOM)
    return STAT_EAGAIN;

  return STAT_OK;
}


/// Shut down system if limit switch fired
static stat_t _limit_switch_handler() {
  if (cm_get_machine_state() == MACHINE_ALARM) return STAT_NOOP;
  if (!switch_get_limit_thrown()) return STAT_NOOP;

  return cm_hard_alarm(STAT_LIMIT_SWITCH_HIT);
}


void controller_init() {
  // clear all values, job_id's, pointers and status
  memset(&cs, 0, sizeof(controller_t));
}


/* Main loop - top-level controller
 *
 * The order of the dispatched tasks is very important.
 * Tasks are ordered by increasing dependency (blocking hierarchy).
 * Tasks that are dependent on completion of lower-level tasks must be
 * later in the list than the task(s) they are dependent upon.
 *
 * Tasks must be written as continuations as they will be called repeatedly,
 * and are called even if they are not currently active.
 *
 * The DISPATCH macro calls the function and returns to the controller parent
 * if not finished (STAT_EAGAIN), preventing later routines from running
 * (they remain blocked). Any other condition - OK or ERR - drops through
 * and runs the next routine in the list.
 *
 * A routine that had no action (i.e. is OFF or idle) should return STAT_NOOP
 */
void controller_run() {
#define DISPATCH(func) if (func == STAT_EAGAIN) return;

  // Interrupt Service Routines are the highest priority controller functions
  // See hardware.h for a list of ISRs and their priorities.

  // Kernel level ISR handlers, flags are set in ISRs, order is important
  DISPATCH(hw_hard_reset_handler());           // 1. handle hard reset requests
  DISPATCH(hw_bootloader_handler());           // 2. handle bootloader requests
  DISPATCH(_shutdown_idler());                 // 3. idle in shutdown state
  DISPATCH(_limit_switch_handler());           // 4. limit switch thrown
  DISPATCH(cm_feedhold_sequencing_callback()); // 5a. feedhold state machine
  DISPATCH(mp_plan_hold_callback());           // 5b. plan a feedhold

  DISPATCH(tmc2660_sync());                    // synchronize driver config
  DISPATCH(motor_power_callback());            // stepper motor power sequencing

  // Planner hierarchy for gcode and cycles
  DISPATCH(cm_arc_callback());                 // arc generation runs
  DISPATCH(cm_homing_callback());              // G28.2 continuation
  DISPATCH(cm_probe_callback());               // G38.2 continuation

  // Command readers and parsers
  // ensure at least one free buffer in planning queue
  DISPATCH(_sync_to_planner());
  // sync with TX buffer (pseudo-blocking)
  DISPATCH(_sync_to_tx_buffer());

  DISPATCH(report_callback());
  DISPATCH(command_dispatch());                // read and execute next command
}
