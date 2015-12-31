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

#include "tinyg.h"                 // #1
#include "config.h"                // #2
#include "controller.h"
#include "json_parser.h"
#include "text_parser.h"
#include "gcode_parser.h"
#include "canonical_machine.h"
#include "plan_arc.h"
#include "planner.h"
#include "stepper.h"

#include "gpio.h"
#include "switch.h"
#include "encoder.h"
#include "hardware.h"
#include "report.h"
#include "help.h"
#include "util.h"
#include "usart.h"

controller_t cs;        // controller state structure

static stat_t _shutdown_idler();
static stat_t _normal_idler();
static stat_t _limit_switch_handler();
static stat_t _system_assertions();
static stat_t _sync_to_planner();
static stat_t _sync_to_tx_buffer();
static stat_t _command_dispatch();

// prep for export to other modules:
stat_t hardware_hard_reset_handler();
stat_t hardware_bootloader_handler();


void controller_init() {
  memset(&cs, 0, sizeof(controller_t));            // clear all values, job_id's, pointers and status
  controller_init_assertions();

  cs.fw_build = TINYG_FIRMWARE_BUILD;
  cs.fw_version = TINYG_FIRMWARE_VERSION;
  cs.hw_platform = TINYG_HARDWARE_PLATFORM;         // NB: HW version is set from EEPROM

  cs.state = CONTROLLER_STARTUP;                    // ready to run startup lines
}


/// controller_test_assertions() - check memory integrity of controller
void controller_init_assertions() {
  cs.magic_start = MAGICNUM;
  cs.magic_end = MAGICNUM;
}


stat_t controller_test_assertions() {
  if ((cs.magic_start != MAGICNUM) || (cs.magic_end != MAGICNUM))
    return STAT_CONTROLLER_ASSERTION_FAILURE;

  return STAT_OK;
}


/*
 * controller_run() - MAIN LOOP - top-level controller
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
  DISPATCH(hw_hard_reset_handler());            // 1. handle hard reset requests
  DISPATCH(hw_bootloader_handler());            // 2. handle requests to enter bootloader
  DISPATCH(_shutdown_idler());                  // 3. idle in shutdown state
  DISPATCH(_limit_switch_handler());            // 4. limit switch has been thrown
  DISPATCH(cm_feedhold_sequencing_callback());  // 5a. feedhold state machine runner
  DISPATCH(mp_plan_hold_callback());            // 5b. plan a feedhold from line runtime
  DISPATCH(_system_assertions());               // 6. system integrity assertions

  // Planner hierarchy for gcode and cycles
  DISPATCH(st_motor_power_callback());        // stepper motor power sequencing
  DISPATCH(sr_status_report_callback());      // conditionally send status report
  DISPATCH(qr_queue_report_callback());       // conditionally send queue report
  DISPATCH(rx_report_callback());             // conditionally send rx report
  DISPATCH(cm_arc_callback());                // arc generation runs behind lines
  DISPATCH(cm_homing_callback());             // G28.2 continuation
  DISPATCH(cm_jogging_callback());            // jog function
  DISPATCH(cm_probe_callback());              // G38.2 continuation
  DISPATCH(cm_deferred_write_callback());     // persist G10 changes when not in machining cycle

  // Command readers and parsers
  DISPATCH(_sync_to_planner());                // ensure there is at least one free buffer in planning queue
  DISPATCH(_sync_to_tx_buffer());              // sync with TX buffer (pseudo-blocking)
  DISPATCH(set_baud_callback());               // perform baud rate update (must be after TX sync)
  DISPATCH(_command_dispatch());               // read and execute next command
  DISPATCH(_normal_idler());                   // blink LEDs slowly to show everything is OK
}


/*****************************************************************************
 * _command_dispatch() - dispatch line received from active input device
 *
 *    Reads next command line and dispatches to relevant parser or action
 *    Accepts commands if the move queue has room - EAGAINS if it doesn't
 *    Also responsible for prompts and for flow control
 */
static stat_t _command_dispatch() {
  stat_t status;

  // read input line or return if not a completed line
  // usart_gets() is a non-blocking workalike of fgets()
  while (true) {
    if ((status = usart_gets(cs.in_buf, sizeof(cs.in_buf))) == STAT_OK) {
      cs.bufp = cs.in_buf;
      break;
    }

    return status;                                // Note: STAT_EAGAIN, errors, etc. will drop through
  }

  // set up the buffers
  cs.linelen = strlen(cs.in_buf) + 1;                    // linelen only tracks primary input
  strncpy(cs.saved_buf, cs.bufp, SAVED_BUFFER_LEN - 1);  // save input buffer for reporting

  // dispatch the new text line
  switch (toupper(*cs.bufp)) {                        // first char
  case '!': cm_request_feedhold(); break;             // include for diagnostics
  case '%': cm_request_queue_flush(); break;
  case '~': cm_request_cycle_start(); break;

  case 0:                                        // blank line (just a CR)
    if (cfg.comm_mode != JSON_MODE)
      text_response(STAT_OK, cs.saved_buf);
    break;

  case '$': case '?': case 'H':                  // text mode input
    cfg.comm_mode = TEXT_MODE;
    text_response(text_parser(cs.bufp), cs.saved_buf);
    break;

  case '{':                                      // JSON input
    cfg.comm_mode = JSON_MODE;
    json_parser(cs.bufp);
    break;

  default:                                          // anything else must be Gcode
    if (cfg.comm_mode == JSON_MODE) {             // run it as JSON...
      strncpy(cs.out_buf, cs.bufp, INPUT_BUFFER_LEN -8);                    // use out_buf as temp
      sprintf((char *)cs.bufp,"{\"gc\":\"%s\"}\n", (char *)cs.out_buf);     // '-8' is used for JSON chars
      json_parser(cs.bufp);

    } else text_response(gc_gcode_parser(cs.bufp), cs.saved_buf);           //...or run it as text
  }

  return STAT_OK;
}


/**** Local Utilities ********************************************************/
/*
 * _shutdown_idler() - blink rapidly and prevent further activity from occurring
 * _normal_idler() - blink Indicator LED slowly to show everything is OK
 *
 *    Shutdown idler flashes indicator LED rapidly to show everything is not OK.
 *    Shutdown idler returns EAGAIN causing the control loop to never advance beyond
 *    this point. It's important that the reset handler is still called so a SW reset
 *    (ctrl-x) or bootloader request can be processed.
 */

static stat_t _shutdown_idler() {
  if (cm_get_machine_state() != MACHINE_SHUTDOWN) return STAT_OK;

  if (SysTickTimer_getValue() > cs.led_timer) {
    cs.led_timer = SysTickTimer_getValue() + LED_ALARM_TIMER;
    IndicatorLed_toggle();
  }

  return STAT_EAGAIN;    // EAGAIN prevents any lower-priority actions from running
}


static stat_t _normal_idler() {
  return STAT_OK;
}

/*
 * _sync_to_tx_buffer() - return eagain if TX queue is backed up
 * _sync_to_planner() - return eagain if planner is not ready for a new command
 * _sync_to_time() - return eagain if planner is not ready for a new command
 */
static stat_t _sync_to_tx_buffer() {
  if (usart_tx_full()) return STAT_EAGAIN;

  return STAT_OK;
}


static stat_t _sync_to_planner() {
  if (mp_get_planner_buffers_available() < PLANNER_BUFFER_HEADROOM) // allow up to N planner buffers for this line
    return STAT_EAGAIN;

  return STAT_OK;
}


/// _limit_switch_handler() - shut down system if limit switch fired
static stat_t _limit_switch_handler() {
  if (cm_get_machine_state() == MACHINE_ALARM) return STAT_NOOP;
  if (!get_limit_switch_thrown()) return STAT_NOOP;

  return cm_hard_alarm(STAT_LIMIT_SWITCH_HIT);
}


/// _system_assertions() - check memory integrity and other assertions
stat_t _system_assertions() {
#define system_assert(a) if ((status_code = a) != STAT_OK) return cm_hard_alarm(status_code);

  system_assert(config_test_assertions());
  system_assert(controller_test_assertions());
  system_assert(canonical_machine_test_assertions());
  system_assert(planner_test_assertions());
  system_assert(stepper_test_assertions());
  system_assert(encoder_test_assertions());

  return STAT_OK;
}
