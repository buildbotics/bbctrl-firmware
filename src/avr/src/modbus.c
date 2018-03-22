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

#include "modbus.h"
#include "usart.h"
#include "status.h"
#include "rtc.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>

#include <string.h>
#include <stdio.h>


static struct {
  bool debug;
  uint8_t baud;

  uint8_t bytes;
  uint8_t command[MODBUS_BUF_SIZE];
  uint8_t command_length;
  uint8_t response[MODBUS_BUF_SIZE];
  uint8_t response_length;

  modbus_cb_t receive_cb;

  uint32_t last;
  uint8_t retry;
  bool connected;
  bool busy;
} mb = {true, USART_BAUD_9600};


static uint16_t _crc16(const uint8_t *buffer, unsigned length) {
  uint16_t crc = 0xffff;

  for (unsigned i = 0; i < length; i++)
    crc = _crc16_update(crc, buffer[i]);

  return crc;
}


static void _set_write(bool x) {SET_PIN(RS485_RW_PIN, x);}


#define INTLVL_SET(REG, NAME, LEVEL)                        \
  REG = ((REG) & ~NAME##INTLVL_gm) | NAME##INTLVL_##LEVEL##_gc


#define INTLVL_ENABLE(REG, NAME, LEVEL, ENABLE)  do {   \
    if (ENABLE) INTLVL_SET(REG, NAME, LEVEL);           \
    else INTLVL_SET(REG, NAME, OFF);                    \
  } while (0)


static void _set_dre_interrupt(bool enable) {
  INTLVL_ENABLE(RS485_PORT.CTRLA, USART_DRE, MED, enable);
}


static void _set_txc_interrupt(bool enable) {
  INTLVL_ENABLE(RS485_PORT.CTRLA, USART_TXC, MED, enable);
}


static void _set_rxc_interrupt(bool enable) {
  INTLVL_ENABLE(RS485_PORT.CTRLA, USART_RXC, MED, enable);
}


static bool _check_response() {
  // Check CRC
  uint16_t computed = _crc16(mb.response, mb.response_length - 2);
  uint16_t expected = (mb.response[mb.response_length - 1] << 8) |
    mb.response[mb.response_length - 2];

  if (computed != expected) {
    STATUS_WARNING(STAT_OK, "modbus: invalid CRC, expected=0x%04u got=0x%04u",
                   expected, computed);
    return false;
  }

  // Check that slave id matches
  if (mb.command[0] != mb.response[0]) {
    STATUS_WARNING(STAT_OK, "modbus: unexpected slave id, expected=%u got=%u",
                   mb.command[0], mb.response[0]);
    return false;
  }

  // Check that function code matches
  if (mb.command[1] != mb.response[1]) {
    STATUS_WARNING(STAT_OK, "modbus: invalid function code, expected=%u got=%u",
                   mb.command[1], mb.response[1]);
    return false;
  }

  return true;
}


static void _handle_response() {
  if (!_check_response()) return;

  mb.last = 0;  // Clear timeout timer
  mb.retry = 0; // Reset retry counter

  if (mb.receive_cb)
    mb.receive_cb(mb.response[0], mb.response[1], mb.response_length - 4,
                  mb.response + 2);

  mb.connected = true;
  mb.busy = false;
}


/// Data register empty interrupt
ISR(RS485_DRE_vect) {
  RS485_PORT.DATA = mb.command[mb.bytes++];

  if (mb.bytes == mb.command_length) {
    _set_dre_interrupt(false);
    _set_txc_interrupt(true);
    mb.bytes = 0;
  }
}


/// Transmit complete interrupt
ISR(RS485_TXC_vect) {
  _set_txc_interrupt(false);
  _set_rxc_interrupt(true);
  _set_write(false);        // Switch to read mode
  mb.last = rtc_get_time(); // Set timeout timer
}


/// Data received interrupt
ISR(RS485_RXC_vect) {
  mb.response[mb.bytes++] = RS485_PORT.DATA;

  if (mb.bytes == mb.response_length) {
    _set_rxc_interrupt(false);
    _set_write(true); // Back to write mode
    mb.bytes = 0;
    _handle_response();
  }
}


static void _reset() {
  _set_dre_interrupt(false);
  _set_txc_interrupt(false);
  _set_rxc_interrupt(false);
  _set_write(true); // RS485 write mode

  // Flush USART
  uint8_t x = RS485_PORT.DATA;
  x = RS485_PORT.STATUS;
  x = x;

  // Save settings
  bool debug = mb.debug;
  uint8_t baud = mb.baud;

  // Clear state
  memset(&mb, 0, sizeof(mb));

  // Restore settings
  mb.debug = debug;
  mb.baud = baud;
}


static void _retry() {
  mb.last = 0;
  mb.bytes = 0;
  mb.retry++;

  _set_write(true); // RS485 write mode

  _set_txc_interrupt(false);
  _set_rxc_interrupt(false);
  _set_dre_interrupt(true);

  if (mb.debug) STATUS_DEBUG("modbus: retry %d", mb.retry);
}


static void _timeout() {
  if (mb.debug) STATUS_DEBUG("modbus: timedout");

  // Try changing pin polarity
  PINCTRL_PIN(RS485_RO_PIN) ^= PORT_INVEN_bm;
  PINCTRL_PIN(RS485_DI_PIN) ^= PORT_INVEN_bm;

  mb.retry = -1;
  _retry();
}


void modbus_init() {
  PR.PRPD &= ~PR_USART1_bm; // Disable power reduction

  DIRCLR_PIN(RS485_RO_PIN); // Input
  OUTSET_PIN(RS485_DI_PIN); // High
  DIRSET_PIN(RS485_DI_PIN); // Output
  OUTSET_PIN(RS485_RW_PIN); // High
  DIRSET_PIN(RS485_RW_PIN); // Output

  usart_set_port_baud(&RS485_PORT, mb.baud);

  // No parity, 8 data bits, 1 stop bit
  RS485_PORT.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc |
    USART_CHSIZE_8BIT_gc;

  // Enable receiver and transmitter
  RS485_PORT.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;

  // TODO remove this
  PINCTRL_PIN(RS485_RO_PIN) ^= PORT_INVEN_bm;
  PINCTRL_PIN(RS485_DI_PIN) ^= PORT_INVEN_bm;

  _reset();
}


void modbus_deinit() {
  _reset();

  // Disable USART
  RS485_PORT.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);

  // Float write pins
  DIRCLR_PIN(RS485_DI_PIN);
  DIRCLR_PIN(RS485_RW_PIN);
}


bool modbus_busy() {return mb.busy;}


void modbus_func(uint8_t slave, uint8_t func, uint8_t send, const uint8_t *data,
                 uint8_t receive, modbus_cb_t receive_cb) {
  ASSERT(send + 4 <= MODBUS_BUF_SIZE);
  ASSERT(receive + 4 <= MODBUS_BUF_SIZE);

  _reset();

  mb.command[0] = slave;
  mb.command[1] = func;
  memcpy(mb.command + 2, data, send);
  uint16_t crc = _crc16(mb.command, send + 2);
  mb.command[send + 2] = crc;
  mb.command[send + 3] = crc >> 8;

  mb.command_length = send + 4;
  mb.response_length = receive + 4;
  mb.receive_cb = receive_cb;

  _set_dre_interrupt(true);
}


void modbus_rtc_callback() {
  if (mb.last && rtc_expired(mb.last + MODBUS_TIMEOUT)) {
    if (mb.debug && mb.bytes) {
      const uint8_t buf_len = 8 * 2 + 1;
      char sent[buf_len];
      char received[buf_len];

      uint8_t i;
      for (i = 0; i < mb.command_length; i++)
        sprintf(sent + i * 2, "%02x", mb.command[i]);
      sent[i * 2] = 0;

      for (i = 0; i < mb.bytes; i++)
        sprintf(received + i * 2, "%02x", mb.response[i]);
      received[i * 2] = 0;

      STATUS_DEBUG("modbus: sent 0x%s received 0x%s expected %u bytes",
                   sent, received, mb.response_length);
    }

    if (mb.retry < MODBUS_RETRIES) _retry();
    else _timeout();
  }
}


// Variable callbacks
bool get_modbus_debug() {return mb.debug;}
void set_modbus_debug(bool value) {mb.debug = value;}
uint8_t get_modbus_baud() {return mb.baud;}


void set_modbus_baud(uint8_t baud) {
  mb.baud = baud;
  usart_set_port_baud(&RS485_PORT, baud);
}


bool get_modbus_connected() {return mb.connected;}
