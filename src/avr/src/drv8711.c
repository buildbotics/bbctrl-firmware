/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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
#include "report.h"

#include <avr/interrupt.h>
#include <util/delay.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define DRIVERS MOTORS
#define COMMANDS 10


#define DRV8711_WORD_BYTE_PTR(WORD, LOW) (((uint8_t *)&(WORD)) + !(LOW))


bool motor_fault = false;


typedef struct {
  float current;
  uint16_t isgain;
  uint8_t torque;
} current_t;


typedef struct {
  uint8_t status;
  uint16_t flags;

  drv8711_state_t state;
  current_t drive;
  current_t idle;
  float stall_threspause;

  uint8_t mode; // microstepping mode
  stall_callback_t stall_cb;

  uint8_t cs_pin;
  uint8_t stall_pin;
} drv8711_driver_t;


static drv8711_driver_t drivers[DRIVERS] = {
  {.cs_pin = SPI_CS_X_PIN, .stall_pin = STALL_X_PIN},
  {.cs_pin = SPI_CS_Y_PIN, .stall_pin = STALL_Y_PIN},
  {.cs_pin = SPI_CS_Z_PIN, .stall_pin = STALL_Z_PIN},
  {.cs_pin = SPI_CS_A_PIN, .stall_pin = STALL_A_PIN},
};


typedef struct {
  uint8_t *read;
  bool callback;
  uint8_t disable_cs_pin;

  uint8_t cmd;
  uint8_t driver;
  bool low_byte;

  uint8_t ncmds;
  uint16_t commands[DRIVERS][COMMANDS];
  uint16_t responses[DRIVERS];
} spi_t;

static spi_t spi = {0};


static void _current_set(current_t *c, float current) {
  c->current = current;

  float torque_over_gain = current * CURRENT_SENSE_RESISTOR / CURRENT_SENSE_REF;
  float gain = 0;

  if (torque_over_gain < 1.0 / 40) {
    c->isgain = DRV8711_CTRL_ISGAIN_40;
    gain = 40;

  } else if (torque_over_gain < 1.0 / 20) {
    c->isgain = DRV8711_CTRL_ISGAIN_20;
    gain = 20;

  } else if (torque_over_gain < 1.0 / 10) {
    c->isgain = DRV8711_CTRL_ISGAIN_10;
    gain = 10;

  } else if (torque_over_gain < 1.0 / 5) {
    c->isgain = DRV8711_CTRL_ISGAIN_5;
    gain = 5;
  }

  c->torque = round(torque_over_gain * gain * 256);
}


static bool _driver_get_enabled(int driver) {
  drv8711_state_t state = drivers[driver].state;
  return state == DRV8711_IDLE || state == DRV8711_ACTIVE;
}


static float _driver_get_current(int driver) {
  drv8711_driver_t *drv = &drivers[driver];

  switch (drv->state) {
  case DRV8711_IDLE: return drv->idle.current;
  case DRV8711_ACTIVE: return drv->drive.current;
  default: return 0; // Off
  }
}


static uint16_t _driver_get_isgain(int driver) {
  drv8711_driver_t *drv = &drivers[driver];

  switch (drv->state) {
  case DRV8711_IDLE: return drv->idle.isgain;
  case DRV8711_ACTIVE: return drv->drive.isgain;
  default: return 0; // Off
  }
}


static uint8_t _driver_get_torque(int driver) {
  drv8711_driver_t *drv = &drivers[driver];

  switch (drv->state) {
  case DRV8711_IDLE: return drv->idle.torque;
  case DRV8711_ACTIVE: return drv->drive.torque;
  default: return 0; // Off
  }
}


static uint8_t _spi_next_command(uint8_t cmd) {
  // Process command responses
  for (int driver = 0; driver < DRIVERS; driver++) {
    drv8711_driver_t *drv = &drivers[driver];
    uint16_t command = spi.commands[driver][cmd];

    if (DRV8711_CMD_IS_READ(command))
      switch (DRV8711_CMD_ADDR(command)) {
      case DRV8711_STATUS_REG:
        drv->status = spi.responses[driver];

        if ((drv->status & drv->flags) != drv->status) {
          drv->flags |= drv->status;
          report_request();
        }
        break;

      case DRV8711_OFF_REG:
        // We read back the OFF register to test for communication failure.
        if ((spi.responses[driver] & 0x1ff) != DRV8711_OFF)
          drv->flags |= DRV8711_COMM_ERROR_bm;
        break;
      }
  }

  // Next command
  if (++cmd == spi.ncmds) {
    cmd = 0; // Wrap around
    st_enable(); // Enable motors
  }

  // Prep next command
  for (int driver = 0; driver < DRIVERS; driver++) {
    drv8711_driver_t *drv = &drivers[driver];
    uint16_t *command = &spi.commands[driver][cmd];

    switch (DRV8711_CMD_ADDR(*command)) {
    case DRV8711_STATUS_REG:
      if (!DRV8711_CMD_IS_READ(*command))
        // Clear STATUS flags
        *command = (*command & 0xf000) | (0x0fff & ~(drv->status));
      break;

    case DRV8711_TORQUE_REG: // Update motor current setting
      *command = (*command & 0xff00) | _driver_get_torque(driver);
      break;

    case DRV8711_CTRL_REG: // Set microsteps
      // NOTE, we disable the driver if it's not active.  Otherwise, the chip
      // gets hot if when idling with the driver enabled.
      *command = (*command & 0xfc86) | _driver_get_isgain(driver) |
        (drv->mode << 3) |
        ((_driver_get_enabled(driver) && _driver_get_torque(driver)) ?
         DRV8711_CTRL_ENBL_bm : 0);
      break;

    default: break;
    }
  }

  return cmd;
}


static void _spi_send() {
  // Flush any status errors (TODO check for errors)
  uint8_t x = SPIC.STATUS;
  x = x;

  // Disable CS
  if (spi.disable_cs_pin) {
    OUTCLR_PIN(spi.disable_cs_pin); // Set low (inactive)
    _delay_us(1);
    spi.disable_cs_pin = 0;
  }

  // Schedule next CS disable or enable next CS now
  if (spi.low_byte) spi.disable_cs_pin = drivers[spi.driver].cs_pin;
  else {
    OUTSET_PIN(drivers[spi.driver].cs_pin); // Set high (active)
    _delay_us(1);
  }

  // Read
  if (spi.read) {
    *spi.read = SPIC.DATA;
    spi.read = 0;
  }

  // Callback, passing current command index, and get next command index
  if (spi.callback) {
    spi.cmd = _spi_next_command(spi.cmd);
    spi.callback = false;
  }

  // Write byte and prep next read
  SPIC.DATA =
    *DRV8711_WORD_BYTE_PTR(spi.commands[spi.driver][spi.cmd], spi.low_byte);
  spi.read = DRV8711_WORD_BYTE_PTR(spi.responses[spi.driver], spi.low_byte);

  // Check if WORD complete, go to next driver & check if command finished
  if (spi.low_byte && ++spi.driver == DRIVERS) {
    spi.driver = 0;      // Wrap around
    spi.callback = true; // Call back after last byte is read
  }

  // Next byte
  spi.low_byte = !spi.low_byte;
}


static void _init_spi_commands() {
  // Setup SPI command sequence
  for (int driver = 0; driver < DRIVERS; driver++) {
    uint16_t *commands = spi.commands[driver];
    spi.ncmds = 0;

    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_OFF_REG,    DRV8711_OFF);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_BLANK_REG,  DRV8711_BLANK);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_DECAY_REG,  DRV8711_DECAY);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_STALL_REG,  DRV8711_STALL);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_DRIVE_REG,  DRV8711_DRIVE);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_TORQUE_REG, DRV8711_TORQUE);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_CTRL_REG,   DRV8711_CTRL);
    commands[spi.ncmds++] = DRV8711_READ(DRV8711_OFF_REG);
    commands[spi.ncmds++] = DRV8711_READ(DRV8711_STATUS_REG);
    commands[spi.ncmds++] = DRV8711_WRITE(DRV8711_STATUS_REG, 0);
  }

  if (COMMANDS < spi.ncmds)
    STATUS_ERROR(STAT_INTERNAL_ERROR,
                 "SPI command buffer overflow increase COMMANDS in %s",
                 __FILE__);

  _spi_send(); // Kick it off
}


ISR(SPIC_INT_vect) {_spi_send();}


ISR(STALL_ISR_vect) {
  for (int i = 0; i < DRIVERS; i++) {
    drv8711_driver_t *driver = &drivers[i];
    if (driver->stall_cb) driver->stall_cb(i);
  }
}


ISR(FAULT_ISR_vect) {motor_fault = !IN_PIN(MOTOR_FAULT_PIN);} // TODO


void drv8711_init() {
  // Setup pins
  // Must set the SS pin either in/high or any/output for master mode to work
  // Note, this pin is also used by the USART as the CTS line
  DIRSET_PIN(SPI_SS_PIN); // Output
  OUTSET_PIN(SPI_CLK_PIN); // High
  DIRSET_PIN(SPI_CLK_PIN); // Output
  DIRCLR_PIN(SPI_MISO_PIN); // Input
  OUTSET_PIN(SPI_MOSI_PIN); // High
  DIRSET_PIN(SPI_MOSI_PIN); // Output

  for (int i = 0; i < DRIVERS; i++) {
    uint8_t cs_pin = drivers[i].cs_pin;
    uint8_t stall_pin = drivers[i].stall_pin;

    OUTSET_PIN(cs_pin);     // High
    DIRSET_PIN(cs_pin);     // Output
    DIRCLR_PIN(stall_pin);  // Input

    // Stall interrupt
    PINCTRL_PIN(stall_pin) = PORT_ISC_FALLING_gc;
    PORT(stall_pin)->INT1MASK |= BM(stall_pin);
    PORT(stall_pin)->INTCTRL |= PORT_INT1LVL_HI_gc;
  }

  // Fault interrupt
  DIRCLR_PIN(MOTOR_FAULT_PIN);
  PINCTRL_PIN(MOTOR_FAULT_PIN) = PORT_ISC_RISING_gc;
  PORT(MOTOR_FAULT_PIN)->INT1MASK |= BM(MOTOR_FAULT_PIN);
  PORT(MOTOR_FAULT_PIN)->INTCTRL |= PORT_INT1LVL_HI_gc;

  // Configure SPI
  PR.PRPC &= ~PR_SPI_bm; // Disable power reduction
  SPIC.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_0_gc |
    SPI_PRESCALER_DIV16_gc; // enable, big endian, master, mode, clock div
  PORT(SPI_CLK_PIN)->REMAP = PORT_SPI_bm; // Swap SCK and MOSI
  SPIC.INTCTRL = SPI_INTLVL_LO_gc; // interupt level

  _init_spi_commands();
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

  drivers[driver].mode = round(logf(msteps) / logf(2));
}


void drv8711_set_stall_callback(int driver, stall_callback_t cb) {
  drivers[driver].stall_cb = cb;
}


float get_drive_current(int driver) {
  if (driver < 0 || DRIVERS <= driver) return 0;
  return drivers[driver].drive.current;
}


void set_drive_current(int driver, float value) {
  if (driver < 0 || DRIVERS <= driver || value < 0 || MAX_CURRENT < value)
    return;
  _current_set(&drivers[driver].drive, value);
}


float get_idle_current(int driver) {
  if (driver < 0 || DRIVERS <= driver) return 0;
  return drivers[driver].idle.current;
}


void set_idle_current(int driver, float value) {
  if (driver < 0 || DRIVERS <= driver || value < 0 || MAX_CURRENT < value)
    return;

  _current_set(&drivers[driver].idle, value);
}


float get_active_current(int driver) {
  if (driver < 0 || DRIVERS <= driver) return 0;
  return _driver_get_current(driver);
}


bool get_motor_fault() {return motor_fault;}


uint16_t get_driver_flags(int driver) {return drivers[driver].flags;}


void print_status_flags(uint16_t flags) {
  bool first = true;

  putchar('"');

  if (DRV8711_STATUS_OTS_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("temp"));
    first = false;
  }

  if (DRV8711_STATUS_AOCP_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("current a"));
    first = false;
  }

  if (DRV8711_STATUS_BOCP_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("current b"));
    first = false;
  }

  if (DRV8711_STATUS_APDF_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("fault a"));
    first = false;
  }

  if (DRV8711_STATUS_BPDF_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("fault b"));
    first = false;
  }

  if (DRV8711_STATUS_UVLO_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("uvlo"));
    first = false;
  }

  if ((DRV8711_STATUS_STD_bm | DRV8711_STATUS_STDLAT_bm) & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("stall"));
    first = false;
  }

  if (DRV8711_COMM_ERROR_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("comm"));
    first = false;
  }

  putchar('"');
}


uint16_t get_status_strings(int driver) {return get_driver_flags(driver);}


// Command callback
void command_mreset(int argc, char *argv[]) {
  if (argc == 1)
    for (int driver = 0; driver < DRIVERS; driver++)
      drivers[driver].flags = 0;

  else {
    int driver = atoi(argv[1]);
    if (driver < DRIVERS) drivers[driver].flags = 0;
  }

  report_request();
}
