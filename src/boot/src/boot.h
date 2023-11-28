/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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

#pragma once

#define INITIAL_WAIT 1000 // In ms

#define UART_RX_PIN             2
#define UART_TX_PIN             3
#define UART_PORT               PORTC
#define UART_DEVICE             USARTC0

// Baud rate 921600 @ 32Mhz
#define UART_BSEL_VALUE         150
#define UART_BSCALE_VALUE       -7


// Protocol
enum {
  CMD_SYNC                = '\x1b',

  // Informational
  CMD_CHECK_AUTOINCREMENT = 'a',
  CMD_CHECK_BLOCK_SUPPORT = 'b',
  CMD_PROGRAMMER_TYPE     = 'p',
  CMD_DEVICE_CODE         = 't',
  CMD_PROGRAM_ID          = 'S',
  CMD_VERSION             = 'V',
  CMD_HW_VERSION          = 'v', // Unsupported extension
  CMD_READ_SIGNATURE      = 's',
  CMD_READ_CHECKSUM       = 'X',
  CMD_FLASH_LENGTH        = 'n',

  // Addressing
  CMD_SET_ADDRESS         = 'A',
  CMD_SET_EXT_ADDRESS     = 'H',

  // Erase
  CMD_FLASH_ERASE         = 'e',
  CMD_EEPROM_ERASE        = '_',

  // Block Access
  CMD_BLOCK_LOAD          = 'B',
  CMD_BLOCK_READ          = 'g',
  CMD_BLOCK_CRC           = 'i',

  // Byte Access
  CMD_READ_BYTE           = 'R',
  CMD_WRITE_LOW_BYTE      = 'c',
  CMD_WRITE_HIGH_BYTE     = 'C',
  CMD_WRITE_PAGE          = 'm',
  CMD_WRITE_EEPROM_BYTE   = 'D',
  CMD_READ_EEPROM_BYTE    = 'd',

  // Lock and Fuse Bits
  CMD_WRITE_LOCK_BITS     = 'l',
  CMD_READ_LOCK_BITS      = 'r',
  CMD_READ_LOW_FUSE_BITS  = 'F',
  CMD_READ_HIGH_FUSE_BITS = 'N',
  CMD_READ_EXT_FUSE_BITS  = 'Q',

  // Control
  CMD_ENTER_PROG_MODE     = 'P',
  CMD_LEAVE_PROG_MODE     = 'L',
  CMD_EXIT_BOOTLOADER     = 'E',
  CMD_SET_LED             = 'x',
  CMD_CLEAR_LED           = 'y',
  CMD_SET_TYPE            = 'T',
};


// Memory types for block access
enum {
  MEM_EEPROM              = 'E',
  MEM_FLASH               = 'F',
  MEM_USERSIG             = 'U',
  MEM_PRODSIG             = 'P',
};


// Command Responses
enum {
  REPLY_ACK               = '\r',
  REPLY_YES               = 'Y',
  REPLY_ERROR             = '?',
};
