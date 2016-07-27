/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2013 Alden S. Hart Jr.
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
\
       You should have received a copy of the GNU Lesser General Public
                License along with the software.  If not, see
                       <http://www.gnu.org/licenses/>.

                For information regarding this software email:
                  "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#include "i2c.h"
#include "config.h"

#include <avr/interrupt.h>

#include <stdbool.h>


typedef struct {
  spi_cb_t cb;
  uint8_t data[I2C_MAX_DATA];
  uint8_t length;
} spi_t;

static spi_t spi = {0};


ISR(I2C_ISR) {
  static bool first = false;
  uint8_t status = I2C_DEV.SLAVE.STATUS;

  // Error or collision
  if (status & (TWI_SLAVE_BUSERR_bm | TWI_SLAVE_COLL_bm)) return; // Ignore

  // Address match
  else if ((status & TWI_SLAVE_APIF_bm) && (status & TWI_SLAVE_AP_bm)) {
    I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc; // ACK address byte
    first = true;
    spi.length = 0;

  } else if (status & TWI_SLAVE_APIF_bm) { // STOP interrupt
    I2C_DEV.SLAVE.STATUS = TWI_SLAVE_APIF_bm; // Clear interrupt flag
    if (spi.cb) spi.cb(spi.data, spi.length);

  } else if (status & TWI_SLAVE_DIF_bm) { // Data interrupt
    if (status & TWI_SLAVE_DIR_bm) { // Write
      // Check if master ACKed last byte sent
      if (status & TWI_SLAVE_RXACK_bm && !first)
        I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc; // End transaction

      else {
        I2C_DEV.SLAVE.DATA = 0; // Send some data
        I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc; // Continue transaction
      }

      first = false;

    } else { // Read
      uint8_t data = I2C_DEV.SLAVE.DATA;
      if (spi.length < I2C_MAX_DATA) spi.data[spi.length++] = data;

      // ACK and continue transaction
      I2C_DEV.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
    }
  }
}


void i2c_init() {
  I2C_DEV.SLAVE.CTRLA = TWI_SLAVE_INTLVL_HI_gc | TWI_SLAVE_DIEN_bm |
    TWI_SLAVE_ENABLE_bm | TWI_SLAVE_APIEN_bm | TWI_SLAVE_PIEN_bm;
  I2C_DEV.SLAVE.ADDR = I2C_ADDR << 1;
}


void i2c_set_callback(spi_cb_t cb) {spi.cb = cb;}
