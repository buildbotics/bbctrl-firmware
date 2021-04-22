/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#include "switch.h"
#include "config.h"

#include <stdbool.h>
#include <stdio.h>


static struct {
  uint16_t debounce;
  uint16_t lockout;
} sw = {
  .debounce = SWITCH_DEBOUNCE,
  .lockout = SWITCH_LOCKOUT,
};

typedef struct {
  uint8_t pin;
  switch_type_t type;

  switch_callback_t cb;
  bool state;
  uint16_t debounce;
  uint16_t lockout;
  bool initialized;
} switch_t;


// Order must match indices in var functions below
static switch_t switches[] = {
  {.pin = ESTOP_PIN,       .type = SW_DISABLED},
  {.pin = PROBE_PIN,       .type = SW_DISABLED},
  {.pin = MIN_0_PIN,       .type = SW_DISABLED},
  {.pin = MAX_0_PIN,       .type = SW_DISABLED},
  {.pin = MIN_1_PIN,       .type = SW_DISABLED},
  {.pin = MAX_1_PIN,       .type = SW_DISABLED},
  {.pin = MIN_2_PIN,       .type = SW_DISABLED},
  {.pin = MAX_2_PIN,       .type = SW_DISABLED},
  {.pin = MIN_3_PIN,       .type = SW_DISABLED},
  {.pin = MAX_3_PIN,       .type = SW_DISABLED},
  {.pin = STALL_0_PIN,     .type = SW_DISABLED},
  {.pin = STALL_1_PIN,     .type = SW_DISABLED},
  {.pin = STALL_2_PIN,     .type = SW_DISABLED},
  {.pin = STALL_3_PIN,     .type = SW_DISABLED},
  {.pin = MOTOR_FAULT_PIN, .type = SW_DISABLED},
};


static const int num_switches = sizeof(switches) / sizeof (switch_t);


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
    if (s->lockout && --s->lockout) continue;

    // Debounce switch
    bool state = IN_PIN(s->pin);
    if (state == s->state && s->initialized) s->debounce = 0;
    else if (++s->debounce == sw.debounce) {
      s->state = state;
      s->debounce = 0;
      s->initialized = true;
      s->lockout = sw.lockout;
      if (s->cb) s->cb((switch_id_t)i, switch_is_active((switch_id_t)i));
    }
  }
}


bool switch_is_active(switch_id_t sw) {
  if (sw < 0 || num_switches <= sw || !switches[sw].initialized) return false;

  // NOTE, switch inputs are active lo
  switch (switches[sw].type) {
  case SW_DISABLED:        break; // A disabled switch cannot be active
  case SW_NORMALLY_OPEN:   return !switches[sw].state;
  case SW_NORMALLY_CLOSED: return switches[sw].state;
  }

  return false;
}


bool switch_is_enabled(switch_id_t sw) {
  return switch_get_type(sw) != SW_DISABLED;
}


switch_type_t switch_get_type(switch_id_t sw) {
  return (sw < 0 || num_switches <= sw) ? SW_DISABLED : switches[sw].type;
}


void switch_set_type(switch_id_t sw, switch_type_t type) {
  if (sw < 0 || num_switches <= sw) return;
  switch_t *s = &switches[sw];

  if (s->type != type) {
    bool wasActive = switch_is_active(sw);
    s->type = type;
    bool isActive = switch_is_active(sw);
    if (wasActive != isActive && s->cb) s->cb(sw, isActive);
  }
}


void switch_set_callback(switch_id_t sw, switch_callback_t cb) {
  if (sw < 0 || num_switches <= sw) return;
  switches[sw].cb = cb;
}


switch_callback_t switch_get_callback(switch_id_t sw) {
  if (sw < 0 || num_switches <= sw) return 0;
  return switches[sw].cb;
}


// Var callbacks
uint8_t get_min_sw_mode(int index) {return switch_get_type(MIN_SWITCH(index));}


void set_min_sw_mode(int index, uint8_t value) {
  switch_set_type(MIN_SWITCH(index), (switch_type_t)value);
}


uint8_t get_max_sw_mode(int index) {return switch_get_type(MAX_SWITCH(index));}


void set_max_sw_mode(int index, uint8_t value) {
  switch_set_type(MAX_SWITCH(index), (switch_type_t)value);
}


uint8_t get_estop_mode() {return switch_get_type(SW_ESTOP);}


void set_estop_mode(uint8_t value) {
  switch_set_type(SW_ESTOP, (switch_type_t)value);
}


uint8_t get_probe_mode() {return switch_get_type(SW_PROBE);}


void set_probe_mode(uint8_t value) {
  switch_set_type(SW_PROBE, (switch_type_t)value);
}


static uint8_t _get_state(int index) {
  if (!switch_is_enabled((switch_id_t)index)) return 2; // Disabled
  return switches[index].state;
}


uint8_t get_min_switch(int index) {return _get_state(MIN_SWITCH(index));}
uint8_t get_max_switch(int index) {return _get_state(MAX_SWITCH(index));}
uint8_t get_estop_switch() {return _get_state(SW_ESTOP);}
uint8_t get_probe_switch() {return _get_state(SW_PROBE);}


void set_switch_debounce(uint16_t debounce) {
  sw.debounce = SWITCH_MAX_DEBOUNCE < debounce ? SWITCH_DEBOUNCE : debounce;
}


uint16_t get_switch_debounce() {return sw.debounce;}


void set_switch_lockout(uint16_t lockout) {
  sw.lockout = SWITCH_MAX_LOCKOUT < lockout ? SWITCH_LOCKOUT : lockout;
}


uint16_t get_switch_lockout() {return sw.lockout;}
