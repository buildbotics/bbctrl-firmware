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

/* Normally open switches (NO) trigger an interrupt on the
 * falling edge and lockout subsequent interrupts for the defined
 * lockout period. This approach beats doing debouncing as an
 * integration as switches fire immediately.
 *
 * Normally closed switches (NC) trigger an interrupt on the
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
#include "config.h"

#include <avr/interrupt.h>

#include <stdbool.h>


typedef enum {
  SW_IDLE,
  SW_DEGLITCHING,
  SW_LOCKOUT
} switch_debounce_t;


typedef struct {
  uint8_t pin;
  switch_type_t type;

  switch_callback_t cb;
  bool state;
  switch_debounce_t debounce;
  int16_t count;
} switch_t;



static switch_t switches[SWITCHES] = {
  {.pin = MIN_X_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MAX_X_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MIN_Y_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MAX_X_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MIN_Z_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MAX_Z_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MIN_A_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = MAX_A_PIN, .type = SW_NORMALLY_OPEN},
  {.pin = ESTOP_PIN, .type = SW_NORMALLY_OPEN},
  //  {.pin = PROBE_PIN, .type = SW_NORMALLY_OPEN},
};


static bool _read_state(const switch_t *s) {
  // A normally open switch drives the pin low when thrown
  return (s->type == SW_NORMALLY_OPEN) ^ IN_PIN(s->pin);
}


static void _switch_isr() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &switches[i];
    bool state = _read_state(s);

    if (state == s->state || s->type == SW_DISABLED) continue;

    s->debounce = SW_DEGLITCHING;
    s->count = -SW_DEGLITCH_TICKS; // reset deglitch count
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
    PORT(s->pin)->INT0MASK |= BM(s->pin);       // Enable INT0
    s->state = _read_state(s);                  // Initialize state
    s->debounce = SW_IDLE;

  } else PORT(s->pin)->INT0MASK &= ~BM(s->pin); // Disable INT0
}


void switch_init() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &switches[i];

    // Pull up and trigger on both edges
    PINCTRL_PIN(s->pin) = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;
    DIRCLR_PIN(s->pin); // Input
    PORT(s->pin)->INTCTRL |= SWITCH_INTLVL; // Set interrupt level

    _switch_enable(s, s->type != SW_DISABLED);
  }
}


/// Called from RTC on each tick
void switch_rtc_callback() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &switches[i];

    if (s->type == SW_DISABLED || s->debounce == SW_IDLE) continue;

    // state is either lockout or deglitching
    if (++s->count == SW_LOCKOUT_TICKS) {
      PORT(s->pin)->INT0MASK |= BM(s->pin); // Renable INT0
      bool state = _read_state(s);
      s->debounce = SW_IDLE;

      // check if the state has changed while we were in lockout
      if (s->state != state) {
        s->state = state;
        s->debounce = SW_DEGLITCHING;
        s->count = -SW_DEGLITCH_TICKS;
      }

      continue;
    }

    if (!s->count) { // switch triggered
      s->debounce = SW_LOCKOUT;
      if (s->cb) s->cb(i, s->state);
    }
  }
}


bool switch_is_active(int index) {
  return switches[index].state;
}


bool switch_is_enabled(int index) {
  return switch_get_type(index) != SW_DISABLED;
}


switch_type_t switch_get_type(int index) {
  return switches[index].type;
}


void switch_set_type(int index, switch_type_t type) {
  switch_t *s = &switches[index];

  if (s->type != type) {
    s->type = type;
    _switch_enable(s, type != SW_DISABLED);
  }
}


void switch_set_callback(int index, switch_callback_t cb) {
  switches[index].cb = cb;
}


// Var callbacks
uint8_t get_switch_type(int index) {return switch_get_type(index);}
void set_switch_type(int index, uint8_t value) {switch_set_type(index, value);}
