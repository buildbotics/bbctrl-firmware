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

#include "i2c.h"

#include <avr/interrupt.h>

#include <stdbool.h>


typedef struct {
  i2c_read_cb_t read_cb;
  i2c_write_cb_t write_cb;
  uint8_t data[I2C_MAX_DATA + 1];
  uint8_t length;
  bool done;
  bool write;
} i2c_t;

static i2c_t i2c = {0};


static void _i2c_reset_command() {
  i2c.length = 0;
  i2c.done = true;
  i2c.write = false;
}


static void _i2c_end_command() {
  if (i2c.length && !i2c.write && i2c.read_cb) {
    i2c.data[i2c.length] = 0; // Null terminate
    i2c.read_cb(i2c.data, i2c.length);
  }

  _i2c_reset_command();
}


static void _i2c_command_byte(uint8_t byte) {
  i2c.data[i2c.length++] = byte;
}


ISR(I2C_ISR) {
  uint8_t status = I2C_DEV.SLAVE.STATUS;

  // Error or collision
  if (status & (TWI_SLAVE_BUSERR_bm | TWI_SLAVE_COLL_bm)) {
    _i2c_reset_command();
    return; // Ignore

  } else if ((status & TWI_SLAVE_APIF_bm) && (status & TWI_SLAVE_AP_bm)) {
    // START + address match
    I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc; // ACK address byte
    _i2c_end_command(); // Handle repeated START

  } else if (status & TWI_SLAVE_APIF_bm) {
    // STOP
    I2C_DEV.SLAVE.STATUS = TWI_SLAVE_APIF_bm; // Clear interrupt flag
    _i2c_end_command();

  } else if (status & TWI_SLAVE_DIF_bm) {
    i2c.write = status & TWI_SLAVE_DIR_bm;

    // DATA
    if (i2c.write) { // Write
      // Check if master ACKed last byte sent
      if (i2c.length && (status & TWI_SLAVE_RXACK_bm || i2c.done))
        I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc; // End transaction

      else {
        // Send some data
        i2c.done = false;
        I2C_DEV.SLAVE.DATA = i2c.write_cb(i2c.length++, &i2c.done);
        I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc; // Continue transaction
      }

    } else { // Read
      _i2c_command_byte(I2C_DEV.SLAVE.DATA);

      // ACK and continue transaction
      I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
    }
  }
}


static uint8_t _i2c_default_write_cb(uint8_t offset, bool *done) {
  *done = true;
  return 0;
}


void i2c_init() {
  i2c_set_write_callback(_i2c_default_write_cb);

  I2C_DEV.SLAVE.CTRLA = TWI_SLAVE_INTLVL_LO_gc | TWI_SLAVE_DIEN_bm |
    TWI_SLAVE_ENABLE_bm | TWI_SLAVE_APIEN_bm | TWI_SLAVE_PIEN_bm;
  I2C_DEV.SLAVE.ADDR = I2C_ADDR << 1;
}


void i2c_set_read_callback(i2c_read_cb_t cb) {i2c.read_cb = cb;}
void i2c_set_write_callback(i2c_write_cb_t cb) {i2c.write_cb = cb;}
