/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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

/******************************************************************************\

  Modbus RTU command format is as follows:

    Function 01: Read Coil Status
    Function 02: Read Input Status

           Send: [id][func][addr][count][crc]
        Receive: [id][func][bytes][flags][crc]

    Function 03: Read Holding Registers
    Function 04: Read Input Registers

           Send: [id][func][addr][count][crc]
        Receive: [id][func][bytes][regs][crc]

    Function 05: Force Single Coil
    Function 06: Preset Single Register

           Send: [id][func][addr][value][crc]
        Receive: [id][func][addr][value][crc]

    Function 15: Force Multiple Coils

           Send: [id][func][addr][count][bytes][flags][crc]
        Receive: [id][func][addr][count][crc]

    Function 16: Preset Multiple Registers

           Send: [id][func][addr][count][bytes][regs][crc]
        Receive: [id][func][addr][count][crc]

  Where:

             id: 1-byte   Slave ID
           func: 1-byte   Function code
           addr: 2-byte   Data address
          count: 2-byte   Address count
          bytes: 1-byte   Number of bytes to follow
          value: 2-byte   Value read or written
           bits: n-bytes  Flags indicating on/off
           regs: n-bytes  register values to write
       checksum: 16-bit   CRC: x^16 + x^15 + x^2 + 1 (0x8005) initial: 0xffff

\******************************************************************************/

#pragma once


#include <stdint.h>
#include <stdbool.h>


typedef enum {
  MODBUS_READ_COILS        = 1,
  MODBUS_READ_CONTACTS     = 2,
  MODBUS_READ_OUTPUT_REG   = 3,
  MODBUS_READ_INPUT_REG    = 4,
  MODBUS_WRITE_COIL        = 5,
  MODBUS_WRITE_OUTPUT_REG  = 6,
  MODBUS_WRITE_COILS       = 15,
  MODBUS_WRITE_OUTPUT_REGS = 16,
} modbus_function_code_t;


typedef enum {
  MODBUS_COIL_BASE       = 0,
  MODBUS_CONTACT_BASE    = 10000,
  MODBUS_INPUT_REG_BASE  = 30000,
  MODBUS_OUTPUT_REG_BASE = 40000,
} modbus_base_addrs_t;


typedef void (*modbus_cb_t)(uint8_t func, uint8_t bytes, const uint8_t *data);
typedef void (*modbus_rw_cb_t)(bool ok, uint16_t addr, uint16_t value);

void modbus_init();
void modbus_deinit();
bool modbus_busy();
void modbus_func(uint8_t func, uint8_t send, const uint8_t *data,
                 uint8_t receive, modbus_cb_t cb);
void modbus_read(uint16_t addr, uint16_t count, modbus_rw_cb_t cb);
void modbus_write(uint16_t addr, uint16_t value, modbus_rw_cb_t cb);
void modbus_multi_write(uint16_t addr, uint16_t value, modbus_rw_cb_t cb);
void modbus_callback();
