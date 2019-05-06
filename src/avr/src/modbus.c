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
#include "util.h"
#include "estop.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>

#include <string.h>
#include <stdio.h>


typedef enum {
  MODBUS_DISCONNECTED,
  MODBUS_OK,
  MODBUS_CRC,
  MODBUS_INVALID,
  MODBUS_TIMEDOUT,
} modbus_status_t;


static struct {
  bool debug;
  uint8_t id;
  baud_t baud;
  parity_t parity;
} cfg = {false, 1, USART_BAUD_9600, USART_NONE};


static struct {
  uint8_t bytes;
  uint8_t command[MODBUS_BUF_SIZE];
  uint8_t command_length;
  uint8_t response[MODBUS_BUF_SIZE];
  uint8_t response_length;

  uint16_t addr;
  modbus_rw_cb_t rw_cb;
  modbus_cb_t receive_cb;

  uint32_t last_write;
  uint32_t last_read;
  uint8_t retry;
  uint8_t status;
  uint16_t crc_errs;
  bool write_ready;
  bool response_ready;
  bool transmit_complete;
  bool busy;
} state = {0};


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


static void _write_word(uint8_t *dst, uint16_t value, bool little_endian) {
  dst[!little_endian]  = value;
  dst[little_endian] = value >> 8;
}


static uint16_t _read_word(const uint8_t *data, bool little_endian) {
  return (uint16_t)data[little_endian] << 8 | data[!little_endian];
}


static bool _check_response() {
  // Check CRC
  uint16_t computed = _crc16(state.response, state.response_length - 2);
  uint16_t expected =
    _read_word(state.response + state.response_length - 2, true);

  if (computed != expected) {
    if (cfg.debug) {
      char sent[state.command_length * 2 + 1];
      char response[state.response_length * 2 + 1];
      format_hex_buf(sent, state.command, state.command_length);
      format_hex_buf(response, state.response, state.response_length);

      STATUS_WARNING(STAT_OK, "modbus: invalid CRC, received=0x%04x "
                     "computed=0x%04x sent=0x%s received=0x%s",
                     expected, computed, sent, response);
    }

    state.crc_errs++;
    state.status = MODBUS_CRC;
    return false;
  }

  // Check that slave id matches
  if (state.command[0] != state.response[0]) {
    STATUS_WARNING(STAT_OK, "modbus: unexpected slave id, expected=%u got=%u",
                   state.command[0], state.response[0]);
    state.status = MODBUS_INVALID;
    return false;
  }

  // Check that function code matches
  if (state.command[1] != state.response[1]) {
    STATUS_WARNING(STAT_OK, "modbus: invalid function code, expected=%u got=%u",
                   state.command[1], state.response[1]);
    state.status = MODBUS_INVALID;
    return false;
  }

  return true;
}


static void _notify(uint8_t func, uint8_t bytes, const uint8_t *data) {
  if (!state.receive_cb) return;

  modbus_cb_t cb = state.receive_cb;
  state.receive_cb = 0; // May be set in callback

  cb(func, bytes, data);
}


static void _handle_response() {
  if (!state.response_ready) return;
  state.response_ready = false;

  if (!_check_response()) return;

  state.last_write = 0;             // Clear timeout timer
  state.last_read = rtc_get_time(); // Set delay timer
  state.retry = 0;                  // Reset retry counter
  state.status = MODBUS_OK;
  state.busy = false;

  _notify(state.response[1], state.response_length - 4, state.response + 2);
}


/// Data register empty interrupt
ISR(RS485_DRE_vect) {
  RS485_PORT.DATA = state.command[state.bytes++];

  if (state.bytes == state.command_length) {
    _set_dre_interrupt(false);
    _set_txc_interrupt(true);
    state.bytes = 0;
  }
}


/// Transmit complete interrupt
ISR(RS485_TXC_vect) {
  _set_txc_interrupt(false);
  _set_rxc_interrupt(true);
  _set_write(false); // Switch to read mode
  state.transmit_complete = true;
}


/// Data received interrupt
ISR(RS485_RXC_vect) {
  state.response[state.bytes] = RS485_PORT.DATA;

  // Ignore leading zeros
  if (state.bytes || state.response[0]) state.bytes++;

  if (state.bytes == state.response_length) {
    _set_rxc_interrupt(false);
    _set_write(true); // Back to write mode
    state.bytes = 0;
    state.response_ready = true;
  }
}


static void _read_cb(uint8_t func, uint8_t bytes, const uint8_t *data) {
  if (func == MODBUS_READ_OUTPUT_REG && data[0] == bytes - 1) {
    if (state.rw_cb)
      for (uint8_t i = 0; i < bytes >> 1; i++)
        state.rw_cb(true, state.addr + i, _read_word(data + i * 2 + 1, false));
    return;
  }

  if (state.rw_cb) state.rw_cb(false, state.addr, 0);
}


static void _write_cb(uint8_t func, uint8_t bytes, const uint8_t *data) {
  if ((func == MODBUS_WRITE_OUTPUT_REG ||
       func == MODBUS_WRITE_OUTPUT_REGS) && bytes == 4 &&
      _read_word(data, false) == state.addr) {
    if (state.rw_cb)
      state.rw_cb(true, state.addr, _read_word(state.command + 4, false));
    return;
  }

  if (state.rw_cb) state.rw_cb(false, state.addr, 0);
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

  // Clear state
  state.write_ready = false;
  state.busy = false;
}


static void _retry() {
  state.last_write = 0;
  state.bytes = 0;
  state.retry++;

  _set_write(true); // RS485 write mode

  _set_txc_interrupt(false);
  _set_rxc_interrupt(false);
  _set_dre_interrupt(true);

  // Try changing pin polarity
  if (state.retry == MODBUS_RETRIES) {
    PINCTRL_PIN(RS485_RO_PIN) ^= PORT_INVEN_bm;
    PINCTRL_PIN(RS485_DI_PIN) ^= PORT_INVEN_bm;
  }

  if (cfg.debug) STATUS_DEBUG("modbus: retry %d", state.retry);
}


static void _timeout() {
  if (cfg.debug) STATUS_DEBUG("modbus: timedout");

  if (state.status == MODBUS_OK || state.status == MODBUS_DISCONNECTED)
    state.status = MODBUS_TIMEDOUT;

  _reset();
  _notify(state.command[1], 0, 0);
}


static stop_t _get_stop() {
  // RTU mode characters must always be 11-bits long
  return cfg.parity == USART_NONE ? USART_2STOP : USART_1STOP;
}


void modbus_init() {
  PR.PRPD &= ~PR_USART1_bm; // Disable power reduction

  DIRCLR_PIN(RS485_RO_PIN); // Input
  OUTSET_PIN(RS485_DI_PIN); // High
  DIRSET_PIN(RS485_DI_PIN); // Output
  OUTSET_PIN(RS485_RW_PIN); // High
  DIRSET_PIN(RS485_RW_PIN); // Output

  _reset();
  memset(&state, 0, sizeof(state));
  state.status = MODBUS_DISCONNECTED;

  usart_init_port(&RS485_PORT, cfg.baud, cfg.parity, USART_8BITS, _get_stop());
}


void modbus_deinit() {
  _reset();
  memset(&state, 0, sizeof(state));
  state.status = MODBUS_DISCONNECTED;

  // Disable USART
  RS485_PORT.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);

  // Float write pins
  DIRCLR_PIN(RS485_DI_PIN);
  DIRCLR_PIN(RS485_RW_PIN);
}


bool modbus_busy() {return state.busy;}


static void _start_write() {
  if (!state.write_ready) return;

  // The minimum delay between modbus messages is 3.5 characters.  There are
  // 11-bits per character in RTU mode.  Character time is calculated as
  // follows:
  //
  //     char time = 11-bits / baud * 3.5
  //
  // At 9600 baud the minimum delay is 4.01ms.  At 19200 it's 2.005ms.  All
  // supported higher baud rates require the 1.75ms minimum delay.  We round up
  // and add 1ms to ensure the delay is never less than the required minimum.
  if (state.last_read &&
      !rtc_expired(state.last_read + (cfg.baud == USART_BAUD_9600 ? 5 : 3)))
    return;
  state.last_read = 0;

  state.write_ready = false;
  _set_dre_interrupt(true);
}


void modbus_func(uint8_t func, uint8_t send, const uint8_t *data,
                 uint8_t receive, modbus_cb_t receive_cb) {
  state.bytes = 0;
  state.command_length = send + 4;
  state.response_length = receive + 4;
  state.receive_cb = receive_cb;
  state.last_write = 0;
  state.retry = 0;

  ESTOP_ASSERT(state.command_length <= MODBUS_BUF_SIZE, STAT_MODBUS_BUF_LENGTH);
  ESTOP_ASSERT(state.response_length <= MODBUS_BUF_SIZE,
               STAT_MODBUS_BUF_LENGTH);

  state.command[0] = cfg.id;
  state.command[1] = func;
  memcpy(state.command + 2, data, send);
  _write_word(state.command + send + 2, _crc16(state.command, send + 2), true);

  state.busy = true;
  state.write_ready = true;
  _start_write();
}


void modbus_read(uint16_t addr, uint16_t count, modbus_rw_cb_t cb) {
  state.rw_cb = cb;
  state.addr = addr;
  uint8_t cmd[4];
  _write_word(cmd, addr, false);
  _write_word(cmd + 2, count, false);
  modbus_func(MODBUS_READ_OUTPUT_REG, 4, cmd, 2 * count + 1, _read_cb);
}


void modbus_write(uint16_t addr, uint16_t value, modbus_rw_cb_t cb) {
  state.rw_cb = cb;
  state.addr = addr;
  uint8_t cmd[4];
  _write_word(cmd, addr, false);
  _write_word(cmd + 2, value, false);
  modbus_func(MODBUS_WRITE_OUTPUT_REG, 4, cmd, 4, _write_cb);
}


void modbus_multi_write(uint16_t addr, uint16_t value, modbus_rw_cb_t cb) {
  state.rw_cb = cb;
  state.addr = addr;
  uint8_t cmd[7];
  _write_word(cmd, addr, false);      // Start address
  _write_word(cmd + 2, 1, false);     // Number of regs
  cmd[4] = 2;                         // Number of bytes
  _write_word(cmd + 5, value, false); // Value
  modbus_func(MODBUS_WRITE_OUTPUT_REGS, 7, cmd, 4, _write_cb);
}


void modbus_callback() {
  if (state.transmit_complete) {
    state.last_write = rtc_get_time();
    state.transmit_complete = false;
  }

  _handle_response();
  _start_write();

  // Timeout out writes
  if (!state.last_write || !rtc_expired(state.last_write + MODBUS_TIMEOUT))
    return;
  state.last_write = 0;

  if (cfg.debug && state.bytes) {
    char sent[state.command_length * 2 + 1];
    char received[state.bytes * 2 + 1];

    format_hex_buf(sent, state.command, state.command_length);
    format_hex_buf(received, state.response, state.bytes);

    STATUS_DEBUG("modbus: sent 0x%s received 0x%s expected %u bytes",
                 sent, received, state.response_length);
  }

  if (state.retry < 2 * MODBUS_RETRIES) _retry();
  else _timeout();
}


// Variable callbacks
bool get_mb_debug() {return cfg.debug;}
void set_mb_debug(bool value) {cfg.debug = value;}
uint8_t get_mb_id() {return cfg.id;}
void set_mb_id(uint8_t id) {cfg.id = id;}
uint8_t get_mb_baud() {return cfg.baud;}


void set_mb_baud(uint8_t baud) {
  cfg.baud = (baud_t)baud;
  usart_set_baud(&RS485_PORT, cfg.baud);
}


uint8_t get_mb_parity() {return cfg.parity;}


void set_mb_parity(uint8_t parity) {
  cfg.parity = (parity_t)parity;
  usart_set_parity(&RS485_PORT, cfg.parity);
  usart_set_stop(&RS485_PORT, _get_stop());
}


uint8_t get_mb_status() {return state.status;}
uint16_t get_mb_crc_errs() {return state.crc_errs;}
