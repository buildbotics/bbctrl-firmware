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

#include "drv8711.h"
#include "status.h"
#include "stepper.h"
#include "switch.h"
#include "estop.h"

#include <avr/interrupt.h>
#include <util/delay.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


#define DRIVERS MOTORS


#define DRV8711_WORD_BYTE_PTR(WORD, LOW) (((uint8_t *)&(WORD)) + !(LOW))


typedef enum {
  SS_WRITE_OFF,
  SS_WRITE_BLANK,
  SS_WRITE_DECAY,
  SS_WRITE_STALL,
  SS_WRITE_DRIVE,
  SS_WRITE_TORQUE,
  SS_WRITE_CTRL,
  SS_READ_OFF,
  SS_READ_STATUS,
  SS_CLEAR_STATUS,
} spi_state_t;


bool motor_fault = false;


typedef struct {
  float current;
  uint8_t torque;
} current_t;


typedef struct {
  uint8_t cs_pin;
  switch_id_t stall_sw;

  uint8_t status;
  uint16_t flags;
  bool reset_flags;
  bool stalled;

  drv8711_state_t state;
  current_t drive;
  current_t idle;
  float stall_threspause;

  uint8_t microstep;
  stall_callback_t stall_cb;

  uint8_t last_torque;
  uint8_t last_microstep;

  spi_state_t spi_state;
} drv8711_driver_t;


static drv8711_driver_t drivers[DRIVERS] = {
  {.cs_pin = SPI_CS_0_PIN, .stall_sw = SW_STALL_0},
  {.cs_pin = SPI_CS_1_PIN, .stall_sw = SW_STALL_1},
  {.cs_pin = SPI_CS_2_PIN, .stall_sw = SW_STALL_2},
  {.cs_pin = SPI_CS_3_PIN, .stall_sw = SW_STALL_3},
};


typedef struct {
  bool advance;
  uint8_t disable_cs_pin;

  uint16_t command;
  uint16_t response;
  uint8_t driver;
  bool low_byte;
} spi_t;

static spi_t spi = {0};


static void _current_set(current_t *c, float current) {
  const float gain = DRV8711_CTRL_GAIN(DRV8711_CTRL);
  const float max_current = CURRENT_SENSE_REF / (CURRENT_SENSE_RESISTOR * gain);

  // Limit to max configurable current (11A)
  if (max_current < current) current = max_current;

  c->current = current;
  c->torque = round(current * CURRENT_SENSE_RESISTOR / CURRENT_SENSE_REF *
                    gain * 255);
}


static bool _driver_fault(drv8711_driver_t *drv) {return drv->flags & 0x1f;}


static bool _driver_active(drv8711_driver_t *drv) {
  return drv->state == DRV8711_ACTIVE;
}


static float _driver_get_current(drv8711_driver_t *drv) {
  if (_driver_fault(drv)) return 0;

  switch (drv->state) {
  case DRV8711_IDLE: return drv->idle.current;
  case DRV8711_ACTIVE: return drv->drive.current;
  default: return 0; // Off
  }
}


static uint8_t _driver_get_torque(drv8711_driver_t *drv) {
  if (estop_triggered()) return 0;

  switch (drv->state) {
  case DRV8711_IDLE:   return drv->idle.torque;
  case DRV8711_ACTIVE: return drv->drive.torque;
  default: return 0; // Off
  }
}


static uint16_t _driver_spi_command(drv8711_driver_t *drv) {
  switch (drv->spi_state) {
  case SS_WRITE_OFF:   return DRV8711_WRITE(DRV8711_OFF_REG,   DRV8711_OFF);
  case SS_WRITE_BLANK: return DRV8711_WRITE(DRV8711_BLANK_REG, DRV8711_BLANK);
  case SS_WRITE_DECAY: return DRV8711_WRITE(DRV8711_DECAY_REG, DRV8711_DECAY);
  case SS_WRITE_STALL: return DRV8711_WRITE(DRV8711_STALL_REG, DRV8711_STALL);
  case SS_WRITE_DRIVE: return DRV8711_WRITE(DRV8711_DRIVE_REG, DRV8711_DRIVE);

  case SS_WRITE_TORQUE:
    drv->last_torque = _driver_get_torque(drv);
    return DRV8711_WRITE(DRV8711_TORQUE_REG, DRV8711_TORQUE | drv->last_torque);

  case SS_WRITE_CTRL: {
    // NOTE, we disable the driver if it's not active.  The chip gets hot
    // idling with the driver enabled.
    bool enable = _driver_get_torque(drv);
    drv->last_microstep = drv->microstep;
    return DRV8711_WRITE(DRV8711_CTRL_REG, DRV8711_CTRL |
                         (drv->microstep << 3) |
                         (enable ? DRV8711_CTRL_ENBL_bm : 0));
  }

  case SS_READ_OFF:    return DRV8711_READ(DRV8711_OFF_REG);
  case SS_READ_STATUS: return DRV8711_READ(DRV8711_STATUS_REG);

  case SS_CLEAR_STATUS:
    drv->reset_flags = false;
    drv->flags = 0;
    return DRV8711_WRITE(DRV8711_STATUS_REG, 0x0fff & ~drv->status);
  }

  return 0; // Should not get here
}


static spi_state_t _driver_spi_next(drv8711_driver_t *drv) {
  // Process response
  switch (drv->spi_state) {
  case SS_READ_OFF:
    // We read back the OFF register to test for communication failure.
    if ((spi.response & 0x1ff) != DRV8711_OFF)
      drv->flags |= DRV8711_COMM_ERROR_bm;
    else drv->flags &= ~DRV8711_COMM_ERROR_bm;
    break;

  case SS_READ_STATUS: {
    drv->status = spi.response;

    // NOTE If there is a power fault and the drivers are not powered
    // then the status flags will read 0xff but the motor fault line will
    // not be asserted.  So, fault flags are not valid with out motor fault.
    // Also, a real stall cannot occur if the driver is inactive.
    bool active = _driver_active(drv);
    uint8_t mask =
      ((motor_fault && !drv->reset_flags) ? 0xff : 0) | (active ? 0xc0 : 0);
    drv->flags = (drv->flags & 0xff00) | (mask & drv->status);

    // EStop on fatal driver faults
    if (_driver_fault(drv)) estop_trigger(STAT_MOTOR_FAULT);
    break;
  }

  default: break;
  }

  switch (drv->spi_state) {
  case SS_READ_OFF:
    if (drv->flags & DRV8711_COMM_ERROR_bm) return SS_WRITE_OFF; // Retry
    break;

  case SS_READ_STATUS:
    if (drv->reset_flags) return SS_CLEAR_STATUS;
    if (drv->last_torque != _driver_get_torque(drv)) return SS_WRITE_TORQUE;
    if (drv->last_microstep != drv->microstep) return SS_WRITE_CTRL;
    // Fall through

  case SS_CLEAR_STATUS: return SS_READ_OFF;

  default: break;
  }

  return (spi_state_t)(drv->spi_state + 1); // Next
}


static void _spi_send() {
  drv8711_driver_t *drv = &drivers[spi.driver];

  // Flush any status errors (TODO check SPI errors)
  uint8_t x = SPIC.STATUS;
  x = x;

  // Read byte
  *DRV8711_WORD_BYTE_PTR(spi.response, !spi.low_byte) = SPIC.DATA;

  // Advance state and process response
  if (spi.advance) {
    spi.advance = false;

    // Handle response and set next state
    drv->spi_state = _driver_spi_next(drv);

    // Next driver
    if (++spi.driver == DRIVERS) spi.driver = 0; // Wrap around
    drv = &drivers[spi.driver];
  }

  // Disable CS
  if (spi.disable_cs_pin) {
    OUTCLR_PIN(spi.disable_cs_pin); // Set low (inactive)
    _delay_us(1);
    spi.disable_cs_pin = 0;
  }

  if (spi.low_byte) {
    spi.disable_cs_pin = drv->cs_pin; // Schedule next CS disable
    spi.advance = true; // Word complete

  } else {
    // Enable CS
    OUTSET_PIN(drv->cs_pin); // Set high (active)
    _delay_us(1);

    // Get next command
    spi.command = _driver_spi_command(drv);
  }

  // Write byte and prep next read
  SPIC.DATA = *DRV8711_WORD_BYTE_PTR(spi.command, spi.low_byte);

  // Next byte
  spi.low_byte = !spi.low_byte;
}


ISR(SPIC_INT_vect) {_spi_send();}


static void _stall_change(int driver, bool stalled) {
  drivers[driver].stalled = stalled;

  // Call stall callback
  if (stalled && drivers[driver].stall_cb)
    drivers[driver].stall_cb(driver);
}


static void _stall_switch_cb(switch_id_t sw, bool active) {
  switch (sw) {
  case SW_STALL_0: _stall_change(0, active); break;
  case SW_STALL_1: _stall_change(1, active); break;
  case SW_STALL_2: _stall_change(2, active); break;
  case SW_STALL_3: _stall_change(3, active); break;
  default: break;
  }
}


static void _motor_fault_switch_cb(switch_id_t sw, bool active) {
  motor_fault = active;
}


void drv8711_init() {
  // Setup pins
  // Must set the SS pin either in/high or out/any for master mode to work
  // Note, this pin is also used by the USART as the CTS line
  DIRSET_PIN(SPI_SS_PIN);   // Output
  OUTSET_PIN(SPI_CLK_PIN);  // High
  DIRSET_PIN(SPI_CLK_PIN);  // Output
  DIRCLR_PIN(SPI_MISO_PIN); // Input
  OUTSET_PIN(SPI_MOSI_PIN); // High
  DIRSET_PIN(SPI_MOSI_PIN); // Output

  // Motor driver enable
  OUTSET_PIN(MOTOR_ENABLE_PIN); // Active high
  DIRSET_PIN(MOTOR_ENABLE_PIN); // Output

  for (int i = 0; i < DRIVERS; i++) {
    uint8_t cs_pin = drivers[i].cs_pin;
    OUTSET_PIN(cs_pin);     // High
    DIRSET_PIN(cs_pin);     // Output

    switch_id_t stall_sw = drivers[i].stall_sw;
    switch_set_type(stall_sw, SW_NORMALLY_OPEN);
    switch_set_callback(stall_sw, _stall_switch_cb);

    drivers[i].reset_flags = true; // Reset flags once on startup
  }

  switch_set_type(SW_MOTOR_FAULT, SW_NORMALLY_OPEN);
  switch_set_callback(SW_MOTOR_FAULT, _motor_fault_switch_cb);

  // Configure SPI
  PR.PRPC &= ~PR_SPI_bm; // Disable power reduction
  SPIC.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_0_gc |
    SPI_PRESCALER_DIV16_gc; // enable, big endian, master, mode, clock div
  PIN_PORT(SPI_CLK_PIN)->REMAP = PORT_SPI_bm; // Swap SCK and MOSI
  SPIC.INTCTRL = SPI_INTLVL_LO_gc; // interupt level

  _spi_send(); // Kick it off
}


drv8711_state_t drv8711_get_state(int driver) {
  if (driver < 0 || DRIVERS <= driver) return DRV8711_DISABLED;
  return drivers[driver].state;
}


void drv8711_set_state(int driver, drv8711_state_t state) {
  if (driver < 0 || DRIVERS <= driver) return;
  drivers[driver].state = state;
}


void drv8711_set_microsteps(int driver, uint16_t msteps) {
  if (driver < 0 || DRIVERS <= driver) return;
  switch (msteps) {
  case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
    break;
  default: return; // Invalid
  }

  drivers[driver].microstep = round(logf(msteps) / logf(2));
}


void drv8711_set_stall_callback(int driver, stall_callback_t cb) {
  drivers[driver].stall_cb = cb;
}


float get_drive_current(int driver) {
  if (driver < 0 || DRIVERS <= driver) return 0;
  return drivers[driver].drive.current;
}


void set_drive_current(int driver, float value) {
  if (driver < 0 || DRIVERS <= driver || value < 0) return;
  if (MAX_CURRENT < value) value = MAX_CURRENT;
  _current_set(&drivers[driver].drive, value);
}


float get_idle_current(int driver) {
  if (driver < 0 || DRIVERS <= driver) return 0;
  return drivers[driver].idle.current;
}


void set_idle_current(int driver, float value) {
  if (driver < 0 || DRIVERS <= driver || value < 0 || MAX_IDLE_CURRENT < value)
    return;

  _current_set(&drivers[driver].idle, value);
}


float get_active_current(int driver) {
  if (driver < 0 || DRIVERS <= driver) return 0;
  return _driver_get_current(&drivers[driver]);
}


bool get_motor_fault() {return motor_fault;}


void set_driver_flags(int driver, uint16_t flags) {
  drivers[driver].reset_flags = true;
}


uint16_t get_driver_flags(int driver) {return drivers[driver].flags;}
bool get_driver_stalled(int driver) {return drivers[driver].stalled;}
