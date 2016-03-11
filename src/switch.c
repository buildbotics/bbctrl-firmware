/*
 * switch.c - switch handling functions
 * This file is part of the TinyG project
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
 *
 * This file ("the software") is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License, version 2 as published by the Free Software
 * Foundation. You should have received a copy of the GNU General
 * Public License, version 2 along with the software. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software
 * library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with other files to
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

/* Switch Modes
 *
 * The switches are considered to be homing switches when machine_state is
 * MACHINE_HOMING. At all other times they are treated as limit switches:
 *
 *   - Hitting a homing switch puts the current move into feedhold
 *
 *   - Hitting a limit switch causes the machine to shut down and go into
 *     lockdown until reset
 *
 * The normally open switch modes (NO) trigger an interrupt on the
 * falling edge and lockout subsequent interrupts for the defined
 * lockout period. This approach beats doing debouncing as an
 * integration as switches fire immediately.
 *
 * The normally closed switch modes (NC) trigger an interrupt on the
 * rising edge and lockout subsequent interrupts for the defined
 * lockout period. Ditto on the method.
 */

#include "switch.h"

#include "hardware.h"
#include "canonical_machine.h"
#include "config.h"

#include <avr/interrupt.h>

#include <stdbool.h>


static void _switch_isr_helper(uint8_t sw_num);

/* Initialize homing/limit switches
 *
 * This function assumes sys_init() and st_init() have been run previously to
 * bind the ports and set bit IO directions, repsectively.
 */
#define PIN_MODE PORT_OPC_PULLUP_gc // pin mode. see iox192a3.h for details


void switch_init() {
  for (uint8_t i = 0; i < NUM_SWITCH_PAIRS; i++) {
    // setup input bits and interrupts (previously set to inputs by st_init())
    if (sw.mode[MIN_SWITCH(i)] != SW_MODE_DISABLED) {
      // set min input - see 13.14.14
      hw.sw_port[i]->DIRCLR = SW_MIN_BIT_bm;
      hw.sw_port[i]->PIN6CTRL = (PIN_MODE | PORT_ISC_BOTHEDGES_gc);
      // interrupt on min switch
      hw.sw_port[i]->INT0MASK = SW_MIN_BIT_bm;

    } else hw.sw_port[i]->INT0MASK = 0; // disable interrupt

    if (sw.mode[MAX_SWITCH(i)] != SW_MODE_DISABLED) {
      // set max input - see 13.14.14
      hw.sw_port[i]->DIRCLR = SW_MAX_BIT_bm;
      hw.sw_port[i]->PIN7CTRL = (PIN_MODE | PORT_ISC_BOTHEDGES_gc);
      hw.sw_port[i]->INT1MASK = SW_MAX_BIT_bm; // max on INT1

    } else hw.sw_port[i]->INT1MASK = 0;

    // set interrupt levels. Interrupts must be enabled in main()
    hw.sw_port[i]->INTCTRL = GPIO1_INTLVL; // see gpio.h for setting
  }

  // Defaults
  sw.switch_type = SWITCH_TYPE;
  sw.mode[0] = X_SWITCH_MODE_MIN;
  sw.mode[1] = X_SWITCH_MODE_MAX;
  sw.mode[2] = Y_SWITCH_MODE_MIN;
  sw.mode[3] = Y_SWITCH_MODE_MAX;
  sw.mode[4] = Z_SWITCH_MODE_MIN;
  sw.mode[5] = Z_SWITCH_MODE_MAX;
  sw.mode[6] = A_SWITCH_MODE_MIN;
  sw.mode[7] = A_SWITCH_MODE_MAX;

  reset_switches();
}


/*
 * These functions interact with each other to process switch closures
 * and firing.  Each switch has a counter which is initially set to
 * negative SW_DEGLITCH_TICKS.  When a switch closure is DETECTED the
 * count increments for each RTC tick.  When the count reaches zero
 * the switch is tripped and action occurs.  The counter continues to
 * increment positive until the lockout is exceeded.
 */

// Switch interrupt handler vectors
ISR(X_MIN_ISR_vect) {_switch_isr_helper(SW_MIN_X);}
ISR(Y_MIN_ISR_vect) {_switch_isr_helper(SW_MIN_Y);}
ISR(Z_MIN_ISR_vect) {_switch_isr_helper(SW_MIN_Z);}
ISR(A_MIN_ISR_vect) {_switch_isr_helper(SW_MIN_A);}
ISR(X_MAX_ISR_vect) {_switch_isr_helper(SW_MAX_X);}
ISR(Y_MAX_ISR_vect) {_switch_isr_helper(SW_MAX_Y);}
ISR(Z_MAX_ISR_vect) {_switch_isr_helper(SW_MAX_Z);}
ISR(A_MAX_ISR_vect) {_switch_isr_helper(SW_MAX_A);}


static void _switch_isr_helper(uint8_t sw_num) {
  if (sw.mode[sw_num] == SW_MODE_DISABLED) return; // never supposed to happen
  if (sw.debounce[sw_num] == SW_LOCKOUT) return;   // switch is in lockout

  // either transitions state from IDLE or overwrites it
  sw.debounce[sw_num] = SW_DEGLITCHING;
  // reset deglitch count regardless of entry state
  sw.count[sw_num] = -SW_DEGLITCH_TICKS;

  // sets the state value in the struct
  read_switch(sw_num);
}


/// Called from RTC for each RTC tick
void switch_rtc_callback() {
  for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
    if (sw.mode[i] == SW_MODE_DISABLED || sw.debounce[i] == SW_IDLE)
      continue;

    // state is either lockout or deglitching
    if (++sw.count[i] == SW_LOCKOUT_TICKS) {
      sw.debounce[i] = SW_IDLE;

      // check if the state has changed while we were in lockout...
      uint8_t old_state = sw.state[i];
      if (old_state != read_switch(i)) {
        sw.debounce[i] = SW_DEGLITCHING;
        sw.count[i] = -SW_DEGLITCH_TICKS;
      }

      continue;
    }

    if (sw.count[i] == 0) { // trigger point
      sw.sw_num_thrown = i; // record number of thrown switch
      sw.debounce[i] = SW_LOCKOUT;

      // regardless of switch type
      if (cm.cycle_state == CYCLE_HOMING || cm.cycle_state == CYCLE_PROBE)
        cm_request_feedhold();

      // should be a limit switch, so fire it.
      else if (sw.mode[i] & SW_LIMIT_BIT)
        sw.limit_flag = true; // triggers an emergency shutdown
    }
  }
}


/// return switch mode setting
uint8_t get_switch_mode(uint8_t sw_num) {return sw.mode[sw_num];}

/// return true if a limit was tripped
uint8_t get_limit_switch_thrown() {return sw.limit_flag;}

/// return switch number most recently thrown
uint8_t get_switch_thrown() {return sw.sw_num_thrown;}


// global switch type
void set_switch_type(uint8_t switch_type) {sw.switch_type = switch_type;}
uint8_t get_switch_type() {return sw.switch_type;}


/// reset all switches and reset limit flag
void reset_switches() {
  for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
    sw.debounce[i] = SW_IDLE;
    read_switch(i);
  }

  sw.limit_flag = false;
}


/// read a switch directly with no interrupts or deglitching
uint8_t read_switch(uint8_t sw_num) {
  if ((sw_num < 0) || (sw_num >= NUM_SWITCHES)) return SW_DISABLED;

  uint8_t read = 0;
  switch (sw_num) {
  case SW_MIN_X: read = hw.sw_port[AXIS_X]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_X: read = hw.sw_port[AXIS_X]->IN & SW_MAX_BIT_bm; break;
  case SW_MIN_Y: read = hw.sw_port[AXIS_Y]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_Y: read = hw.sw_port[AXIS_Y]->IN & SW_MAX_BIT_bm; break;
  case SW_MIN_Z: read = hw.sw_port[AXIS_Z]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_Z: read = hw.sw_port[AXIS_Z]->IN & SW_MAX_BIT_bm; break;
  case SW_MIN_A: read = hw.sw_port[AXIS_A]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_A: read = hw.sw_port[AXIS_A]->IN & SW_MAX_BIT_bm; break;
  }

  // A NO switch drives the pin LO when thrown
  if (sw.switch_type == SW_TYPE_NORMALLY_OPEN)
    sw.state[sw_num] = (read == 0) ? SW_CLOSED : SW_OPEN;
  else sw.state[sw_num] = (read != 0) ? SW_CLOSED : SW_OPEN;

  return sw.state[sw_num];
}
