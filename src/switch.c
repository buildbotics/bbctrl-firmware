/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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
 * lockout period.
 */

#include "switch.h"

#include "hardware.h"
#include "canonical_machine.h"
#include "config.h"

#include <avr/interrupt.h>

#include <stdbool.h>


swSingleton_t sw;


/* Initialize homing/limit switches
 *
 * This function assumes sys_init() and st_init() have been run previously to
 * bind the ports and set bit IO directions, repsectively.
 */
void switch_init() {
  for (int i = 0; i < NUM_SWITCH_PAIRS; i++) {
    // setup input bits and interrupts (previously set to inputs by st_init())
    if (sw.switches[MIN_SWITCH(i)].mode != SW_MODE_DISABLED) {
      hw.sw_port[i]->DIRCLR = SW_MIN_BIT_bm;   // set min input - see 13.14.14
      hw.sw_port[i]->PIN6CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;
      hw.sw_port[i]->INT0MASK = SW_MIN_BIT_bm; // min on INT0

    } else hw.sw_port[i]->INT0MASK = 0; // disable interrupt

    if (sw.switches[MAX_SWITCH(i)].mode != SW_MODE_DISABLED) {
      hw.sw_port[i]->DIRCLR = SW_MAX_BIT_bm;   // set max input - see 13.14.14
      hw.sw_port[i]->PIN7CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;
      hw.sw_port[i]->INT1MASK = SW_MAX_BIT_bm; // max on INT1

    } else hw.sw_port[i]->INT1MASK = 0; // disable interrupt

    // set interrupt levels. Interrupts must be enabled in main()
    hw.sw_port[i]->INTCTRL = SWITCH_INTLVL;
  }

  // Defaults
  for (int i = 0; i < SWITCHES; i++)
    sw.switches[i].type = SWITCH_TYPE;

  sw.switches[0].mode = X_SWITCH_MODE_MIN;
  sw.switches[1].mode = X_SWITCH_MODE_MAX;
  sw.switches[2].mode = Y_SWITCH_MODE_MIN;
  sw.switches[3].mode = Y_SWITCH_MODE_MAX;
  sw.switches[4].mode = Z_SWITCH_MODE_MIN;
  sw.switches[5].mode = Z_SWITCH_MODE_MAX;
  sw.switches[6].mode = A_SWITCH_MODE_MIN;
  sw.switches[7].mode = A_SWITCH_MODE_MAX;

  reset_switches();
}


/* These functions interact with each other to process switch closures
 * and firing.  Each switch has a counter which is initially set to
 * negative SW_DEGLITCH_TICKS.  When a switch closure is DETECTED the
 * count increments for each RTC tick.  When the count reaches zero
 * the switch is tripped and action occurs.  The counter continues to
 * increment positive until the lockout is exceeded.
 */

static void _switch_isr(uint8_t sw_num) {
  switch_t *s = &sw.switches[sw_num];

  if (s->mode == SW_MODE_DISABLED) return; // never supposed to happen
  if (s->debounce == SW_LOCKOUT) return;   // switch is in lockout

  // either transitions state from IDLE or overwrites it
  s->debounce = SW_DEGLITCHING;
  // reset deglitch count regardless of entry state
  s->count = -SW_DEGLITCH_TICKS;

  // sets the state value in the struct
  read_switch(sw_num);
}


// Switch interrupt handler vectors
ISR(X_MIN_ISR_vect) {_switch_isr(SW_MIN_X);}
ISR(Y_MIN_ISR_vect) {_switch_isr(SW_MIN_Y);}
ISR(Z_MIN_ISR_vect) {_switch_isr(SW_MIN_Z);}
ISR(A_MIN_ISR_vect) {_switch_isr(SW_MIN_A);}
ISR(X_MAX_ISR_vect) {_switch_isr(SW_MAX_X);}
ISR(Y_MAX_ISR_vect) {_switch_isr(SW_MAX_Y);}
ISR(Z_MAX_ISR_vect) {_switch_isr(SW_MAX_Z);}
ISR(A_MAX_ISR_vect) {_switch_isr(SW_MAX_A);}


/// Called from RTC for each RTC tick
void switch_rtc_callback() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &sw.switches[i];

    if (s->mode == SW_MODE_DISABLED || s->debounce == SW_IDLE)
      continue;

    // state is either lockout or deglitching
    if (++s->count == SW_LOCKOUT_TICKS) {
      s->debounce = SW_IDLE;

      // check if the state has changed while we were in lockout...
      uint8_t old_state = s->state;
      if (old_state != read_switch(i)) {
        s->debounce = SW_DEGLITCHING;
        s->count = -SW_DEGLITCH_TICKS;
      }

      continue;
    }

    if (!s->count) { // trigger point
      sw.switch_thrown = i; // record number of thrown switch
      s->debounce = SW_LOCKOUT;

      // regardless of switch type
      if (cm.cycle_state == CYCLE_HOMING || cm.cycle_state == CYCLE_PROBE)
        cm_request_feedhold();

      // should be a limit switch, so fire it.
      else if (s->mode & SW_LIMIT_BIT)
        sw.limit_thrown = true; // triggers an emergency shutdown
    }
  }
}


/// return switch mode setting
swMode_t get_switch_mode(uint8_t sw_num) {return sw.switches[sw_num].mode;}

/// return true if a limit was tripped
bool get_limit_switch_thrown() {return sw.limit_thrown;}


/// reset all switches and reset limit flag
void reset_switches() {
  for (uint8_t i = 0; i < SWITCHES; i++) {
    sw.switches[i].debounce = SW_IDLE;
    read_switch(i);
  }

  sw.limit_thrown = false;
}


/// read a switch directly with no interrupts or deglitching
uint8_t read_switch(uint8_t sw_num) {
  if (sw_num < 0 || sw_num >= SWITCHES) return SW_DISABLED;

  bool hi = false;
  switch (sw_num) {
  case SW_MIN_X: hi = hw.sw_port[AXIS_X]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_X: hi = hw.sw_port[AXIS_X]->IN & SW_MAX_BIT_bm; break;
  case SW_MIN_Y: hi = hw.sw_port[AXIS_Y]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_Y: hi = hw.sw_port[AXIS_Y]->IN & SW_MAX_BIT_bm; break;
  case SW_MIN_Z: hi = hw.sw_port[AXIS_Z]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_Z: hi = hw.sw_port[AXIS_Z]->IN & SW_MAX_BIT_bm; break;
  case SW_MIN_A: hi = hw.sw_port[AXIS_A]->IN & SW_MIN_BIT_bm; break;
  case SW_MAX_A: hi = hw.sw_port[AXIS_A]->IN & SW_MAX_BIT_bm; break;
  }

  // A NO switch drives the pin LO when thrown
  if (sw.switches[sw_num].type == SW_TYPE_NORMALLY_OPEN)
    sw.switches[sw_num].state = hi ? SW_OPEN : SW_CLOSED;
  else sw.switches[sw_num].state = hi ? SW_CLOSED : SW_OPEN;

  return sw.switches[sw_num].state;
}


uint8_t get_switch_type(int index) {
  return sw.switches[index].type;
}


void set_switch_type(int index, uint8_t value) {
  sw.switches[index].type = value;
}
