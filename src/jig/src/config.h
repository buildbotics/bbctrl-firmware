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

#pragma once

#include "pins.h"


// Pins
enum {
  STALL_X_PIN = PORT_A << 3,
  STALL_Y_PIN,
  STALL_Z_PIN,
  STALL_A_PIN,
  SPIN_DIR_PIN,
  SPIN_ENABLE_PIN,
  ANALOG_PIN,
  PROBE_PIN,

  MIN_X_PIN = PORT_B << 3,
  MAX_X_PIN,
  MIN_A_PIN,
  MAX_A_PIN,
  MIN_Y_PIN,
  MAX_Y_PIN,
  MIN_Z_PIN,
  MAX_Z_PIN,

  SDA_PIN = PORT_C << 3,
  SCL_PIN,
  SERIAL_RX_PIN,
  SERIAL_TX_PIN,
  SERIAL_CTS_PIN,
  SPI_CLK_PIN,
  SPI_MISO_PIN,
  SPI_MOSI_PIN,

  ENCODER_X_A_PIN = PORT_D << 3,
  ENCODER_X_B_PIN,
  SPI_CS_A_PIN,
  SPI_CS_Z_PIN,
  SPIN_PWM_PIN,
  SWITCH_1_PIN,
  RS485_RO_PIN,
  RS485_DI_PIN,

  STEP_Y_PIN = PORT_E << 3,
  SPI_CS_Y_PIN,
  DIR_X_PIN,
  DIR_Y_PIN,
  STEP_A_PIN,
  SWITCH_2_PIN,
  DIR_Z_PIN,
  DIR_A_PIN,

  STEP_Z_PIN = PORT_F << 3,
  RS485_RW_PIN,
  FAULT_PIN,
  ESTOP_PIN,
  MOTOR_FAULT_PIN,
  MOTOR_ENABLE_PIN,
  NC_0_PIN,
  NC_1_PIN,
};


// Serial settings
#define SERIAL_BAUD            USART_BAUD_115200
#define SERIAL_PORT            USARTC0
#define SERIAL_DRE_vect        USARTC0_DRE_vect
#define SERIAL_RXC_vect        USARTC0_RXC_vect


// Input
#define INPUT_BUFFER_LEN       255 // text buffer size (255 max)


// Encoder
#define ENCODER_LINES 600
