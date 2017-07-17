/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "switch.h"
#include "config.h"

#include <avr/interrupt.h>

#include <stdbool.h>


typedef struct {
  uint8_t pin;
  switch_type_t type;

  switch_callback_t cb;
  bool state;
  bool triggered;
} switch_t;


// Order must match indices in var functions below
static switch_t switches[SWITCHES] = {
  {.pin = MIN_X_PIN, .type = SW_DISABLED},
  {.pin = MAX_X_PIN, .type = SW_DISABLED},
  {.pin = MIN_Y_PIN, .type = SW_DISABLED},
  {.pin = MAX_Y_PIN, .type = SW_DISABLED},
  {.pin = MIN_Z_PIN, .type = SW_DISABLED},
  {.pin = MAX_Z_PIN, .type = SW_DISABLED},
  {.pin = MIN_A_PIN, .type = SW_DISABLED},
  {.pin = MAX_A_PIN, .type = SW_DISABLED},
  {.pin = ESTOP_PIN, .type = SW_DISABLED},
  {.pin = PROBE_PIN, .type = SW_DISABLED},
};


static bool _read_state(const switch_t *s) {return IN_PIN(s->pin);}


static void _switch_isr() {
  for (int i = 0; i < SWITCHES; i++) {
    switch_t *s = &switches[i];
    if (s->type == SW_DISABLED || s->triggered) continue;
    s->triggered = _read_state(s) != s->state;
    if (s->triggered) PORT(s->pin)->INT0MASK &= ~BM(s->pin); // Disable INT0
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
    s->triggered = false;
    s->state = _read_state(s);                  // Initialize state
    PORT(s->pin)->INT0MASK |= BM(s->pin);       // Enable INT0

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

    if (s->type == SW_DISABLED || !s->triggered) continue;

    bool state = _read_state(s);
    s->triggered = false;
    PORT(s->pin)->INT0MASK |= BM(s->pin); // Reenable INT0

    if (state != s->state) {
      s->state = state;
      if (s->cb) s->cb(i, switch_is_active(i));
    }
  }
}


bool switch_is_active(int index) {
  switch (switches[index].type) {
  case SW_DISABLED: break; // A disabled switch cannot be active
  case SW_NORMALLY_OPEN: return !switches[index].state;
  case SW_NORMALLY_CLOSED: return switches[index].state;
  }
  return false;
}


bool switch_is_enabled(int index) {
  return switch_get_type(index) != SW_DISABLED;
}


switch_type_t switch_get_type(int index) {
  return (index < 0 || SWITCHES <= index) ? SW_DISABLED : switches[index].type;
}


void switch_set_type(int index, switch_type_t type) {
  if (index < 0 || SWITCHES <= index) return;
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
uint8_t get_min_sw_mode(int index) {return switch_get_type(MIN_SWITCH(index));}


void set_min_sw_mode(int index, uint8_t value) {
  switch_set_type(MIN_SWITCH(index), value);
}


uint8_t get_max_sw_mode(int index) {return switch_get_type(MAX_SWITCH(index));}


void set_max_sw_mode(int index, uint8_t value) {
  switch_set_type(MAX_SWITCH(index), value);
}


uint8_t get_estop_mode() {return switch_get_type(SW_ESTOP);}
void set_estop_mode(uint8_t value) {switch_set_type(SW_ESTOP, value);}
uint8_t get_probe_mode() {return switch_get_type(SW_PROBE);}
void set_probe_mode(uint8_t value) {switch_set_type(SW_PROBE, value);}


static uint8_t _get_state(int index) {
  if (!switch_is_enabled(index)) return 2; // Disabled
  return switches[index].state;
}


uint8_t get_min_switch(int index) {return _get_state(MIN_SWITCH(index));}
uint8_t get_max_switch(int index) {return _get_state(MAX_SWITCH(index));}
uint8_t get_estop_switch() {return _get_state(SW_ESTOP);}
uint8_t get_probe_switch() {return _get_state(SW_PROBE);}
