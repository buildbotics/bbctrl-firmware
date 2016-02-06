/*
 * controller.c - tinyg controller and top level parser
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

#include "controller.h"

#include "canonical_machine.h"
#include "stepper.h"
#include "gpio.h"
#include "switch.h"
#include "encoder.h"
#include "hardware.h"
#include "usart.h"
#include "util.h"
#include "rtc.h"
#include "command.h"
#include "report.h"

#include "plan/arc.h"
#include "plan/planner.h"

#include <string.h>
#include <stdio.h>


controller_t cs;        // controller state structure


static stat_t _shutdown_idler();
static stat_t _limit_switch_handler();
static stat_t _sync_to_planner();
static stat_t _sync_to_tx_buffer();


void controller_init() {
  memset(&cs, 0, sizeof(controller_t));            // clear all values, job_id's, pointers and status
}


/*
 * Main loop - top-level controller
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
  DISPATCH(hw_bootloader_handler());           // 2. handle requests to enter bootloader
  DISPATCH(_shutdown_idler());                 // 3. idle in shutdown state
  DISPATCH(_limit_switch_handler());           // 4. limit switch has been thrown
  DISPATCH(cm_feedhold_sequencing_callback()); // 5a. feedhold state machine runner
  DISPATCH(mp_plan_hold_callback());           // 5b. plan a feedhold from line runtime

  // Planner hierarchy for gcode and cycles
  DISPATCH(st_motor_power_callback());         // stepper motor power sequencing
  DISPATCH(cm_arc_callback());                 // arc generation runs behind lines
  DISPATCH(cm_homing_callback());              // G28.2 continuation
  DISPATCH(cm_jogging_callback());             // jog function
  DISPATCH(cm_probe_callback());               // G38.2 continuation
  DISPATCH(cm_deferred_write_callback());      // persist G10 changes when not in machining cycle

  // Command readers and parsers
  DISPATCH(_sync_to_planner());                // ensure at least one free buffer in planning queue
  DISPATCH(_sync_to_tx_buffer());              // sync with TX buffer (pseudo-blocking)
  DISPATCH(report_callback());
  DISPATCH(command_dispatch());                // read and execute next command
}



/// Blink rapidly and prevent further activity from occurring
/// Shutdown idler flashes indicator LED rapidly to show everything is not OK.
/// Shutdown idler returns EAGAIN causing the control loop to never advance beyond
/// this point. It's important that the reset handler is still called so a SW reset
/// (ctrl-x) or bootloader request can be processed.
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


/// _limit_switch_handler() - shut down system if limit switch fired
static stat_t _limit_switch_handler() {
  if (cm_get_machine_state() == MACHINE_ALARM) return STAT_NOOP;
  if (!get_limit_switch_thrown()) return STAT_NOOP;

  return cm_hard_alarm(STAT_LIMIT_SWITCH_HIT);
}
