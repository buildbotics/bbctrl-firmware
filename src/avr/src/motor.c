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

#include "motor.h"
#include "config.h"
#include "hardware.h"
#include "cpp_magic.h"
#include "rtc.h"
#include "report.h"
#include "stepper.h"
#include "drv8711.h"
#include "estop.h"
#include "axis.h"
#include "util.h"
#include "pgmspace.h"
#include "exec.h"

#include <util/delay.h>

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  // Config
  uint8_t axis;                  // map motor to axis
  uint8_t step_pin;
  uint8_t dir_pin;
  TC0_t *timer;
  DMA_CH_t *dma;
  uint8_t dma_trigger;

  bool slave;
  uint16_t microsteps;           // microsteps per full step
  bool reverse;
  motor_power_mode_t power_mode;
  float step_angle;              // degrees per whole step
  float travel_rev;              // mm or deg of travel per motor revolution
  float min_soft_limit;
  float max_soft_limit;
  bool homed;

  // Computed
  float steps_per_unit;

  // Runtime state
  uint32_t power_timeout;
  int32_t commanded;
  int32_t encoder;
  int16_t error;
  bool last_negative;

  // Move prep
  bool prepped;
  uint16_t timer_period;
  bool negative;
  int32_t position;
} motor_t;


static motor_t motors[MOTORS] = {
  {
    .axis            = AXIS_X,
    .step_pin        = STEP_X_PIN,
    .dir_pin         = DIR_X_PIN,
    .timer           = &M1_TIMER,
    .dma             = &M1_DMA_CH,
    .dma_trigger     = M1_DMA_TRIGGER,
  }, {
    .axis            = AXIS_Y,
    .step_pin        = STEP_Y_PIN,
    .dir_pin         = DIR_Y_PIN,
    .timer           = &M2_TIMER,
    .dma             = &M2_DMA_CH,
    .dma_trigger     = M2_DMA_TRIGGER,
  }, {
    .axis            = AXIS_Z,
    .step_pin        = STEP_Z_PIN,
    .dir_pin         = DIR_Z_PIN,
    .timer           = &M3_TIMER,
    .dma             = &M3_DMA_CH,
    .dma_trigger     = M3_DMA_TRIGGER,
  }, {
    .axis            = AXIS_A,
    .step_pin        = STEP_A_PIN,
    .dir_pin         = DIR_A_PIN,
    .timer           = (TC0_t *)&M4_TIMER,
    .dma             = &M4_DMA_CH,
    .dma_trigger     = M4_DMA_TRIGGER,
    }
};


static uint8_t _dummy;


static void _update_config(int motor) {
  motor_t *m = &motors[motor];

  m->steps_per_unit = 360.0 * m->microsteps / m->travel_rev / m->step_angle;
}


void motor_init() {
  // Enable DMA
  DMA.CTRL = DMA_RESET_bm;
  DMA.CTRL = DMA_ENABLE_bm;
  DMA.INTFLAGS = 0xff; // clear all pending interrupts

  for (int motor = 0; motor < MOTORS; motor++) {
    motor_t *m = &motors[motor];

    // Default soft limits
    m->min_soft_limit = -INFINITY;
    m->max_soft_limit = INFINITY;

    _update_config(motor);

    // Setup motor timer
    m->timer->CTRLB = TC_WGMODE_SINGLESLOPE_gc | TC1_CCAEN_bm;
    m->timer->CCA = STEP_PULSE_WIDTH;

    // Setup DMA channel as timer event counter
    m->dma->ADDRCTRL = DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTDIR_FIXED_gc;
    m->dma->TRIGSRC = m->dma_trigger;

    // Note, the DMA transfer must read CCA to clear the trigger
    m->dma->SRCADDR0 = (((uintptr_t)&m->timer->CCA) >> 0) & 0xff;
    m->dma->SRCADDR1 = (((uintptr_t)&m->timer->CCA) >> 8) & 0xff;
    m->dma->SRCADDR2 = 0;

    m->dma->DESTADDR0 = (((uintptr_t)&_dummy) >> 0) & 0xff;
    m->dma->DESTADDR1 = (((uintptr_t)&_dummy) >> 8) & 0xff;
    m->dma->DESTADDR2 = 0;

    m->dma->CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;

    // IO pins
    PINCTRL_PIN(m->step_pin) = PORT_INVEN_bm; // Inverted output
    DIRSET_PIN(m->step_pin); // Output
    DIRSET_PIN(m->dir_pin);  // Output
  }

  axis_map_motors();
}


bool motor_is_enabled(int motor) {
  return motors[motor].power_mode != MOTOR_DISABLED;
}


int motor_get_axis(int motor) {return motors[motor].axis;}


static int32_t _position_to_steps(int motor, float position) {
  return (int32_t)round(position * motors[motor].steps_per_unit);
}


void motor_set_position(int motor, float position) {
  motor_t *m = &motors[motor];
  m->commanded = m->encoder = m->position = _position_to_steps(motor, position);
  m->error = 0;
}


float motor_get_soft_limit(int motor, bool min) {
  return min ? motors[motor].min_soft_limit : motors[motor].max_soft_limit;
}


bool motor_get_homed(int motor) {return motors[motor].homed;}


static void _update_power(int motor) {
  motor_t *m = &motors[motor];

  switch (m->power_mode) {
  case MOTOR_POWERED_ONLY_WHEN_MOVING:
  case MOTOR_POWERED_IN_CYCLE:
    if (rtc_expired(m->power_timeout)) {
      drv8711_set_state(motor, DRV8711_IDLE);
      break;
    }
    // Fall through

  case MOTOR_ALWAYS_POWERED:
    // NOTE, we have ~5ms to enable the motor
    drv8711_set_state(motor, DRV8711_ACTIVE);
    break;

  default: // Disabled
    drv8711_set_state(motor, DRV8711_DISABLED);
  }
}


/// Callback to manage motor power sequencing and power-down timing.
stat_t motor_rtc_callback() { // called by controller
  for (int motor = 0; motor < MOTORS; motor++)
    _update_power(motor);

  return STAT_OK;
}


void motor_end_move(int motor) {
  motor_t *m = &motors[motor];

  if (!m->timer->CTRLA) return;

  // Stop clock
  m->timer->CTRLA = 0;

  // Wait for pending DMA transfers
  while (m->dma->CTRLB & DMA_CH_CHPEND_bm) continue;

  // Get actual step count from DMA channel
  const int32_t steps = 0xffff - m->dma->TRFCNT;

  // Accumulate encoder & compute error
  m->encoder += m->last_negative ? -steps : steps;
  m->error = m->commanded - m->encoder;
}


void motor_load_move(int motor) {
  motor_t *m = &motors[motor];

  // Clear move
  ASSERT(m->prepped);
  m->prepped = false;

  motor_end_move(motor);

  if (!m->timer_period) return; // Leave clock stopped

  // Set direction, compensating for polarity but only when moving
  const bool dir = m->negative ^ m->reverse;
  if (dir != IN_PIN(m->dir_pin)) {
    SET_PIN(m->dir_pin, dir);
    // Need at least 200ns between direction change and next step.
    if (m->timer->CCA < m->timer->CNT) m->timer->CNT = m->timer->CCA + 1;
  }

  // Reset DMA step counter
  m->dma->CTRLA &= ~DMA_CH_ENABLE_bm;
  m->dma->TRFCNT = 0xffff;
  m->dma->CTRLA |= DMA_CH_ENABLE_bm;

  // Note, it is important to start the clock, if it is stopped, before
  // setting PERBUF so that PER is not updated immediately possibly
  // interrupting the clock mid step or causing counter wrap around.

  // Set clock and period
  m->timer->CTRLA = TC_CLKSEL_DIV2_gc; // Start clock
  m->timer->PERBUF = m->timer_period;  // Set next frequency
  m->last_negative = m->negative;
  m->commanded = m->position;
}


void motor_prep_move(int motor, float target) {
  // Validate input
  ASSERT(0 <= motor && motor < MOTORS);
  ASSERT(isfinite(target));

  motor_t *m = &motors[motor];
  ASSERT(!m->prepped);

  // Travel in steps
  int32_t position = _position_to_steps(motor, target);
  int24_t steps = position - m->position;
  m->position = position;

  // Error correction
  int16_t correction = abs(m->error);
  if (MIN_STEP_CORRECTION <= correction) {
    // Dampen correction oscillation
    correction >>= 1;

    // Make correction
    steps += m->error < 0 ? -correction : correction;
  }

  // Positive steps from here on
  m->negative = steps < 0;
  if (m->negative) steps = -steps;

  // We use clock / 2
  float seg_clocks = SEGMENT_TIME * (F_CPU * 60 / 2);
  float ticks_per_step = round(seg_clocks / steps);

  // Limit clock if step rate is too fast, disable if too slow
  if (ticks_per_step < STEP_PULSE_WIDTH * 2)
    ticks_per_step = STEP_PULSE_WIDTH * 2;           // Too fast
  if (0xffff <= ticks_per_step) m->timer_period = 0; // Too slow
  else m->timer_period = ticks_per_step;             // Just right

  if (!steps) m->timer_period = 0;

  // Power motor
  switch (m->power_mode) {
  case MOTOR_POWERED_ONLY_WHEN_MOVING:
    if (!m->timer_period) break; // Not moving
    // Fall through

  case MOTOR_ALWAYS_POWERED: case MOTOR_POWERED_IN_CYCLE:
    // Reset timeout
    m->power_timeout = rtc_get_time() + MOTOR_IDLE_TIMEOUT * 1000;
    break;

  default: // Disabled
    m->timer_period = 0;
    m->encoder = m->commanded = m->position;
    m->error = 0;
    break;
  }
  _update_power(motor);

  // Queue move
  m->prepped = true;
}


// Var callbacks
uint8_t get_power_mode(int motor) {return motors[motor].power_mode;}


void set_power_mode(int motor, uint8_t value) {
  if (motors[motor].slave) return;

  for (int m = motor; m < MOTORS; m++)
    if (motors[m].axis == motors[motor].axis)
      motors[m].power_mode =
        value <= MOTOR_POWERED_ONLY_WHEN_MOVING ?
        (motor_power_mode_t)value : MOTOR_DISABLED;
}


float get_step_angle(int motor) {return motors[motor].step_angle;}


void set_step_angle(int motor, float value) {
  if (motors[motor].slave) return;

  for (int m = motor; m < MOTORS; m++)
    if (motors[m].axis == motors[motor].axis) {
      motors[m].step_angle = value;
      _update_config(m);
    }
}


float get_travel(int motor) {return motors[motor].travel_rev;}


void set_travel(int motor, float value) {
  if (motors[motor].slave) return;

  for (int m = motor; m < MOTORS; m++)
    if (motors[m].axis == motors[motor].axis) {
      motors[m].travel_rev = value;
      _update_config(m);
    }
}


uint16_t get_microstep(int motor) {return motors[motor].microsteps;}


void set_microstep(int motor, uint16_t value) {
  switch (value) {
  case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
    break;
  default: return;
  }

  if (motors[motor].slave) return;

  for (int m = motor; m < MOTORS; m++)
    if (motors[m].axis == motors[motor].axis) {
      motors[m].microsteps = value;
      _update_config(m);
      drv8711_set_microsteps(m, value);
    }
}


char get_motor_axis(int motor) {return motors[motor].axis;}


void set_motor_axis(int motor, uint8_t axis) {
  if (MOTORS <= motor || AXES <= axis || axis == motors[motor].axis) return;
  motors[motor].axis = axis;
  axis_map_motors();

  // Reset encoder
  motor_set_position(motor, exec_get_axis_position(axis));

  // Check if this is now a slave motor
  motors[motor].slave = false;
  for (int m = 0; m < motor; m++)
    if (motors[m].axis == motors[motor].axis) {
      // Sync with master
      set_step_angle(motor, motors[m].step_angle);
      set_travel(motor, motors[m].travel_rev);
      set_microstep(motor, motors[m].microsteps);
      set_power_mode(motor, motors[m].power_mode);
      motors[motor].slave = true;
      break;
    }
}


bool get_reverse(int motor) {return motors[motor].reverse;}
void set_reverse(int motor, bool value) {motors[motor].reverse = value;}


float get_min_soft_limit(int motor) {return motors[motor].min_soft_limit;}
float get_max_soft_limit(int motor) {return motors[motor].max_soft_limit;}


void set_min_soft_limit(int motor, float limit) {
  motors[motor].min_soft_limit = limit;
}


void set_max_soft_limit(int motor, float limit) {
  motors[motor].max_soft_limit = limit;
}


bool get_homed(int motor) {return motors[motor].homed;}
void set_homed(int motor, bool homed) {motors[motor].homed = homed;}


int32_t get_encoder(int m) {return motors[m].encoder;}
int32_t get_error(int m) {return motors[m].error;}
