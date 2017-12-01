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

#include "huanyang.h"
#include "config.h"
#include "rtc.h"
#include "report.h"
#include "pgmspace.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>

#include <stdio.h>
#include <string.h>
#include <math.h>


enum {
  HUANYANG_FUNC_READ = 1,
  HUANYANG_FUNC_WRITE,
  HUANYANG_CTRL_WRITE,
  HUANYANG_CTRL_READ,
  HUANYANG_FREQ_WRITE,
  HUANYANG_RESERVED_1,
  HUANYANG_RESERVED_2,
  HUANYANG_LOOP_TEST
};


enum {
  HUANYANG_BASE_FREQ = 4,
  HUANYANG_MAX_FREQ = 5,
  HUANYANG_MIN_FREQ = 11,
  HUANYANG_RATED_MOTOR_VOLTAGE = 141,
  HUANYANG_RATED_MOTOR_CURRENT = 142,
  HUANYANG_MOTOR_POLE = 143,
  HUANYANG_RATED_RPM = 144,
};


enum {
  HUANYANG_TARGET_FREQ,
  HUANYANG_ACTUAL_FREQ,
  HUANYANG_ACTUAL_CURRENT,
  HUANYANG_ACTUAL_RPM,
  HUANYANG_CONT,
  HUANYANG_TEMPERATURE,
};


enum {
  HUANYANG_FORWARD = 1,
  HUANYANG_STOP = 8,
  HUANYANG_REVERSE = 17,
};


enum {
  HUANYANG_RUN         = 1 << 0,
  HUANYANG_JOG         = 1 << 1,
  HUANYANG_COMMAND_RF  = 1 << 2,
  HUANYANG_RUNNING     = 1 << 3,
  HUANYANG_JOGGING     = 1 << 4,
  HUANYANG_RUNNING_RF  = 1 << 5,
  HUANYANG_BRACKING    = 1 << 6,
  HUANYANG_TRACK_START = 1 << 7,
};


typedef bool (*next_command_cb_t)(int index);


typedef struct {
  uint8_t id;
  bool debug;

  next_command_cb_t next_command_cb;
  uint8_t command_index;
  uint8_t current_offset;
  uint8_t command[8];
  uint8_t command_length;
  uint8_t response_length;
  uint8_t response[10];
  uint32_t last;
  uint8_t retry;

  bool connected;
  bool changed;
  float speed;

  float actual_freq;
  float actual_current;
  uint16_t actual_rpm;
  uint16_t temperature;

  float max_freq;
  float min_freq;
  uint16_t rated_rpm;

  uint8_t status;
} huanyang_t;


static huanyang_t ha = {0};


#define CTRL_STATUS_RESPONSE(R) ((uint16_t)R[4] << 8 | R[5])
#define FUNC_RESPONSE(R) (R[2] == 2 ? R[4] : ((uint16_t)R[4] << 8 | R[5]))


static uint16_t _crc16(const uint8_t *buffer, unsigned length) {
  uint16_t crc = 0xffff;

  for (unsigned i = 0; i < length; i++)
    crc = _crc16_update(crc, buffer[i]);

  return crc;
}


static void _set_baud(uint16_t bsel, uint8_t bscale) {
  HUANYANG_PORT.BAUDCTRLB = (uint8_t)((bscale << 4) | (bsel >> 8));
  HUANYANG_PORT.BAUDCTRLA = bsel;
}


static void _set_write(bool x) {SET_PIN(RS485_RW_PIN, x);}


static void _set_dre_interrupt(bool enable) {
  if (enable) HUANYANG_PORT.CTRLA |= USART_DREINTLVL_MED_gc;
  else HUANYANG_PORT.CTRLA &= ~USART_DREINTLVL_MED_gc;
}


static void _set_txc_interrupt(bool enable) {
  if (enable) HUANYANG_PORT.CTRLA |= USART_TXCINTLVL_MED_gc;
  else HUANYANG_PORT.CTRLA &= ~USART_TXCINTLVL_MED_gc;
}


static void _set_rxc_interrupt(bool enable) {
  if (enable) HUANYANG_PORT.CTRLA |= USART_RXCINTLVL_MED_gc;
  else HUANYANG_PORT.CTRLA &= ~USART_RXCINTLVL_MED_gc;
}


static void _set_command1(int code, uint8_t arg0) {
  ha.command_length = 4;
  ha.command[1] = code;
  ha.command[2] = 1;
  ha.command[3] = arg0;
}


static void _set_command2(int code, uint8_t arg0, uint8_t arg1) {
  ha.command_length = 5;
  ha.command[1] = code;
  ha.command[2] = 2;
  ha.command[3] = arg0;
  ha.command[4] = arg1;
}


static void _set_command3(int code, uint8_t arg0, uint8_t arg1, uint8_t arg2) {
  ha.command_length = 6;
  ha.command[1] = code;
  ha.command[2] = 3;
  ha.command[3] = arg0;
  ha.command[4] = arg1;
  ha.command[5] = arg2;
}


static int _response_length(int code) {
  switch (code) {
  case HUANYANG_FUNC_READ:  return 8;
  case HUANYANG_FUNC_WRITE: return 8;
  case HUANYANG_CTRL_WRITE: return 6;
  case HUANYANG_CTRL_READ:  return 8;
  case HUANYANG_FREQ_WRITE: return 7;
  default: return -1;
  }
}


static bool _update(int index) {
  // Read response
  switch (index) {
  case 1: ha.status = ha.response[5]; break;
  case 2: ha.max_freq = FUNC_RESPONSE(ha.response) * 0.01; break;
  case 3: ha.min_freq = FUNC_RESPONSE(ha.response) * 0.01; break;
  case 4: ha.rated_rpm = FUNC_RESPONSE(ha.response); break;
  default: break;
  }

  // Setup next command
  uint8_t var;
  switch (index) {
  case 0: { // Update direction
    uint8_t state = HUANYANG_STOP;
    if (0 < ha.speed) state = HUANYANG_FORWARD;
    else if (ha.speed < 0) state = HUANYANG_REVERSE;

    _set_command1(HUANYANG_CTRL_WRITE, state);

    return true;
  }

  case 1: var = HUANYANG_MAX_FREQ; break;
  case 2: var = HUANYANG_MIN_FREQ; break;
  case 3: var = HUANYANG_RATED_RPM; break;

  case 4: { // Update freqency
    // Compute frequency in Hz
    float freq = fabs(ha.speed * 50 / ha.rated_rpm);

    // Clamp frequency
    if (ha.max_freq < freq) freq = ha.max_freq;
    if (freq < ha.min_freq) freq = ha.min_freq;

    // Frequency write command
    uint16_t f = freq * 100;
    _set_command2(HUANYANG_FREQ_WRITE, f >> 8, f);

    return true;
  }

  default:
    report_request();
    return false;
  }

  _set_command3(HUANYANG_FUNC_READ, var, 0, 0);

  return true;
}


static bool _query_status(int index) {
  // Read response
  switch (index) {
  case 1: ha.actual_freq = CTRL_STATUS_RESPONSE(ha.response) * 0.01; break;
  case 2: ha.actual_current = CTRL_STATUS_RESPONSE(ha.response) * 0.1; break;
  case 3: ha.actual_rpm = CTRL_STATUS_RESPONSE(ha.response); break;
  case 4: ha.temperature = CTRL_STATUS_RESPONSE(ha.response); break;
  default: break;
  }

  // Setup next command
  uint8_t var;
  switch (index) {
  case 0: var = HUANYANG_ACTUAL_FREQ; break;
  case 1: var = HUANYANG_ACTUAL_CURRENT; break;
  case 2: var = HUANYANG_ACTUAL_RPM; break;
  case 3: var = HUANYANG_TEMPERATURE; break;
  default:
    report_request();
    return false;
  }

  _set_command1(HUANYANG_CTRL_READ, var);

  return true;
}


static void _next_command();


static void _next_state() {
  if (ha.changed) {
    ha.next_command_cb = _update;
    ha.changed = false;

  } else ha.next_command_cb = _query_status;

  _next_command();
}


static bool _check_response() {
  // Check CRC
  uint16_t computed = _crc16(ha.response, ha.response_length - 2);
  uint16_t expected = (ha.response[ha.response_length - 1] << 8) |
    ha.response[ha.response_length - 2];

  if (computed != expected) {
    STATUS_WARNING("huanyang: invalid CRC, expected=0x%04u got=0x%04u",
                   expected, computed);
    return false;
  }

  // Check return function code matches sent
  if (ha.command[1] != ha.response[1]) {
    STATUS_WARNING("huanyang: invalid function code, expected=%u got=%u",
                   ha.command[2], ha.response[2]);
    return false;
  }

  return true;
}


static void _next_command() {
  if (ha.next_command_cb && ha.next_command_cb(ha.command_index++)) {
    ha.response_length = _response_length(ha.command[1]);

    ha.command[0] = ha.id;

    uint16_t crc = _crc16(ha.command, ha.command_length);
    ha.command[ha.command_length++] = crc;
    ha.command[ha.command_length++] = crc >> 8;

    _set_dre_interrupt(true);

    return;
  }

  ha.command_index = 0;
  _next_state();
}


static void _retry_command() {
  ha.last = 0;
  ha.current_offset = 0;
  ha.retry++;

  _set_write(true); // RS485 write mode

  _set_txc_interrupt(false);
  _set_rxc_interrupt(false);
  _set_dre_interrupt(true);

  if (ha.debug) STATUS_DEBUG("huanyang: retry %d", ha.retry);
}


// Data register empty interrupt
ISR(HUANYANG_DRE_vect) {
  HUANYANG_PORT.DATA = ha.command[ha.current_offset++];

  if (ha.current_offset == ha.command_length) {
    _set_dre_interrupt(false);
    _set_txc_interrupt(true);
    ha.current_offset = 0;
  }
}


/// Transmit complete interrupt
ISR(HUANYANG_TXC_vect) {
  _set_txc_interrupt(false);
  _set_rxc_interrupt(true);
  _set_write(false); // RS485 read mode
  ha.last = rtc_get_time(); // Set timeout timer
}


// Data received interrupt
ISR(HUANYANG_RXC_vect) {
  ha.response[ha.current_offset++] = USARTD1.DATA;

  if (ha.current_offset == ha.response_length) {
    _set_rxc_interrupt(false);
    ha.current_offset = 0;
    _set_write(true); // RS485 write mode

    if (_check_response())  {
      ha.last = 0; // Clear timeout timer
      ha.retry = 0; // Reset retry counter
      ha.connected = true;

      _next_command();
    }
  }
}


void huanyang_init() {
  PR.PRPD &= ~PR_USART1_bm; // Disable power reduction

  DIRCLR_PIN(RS485_RO_PIN); // Input
  OUTSET_PIN(RS485_DI_PIN); // High
  DIRSET_PIN(RS485_DI_PIN); // Output
  OUTSET_PIN(RS485_RW_PIN); // High
  DIRSET_PIN(RS485_RW_PIN); // Output

  _set_baud(3325, 0b1101); // 9600 @ 32MHz with 2x USART

  // No parity, 8 data bits, 1 stop bit
  USARTD1.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc |
    USART_CHSIZE_8BIT_gc;

  // Configure receiver and transmitter
  USARTD1.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;

  ha.id = HUANYANG_ID;
  huanyang_reset();
}


void huanyang_set(float speed) {
  if (ha.speed != speed) {
    if (ha.debug) STATUS_DEBUG("huanyang: speed=%0.2f", speed);
    ha.speed = speed;
    ha.changed = true;
  }
}


void huanyang_reset() {
  _set_dre_interrupt(false);
  _set_txc_interrupt(false);
  _set_rxc_interrupt(false);
  _set_write(true); // RS485 write mode

  // Flush USART
  uint8_t x = USARTD1.DATA;
  x = USARTD1.STATUS;
  x = x;

  // Save settings
  uint8_t id = ha.id;
  float speed = ha.speed;
  bool debug = ha.debug;

  // Clear state
  memset(&ha, 0, sizeof(ha));

  // Restore settings
  ha.id = id;
  ha.speed = speed;
  ha.debug = debug;
  ha.changed = true;

  _next_state();
}


void huanyang_rtc_callback() {
  if (ha.last && rtc_expired(ha.last + HUANYANG_TIMEOUT)) {
    if (ha.retry < HUANYANG_RETRIES) _retry_command();
    else {
      if (ha.debug) STATUS_DEBUG("huanyang: timedout");

      if (ha.debug && ha.current_offset) {
        const uint8_t buf_len = 8 * 2 + 1;
        char sent[buf_len];
        char received[buf_len];

        uint8_t i;
        for (i = 0; i < ha.command_length; i++)
          sprintf(sent + i * 2, "%02x", ha.command[i]);
        sent[i * 2] = 0;

        for (i = 0; i < ha.current_offset; i++)
          sprintf(received + i * 2, "%02x", ha.response[i]);
        received[i * 2] = 0;

        STATUS_DEBUG("huanyang: sent 0x%s received 0x%s expected %u bytes",
                     sent, received, ha.response_length);
      }

      huanyang_reset();
    }
  }
}


void huanyang_stop() {
  huanyang_set(0);
  huanyang_reset();
}


uint8_t get_huanyang_id() {return ha.id;}
void set_huanyang_id(uint8_t value) {ha.id = value;}
bool get_huanyang_debug() {return ha.debug;}
void set_huanyang_debug(bool value) {ha.debug = value;}
bool get_huanyang_connected() {return ha.connected;}
float get_huanyang_freq() {return ha.actual_freq;}
float get_huanyang_current() {return ha.actual_current;}
uint16_t get_huanyang_rpm() {return ha.actual_rpm;}
uint16_t get_huanyang_temp() {return ha.temperature;}
float get_huanyang_max_freq() {return ha.max_freq;}
float get_huanyang_min_freq() {return ha.min_freq;}
uint16_t get_huanyang_rated_rpm() {return ha.rated_rpm;}
float get_huanyang_status() {return ha.status;}
