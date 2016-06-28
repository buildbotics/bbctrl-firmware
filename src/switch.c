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


typedef enum {
  SW_IDLE,
  SW_DEGLITCHING,
  SW_LOCKOUT
} swDebounce_t;


typedef struct {
  swType_t type;
  swMode_t mode;
  uint8_t pin;
  bool min;

  bool state;
  swDebounce_t debounce;
  int8_t count;
} switch_t;


typedef struct {
  bool limit_thrown;
  switch_t switches[SWITCHES];
} swSingleton_t;


swSingleton_t sw = {
  .switches = {
    { //    X min
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_HOMING,
      .pin  = MIN_X_PIN,
      .min  = true,
    }, { // X max
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_DISABLED,
      .pin  = MAX_X_PIN,
      .min  = false,
    }, { // Y min
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_HOMING,
      .pin  = MIN_Y_PIN,
      .min  = true,
    }, { // Y max
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_DISABLED,
      .pin  = MAX_X_PIN,
      .min  = false,
    }, { // Z min
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_DISABLED,
      .pin  = MIN_Z_PIN,
      .min  = true,
    }, { // Z max
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_HOMING,
      .pin  = MAX_Z_PIN,
      .min  = false,
    }, { // A min
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_DISABLED, // SW_MODE_HOMING,
      .pin  = MIN_A_PIN,
      .min = true,
    }, { // A max
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_MODE_DISABLED,
      .pin  = MAX_A_PIN,
      .min  = false,
    }, { // EStop
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_ESTOP_BIT,
      .pin  = ESTOP_PIN,
    }, { // Probe
      .type = SW_TYPE_NORMALLY_OPEN,
      .mode = SW_PROBE_BIT,
      .pin  = PROBE_PIN,
    },
  }
};


static bool _read_state(const switch_t *s) {
  // A normally open switch drives the pin low when thrown
  return (s->type == SW_TYPE_NORMALLY_OPEN) ^ IN_PIN(s->pin);
}


static void _switch_isr() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &sw.switches[i];

    bool state = _read_state(s);
    if (state == s->state || s->mode == SW_MODE_DISABLED ||
        s->debounce == SW_LOCKOUT) continue;

    // either transitions state from IDLE or overwrites it
    s->debounce = SW_DEGLITCHING;
    // reset deglitch count regardless of entry state
    s->count = -SW_DEGLITCH_TICKS;
    s->state = state;
    PORT(s->pin)->INT0MASK &= ~BM(s->pin); // Disable INT0
  }
}


// Switch interrupt handler vectors
ISR(PORTA_INT0_vect) {_switch_isr();}
ISR(PORTB_INT0_vect) {_switch_isr();}
ISR(PORTC_INT0_vect) {_switch_isr();}
ISR(PORTD_INT0_vect) {_switch_isr();}
ISR(PORTE_INT0_vect) {_switch_isr();}
ISR(PORTF_INT0_vect) {_switch_isr();}


void _switch_enable(switch_t *s, bool enable) {
  if (enable) {
    PORT(s->pin)->INT0MASK |= BM(s->pin);    // Enable INT0

    // Pull up and trigger on both edges
    PINCTRL_PIN(s->pin) = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;

    // Initialize state
    s->state = _read_state(s);

  } else {
    PORT(s->pin)->INT0MASK &= ~BM(s->pin); // Disable INT0
    PINCTRL_PIN(s->pin) = 0;
  }
}


void switch_init() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &sw.switches[i];

    DIRCLR_PIN(s->pin); // Input
    PORT(s->pin)->INTCTRL |= SWITCH_INTLVL; // Set interrupt level

    _switch_enable(s, s->mode != SW_MODE_DISABLED);
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
      PORT(s->pin)->INT0MASK |= BM(s->pin);    // Enable INT0

      // check if the state has changed while we were in lockout
      if (s->state != _read_state(s)) {
        s->debounce = SW_DEGLITCHING;
        s->count = -SW_DEGLITCH_TICKS;
      }

      continue;
    }

    if (!s->count) { // switch triggered
      s->debounce = SW_LOCKOUT;

      if (cm.cycle_state == CYCLE_HOMING || cm.cycle_state == CYCLE_PROBE)
        cm_request_feedhold();

      else if (s->mode & SW_LIMIT_BIT || s->mode & SW_ESTOP_BIT)
        sw.limit_thrown = true; // triggers an emergency shutdown
    }
  }
}


bool switch_get_active(int index) {
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
  switch_t *s = &sw.switches[index];

  if (s->mode != mode) {
    s->mode = mode;
    _switch_enable(s, s->mode != SW_MODE_DISABLED);
  }
}


bool switch_get_limit_thrown() {
  return sw.limit_thrown;
}


// Var callbacks
uint8_t get_switch_type(int index) {
  return sw.switches[index].type;
}


void set_switch_type(int index, uint8_t value) {
  sw.switches[index].type = value;
}
