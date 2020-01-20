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
  bool enabled;
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
  uint8_t clock;
  uint16_t timer_period;
  bool negative;
  int32_t position;
} motor_t;


static motor_t motors[MOTORS] = {
  {
    .axis            = AXIS_X,
    .step_pin        = STEP_0_PIN,
    .dir_pin         = DIR_0_PIN,
    .timer           = &TCD0,
    .dma             = &DMA.CH0,
    .dma_trigger     = DMA_CH_TRIGSRC_TCD0_CCA_gc,
  }, {
    .axis            = AXIS_Y,
    .step_pin        = STEP_1_PIN,
    .dir_pin         = DIR_1_PIN,
    .timer           = &TCE0,
    .dma             = &DMA.CH1,
    .dma_trigger     = DMA_CH_TRIGSRC_TCE0_CCA_gc,
  }, {
    .axis            = AXIS_Z,
    .step_pin        = STEP_2_PIN,
    .dir_pin         = DIR_2_PIN,
    .timer           = &TCF0,
    .dma             = &DMA.CH2,
    .dma_trigger     = DMA_CH_TRIGSRC_TCF0_CCA_gc,
  }, {
    .axis            = AXIS_A,
    .step_pin        = STEP_3_PIN,
    .dir_pin         = DIR_3_PIN,
    .timer           = (TC0_t *)&TCE1,
    .dma             = &DMA.CH3,
    .dma_trigger     = DMA_CH_TRIGSRC_TCE1_CCA_gc,
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


bool motor_is_enabled(int motor) {return motors[motor].enabled;}
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

  if (m->enabled) {
    bool timedout = rtc_expired(m->power_timeout);
    // NOTE, we have ~5ms to update the driver config
    drv8711_set_state(motor, timedout ? DRV8711_IDLE : DRV8711_ACTIVE);

  } else drv8711_set_state(motor, DRV8711_DISABLED);
}


/// Callback to manage motor power sequencing and power-down timing.
stat_t motor_rtc_callback() { // called by controller
  for (int motor = 0; motor < MOTORS; motor++)
    _update_power(motor);

  return STAT_OK;
}


void motor_emulate_steps(int motor) {
  motor_t *m = &motors[motor];
  m->dma->TRFCNT = 0xffff - abs(m->commanded - m->encoder);
}


void motor_end_move(int motor) {
  motor_t &m = motors[motor];

  if (!m.timer->CTRLA) return;

  // Stop clock
  m.timer->CTRLA = 0;

  // Wait for pending DMA transfers
  while (m.dma->CTRLB & DMA_CH_CHPEND_bm) continue;

  // Get actual step count from DMA channel
  const int32_t steps = 0xffff - m.dma->TRFCNT;

  // Accumulate encoder & compute error
  m.encoder += m.last_negative ? -steps : steps;
  m.error = m.commanded - m.encoder;
}


void motor_load_move(int motor) {
  motor_t &m = motors[motor];

  // Clear move
  ESTOP_ASSERT(m.prepped, STAT_MOTOR_NOT_PREPPED);
  m.prepped = false;

  motor_end_move(motor);

  if (!m.timer_period) return; // Leave clock stopped

  // Set direction, compensating for polarity but only when moving
  const bool dir = m.negative ^ m.reverse;
  if (dir != IN_PIN(m.dir_pin)) {
    SET_PIN(m.dir_pin, dir);

    // We need at least 200ns between direction change and next step.
    if (m.timer->CCA < m.timer->CNT) m.timer->CNT = m.timer->CCA + 1;
  }

  // Reset DMA step counter
  m.dma->CTRLA &= ~DMA_CH_ENABLE_bm;
  m.dma->TRFCNT = 0xffff;
  m.dma->CTRLA |= DMA_CH_ENABLE_bm;

  // To avoid causing couter wrap around, it is important to start the clock
  // before setting PERBUF.  If PERBUF is set before the clock is started PER
  // updates immediately and possibly mid step.

  // Set clock and period
  m.timer->CTRLA  = m.clock;         // Start clock
  m.timer->PERBUF = m.timer_period;  // Set next frequency
  m.last_negative = m.negative;
  m.commanded     = m.position;
}


void motor_prep_move(int motor, float target) {
  // Validate input
  ESTOP_ASSERT(0 <= motor && motor < MOTORS, STAT_MOTOR_ID_INVALID);
  ESTOP_ASSERT(isfinite(target), STAT_BAD_FLOAT);

  motor_t &m = motors[motor];
  ESTOP_ASSERT(!m.prepped, STAT_MOTOR_NOT_READY);

  // Travel in steps
  int32_t position = _position_to_steps(motor, target);
  int24_t steps = position - m.position;
  m.position = position;

  // Error correction
  int16_t correction = abs(m.error);
  if (MIN_STEP_CORRECTION <= correction) {
    // Dampen correction oscillation
    correction >>= 1;

    // Make correction
    steps += m.error < 0 ? -correction : correction;
  }

  // Positive steps from here on
  m.negative = steps < 0;
  if (m.negative) steps = -steps;

  // Start with clock / 2
  const float seg_clocks = SEGMENT_TIME * (F_CPU * 60 / 2);
  float ticks_per_step = seg_clocks / steps;

  // Use faster clock with faster step rates for increased resolution.
  if (ticks_per_step < 0x7fff) {
    ticks_per_step *= 2;
    m.clock = TC_CLKSEL_DIV1_gc;

    // Limit clock if step rate is too fast
    // We allow a slight fudge here (i.e. 1.9 instead 2) because the motor
    // driver is able to handle it and otherwise we could not actually hit
    // an average rate of 250k usteps/sec.
    if (ticks_per_step < STEP_PULSE_WIDTH * 1.9)
      ticks_per_step = STEP_PULSE_WIDTH * 1.9; // Too fast

  } else m.clock = TC_CLKSEL_DIV2_gc; // NOTE, pulse width will be twice as long

  // Disable clock if too slow
  if (0xffff <= ticks_per_step) ticks_per_step = 0;

  m.timer_period = steps ? round(ticks_per_step) : 0;

  // Power motor
  if (!m.enabled) {
    m.timer_period = 0;
    m.encoder = m.commanded = m.position;
    m.error = 0;

  } else if (m.timer_period) // Motor is moving so reset power timeout
    m.power_timeout = rtc_get_time() + MOTOR_IDLE_TIMEOUT * 1000;
  _update_power(motor);

  // Queue move
  m.prepped = true;
}


// Var callbacks
bool get_motor_enabled(int motor) {return motors[motor].enabled;}


void set_motor_enabled(int motor, bool enabled) {
  if (motors[motor].slave) return;

  for (int m = motor; m < MOTORS; m++)
    if (motors[m].axis == motors[motor].axis)
      motors[m].enabled = enabled;
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
      set_motor_enabled(motor, motors[m].enabled);
      motors[motor].slave = true; // Must be last
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
