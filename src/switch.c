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
 *
 * These functions interact with each other to process switch closures
 * and firing.  Each switch has a counter which is initially set to
 * negative SW_DEGLITCH_TICKS.  When a switch closure is DETECTED the
 * count increments for each RTC tick.  When the count reaches zero
 * the switch is tripped and action occurs.  The counter continues to
 * increment positive until the lockout is exceeded.
 */

#include "switch.h"

#include "hardware.h"
#include "canonical_machine.h"
#include "config.h"

#include <avr/interrupt.h>

#include <stdbool.h>


typedef enum { // state machine for managing debouncing and lockout
  SW_IDLE,
  SW_DEGLITCHING,
  SW_LOCKOUT
} swDebounce_t;

typedef struct {
  bool last;
  bool state;
  swType_t type;
  swMode_t mode;
  swDebounce_t debounce; // debounce state
  int8_t count;          // deglitching and lockout counter
  PORT_t *port;
  bool min;
} switch_t;

/* Switch control structures
 * Note 1: The term "thrown" is used because switches could be normally-open
 * or normally-closed. "Thrown" means activated.
 */
typedef struct {
  bool limit_thrown;
  switch_t switches[SWITCHES];
} swSingleton_t;


swSingleton_t sw = {
  .switches = {
    {
      .type = SWITCH_TYPE,
      .mode = X_SWITCH_MODE_MIN,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_X,
      .min = true,
    }, {
      .type = SWITCH_TYPE,
      .mode = X_SWITCH_MODE_MAX,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_X,
      .min = false,
    }, {
      .type = SWITCH_TYPE,
      .mode = Y_SWITCH_MODE_MIN,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_Y,
      .min = true,
   }, {
      .type = SWITCH_TYPE,
      .mode = Y_SWITCH_MODE_MAX,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_Y,
      .min = false,
    }, {
      .type = SWITCH_TYPE,
      .mode = Z_SWITCH_MODE_MIN,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_Z,
      .min = true,
    }, {
      .type = SWITCH_TYPE,
      .mode = Z_SWITCH_MODE_MAX,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_Z,
      .min = false,
    }, {
      .type = SWITCH_TYPE,
      .mode = A_SWITCH_MODE_MIN,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_A,
      .min = true,
    }, {
      .type = SWITCH_TYPE,
      .mode = A_SWITCH_MODE_MAX,
      .debounce = SW_IDLE,
      .port = &PORT_SWITCH_A,
      .min = false,
    },
  }
};


static bool _read_switch(uint8_t i) {
  return sw.switches[i].port->IN &
    (sw.switches[i].min ? SW_MIN_BIT_bm : SW_MAX_BIT_bm);
}


static void _switch_isr() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &sw.switches[i];

    bool set = _read_switch(i);
    if (set == s->last) continue;

    if (s->mode == SW_MODE_DISABLED) return; // never supposed to happen
    if (s->debounce == SW_LOCKOUT) return;   // switch is in lockout

    // either transitions state from IDLE or overwrites it
    s->debounce = SW_DEGLITCHING;
    // reset deglitch count regardless of entry state
    s->count = -SW_DEGLITCH_TICKS;

    // A NO switch drives the pin LO when thrown
    s->state = (s->type == SW_TYPE_NORMALLY_OPEN) ^ set;
  }
}


// Switch interrupt handler vectors
ISR(X_SWITCH_ISR_vect) {_switch_isr();}
ISR(Y_SWITCH_ISR_vect) {_switch_isr();}
ISR(Z_SWITCH_ISR_vect) {_switch_isr();}
ISR(A_SWITCH_ISR_vect) {_switch_isr();}


void switch_init() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &sw.switches[i];
    PORT_t *port = s->port;
    uint8_t bm = s->min ? SW_MIN_BIT_bm : SW_MAX_BIT_bm;

    if (s->mode == SW_MODE_DISABLED) continue;

    port->DIRCLR = bm;   // See 13.14.14
    port->INT0MASK |= bm; // Enable INT0
    port->INTCTRL |= SWITCH_INTLVL; // Set interrupt level

    if (s->min) port->PIN6CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;
    else port->PIN7CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;

    // Initialize state
    s->state = (s->type == SW_TYPE_NORMALLY_OPEN) ^ _read_switch(i);
  }
}


/// Called from RTC on each tick
void switch_rtc_callback() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &sw.switches[i];

    if (s->mode == SW_MODE_DISABLED || s->debounce == SW_IDLE)
      continue;

    // state is either lockout or deglitching
    if (++s->count == SW_LOCKOUT_TICKS) {
      s->debounce = SW_IDLE;

      // check if the state has changed while we were in lockout...
      bool old_state = s->state;
      if (old_state != _read_switch(i)) {
        s->debounce = SW_DEGLITCHING;
        s->count = -SW_DEGLITCH_TICKS;
      }

      continue;
    }

    if (!s->count) { // trigger point
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


bool switch_get_closed(int index) {
  return sw.switches[index].state;
}


swType_t switch_get_type(int index) {
  return sw.switches[index].type;
}


void switch_set_type(int index, swType_t type) {
  sw.switches[index].type = type;
}


swMode_t switch_get_mode(int index) {
  return sw.switches[index].mode;
}


void switch_set_mode(int index, swMode_t mode) {
  sw.switches[index].mode = mode;
}


bool switch_get_limit_thrown() {
  return sw.limit_thrown;
}


uint8_t get_switch_type(int index) {
  return sw.switches[index].type;
}


void set_switch_type(int index, uint8_t value) {
  sw.switches[index].type = value;
}
