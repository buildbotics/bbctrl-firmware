/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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
  int8_t debounce;
} switch_t;


// Order must match indices in var functions below
static switch_t switches[] = {
  {.pin = ESTOP_PIN,       .type = SW_DISABLED},
  {.pin = PROBE_PIN,       .type = SW_DISABLED},
  {.pin = MIN_X_PIN,       .type = SW_DISABLED},
  {.pin = MAX_X_PIN,       .type = SW_DISABLED},
  {.pin = MIN_Y_PIN,       .type = SW_DISABLED},
  {.pin = MAX_Y_PIN,       .type = SW_DISABLED},
  {.pin = MIN_Z_PIN,       .type = SW_DISABLED},
  {.pin = MAX_Z_PIN,       .type = SW_DISABLED},
  {.pin = MIN_A_PIN,       .type = SW_DISABLED},
  {.pin = MAX_A_PIN,       .type = SW_DISABLED},
  {.pin = STALL_X_PIN,     .type = SW_DISABLED},
  {.pin = STALL_Y_PIN,     .type = SW_DISABLED},
  {.pin = STALL_Z_PIN,     .type = SW_DISABLED},
  {.pin = STALL_A_PIN,     .type = SW_DISABLED},
  {.pin = MOTOR_FAULT_PIN, .type = SW_DISABLED},
};


const int num_switches = sizeof(switches) / sizeof (switch_t);


void switch_init() {
  for (int i = 0; i < num_switches; i++) {
    switch_t *s = &switches[i];
    PINCTRL_PIN(s->pin) = PORT_OPC_PULLUP_gc; // Pull up
    DIRCLR_PIN(s->pin); // Input
  }
}


/// Called from RTC on each tick
void switch_rtc_callback() {
  for (int i = 0; i < num_switches; i++) {
    switch_t *s = &switches[i];

    if (s->type == SW_DISABLED) continue;

    // Debounce switch
    bool state = IN_PIN(s->pin);
    if (state == s->state) s->debounce = 0;
    else if (++s->debounce == SWITCH_DEBOUNCE) {
      s->state = state;
      s->debounce = 0;
      if (s->cb) s->cb(i, switch_is_active(i));
    }
  }
}


bool switch_is_active(int index) {
  // NOTE, switch inputs are active lo
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
  return
    (index < 0 || num_switches <= index) ? SW_DISABLED : switches[index].type;
}


void switch_set_type(int index, switch_type_t type) {
  if (index < 0 || num_switches <= index) return;
  switch_t *s = &switches[index];

  if (s->type != type) {
    bool wasActive = switch_is_active(index);
    s->type = type;
    bool isActive = switch_is_active(index);
    if (wasActive != isActive && s->cb) s->cb(index, isActive);
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
