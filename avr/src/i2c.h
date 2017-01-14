/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

       You should have received a copy of the GNU Lesser General Public
                License along with the software.  If not, see
                       <http://www.gnu.org/licenses/>.

                For information regarding this software email:
                  "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#pragma once

#include "config.h"

#include <stdbool.h>


typedef enum {
  I2C_NULL,
  I2C_ESTOP,
  I2C_CLEAR,
  I2C_PAUSE,
  I2C_OPTIONAL_PAUSE,
  I2C_RUN,
  I2C_STEP,
  I2C_FLUSH,
  I2C_REPORT,
  I2C_HOME,
  I2C_REBOOT,
  I2C_ZERO,
} i2c_cmd_t;


typedef void (*i2c_read_cb_t)(i2c_cmd_t cmd, uint8_t *data, uint8_t length);
typedef uint8_t (*i2c_write_cb_t)(uint8_t offset, bool *done);


void i2c_init();
void i2c_set_read_callback(i2c_read_cb_t cb);
void i2c_set_write_callback(i2c_write_cb_t cb);
