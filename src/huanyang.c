/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
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
  HUANYANG_DC_VOLTAGE,
  HUANYANG_AC_VOLTAGE,
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
  cmSpindleMode_t mode;
  float speed;

  float actual_freq;
  float actual_current;
  uint16_t actual_rpm;
  uint16_t dc_voltage;
  uint16_t ac_voltage;
  uint16_t temperature;

  float max_freq;
  float min_freq;
  uint16_t rated_rpm;

  uint8_t status;
} huanyang_t;


static huanyang_t ha = {};


#define CTRL_STATUS_RESPONSE(R) ((uint16_t)R[4] << 8 | R[5])
#define FUNC_RESPONSE(R) (R[2] == 2 ? R[4] : ((uint16_t)R[4] << 8 | R[5]))


static uint16_t _crc16(const uint8_t *buffer, unsigned length) {
  uint16_t crc = 0xffff;

  for (unsigned i = 0; i < length; i++)
    crc = _crc16_update(crc, buffer[i]);

  return crc;
}


static void _set_baud(uint16_t bsel, uint8_t bscale) {
  USARTD1.BAUDCTRLB = (uint8_t)((bscale << 4) | (bsel >> 8));
  USARTD1.BAUDCTRLA = bsel;
}


static void _set_write(bool x) {
  if (x)  {
    PORTB.OUTSET = 1 << 4; // High
    PORTB.OUTSET = 1 << 5; // High

  } else {
    PORTB.OUTCLR = 1 << 4; // Low
    PORTB.OUTCLR = 1 << 5; // Low
  }
}


static void _set_dre_interrupt(bool enable) {
  if (enable) USARTD1.CTRLA |= USART_DREINTLVL_MED_gc;
  else USARTD1.CTRLA &= ~USART_DREINTLVL_MED_gc;
}


static void _set_txc_interrupt(bool enable) {
  if (enable) USARTD1.CTRLA |= USART_TXCINTLVL_MED_gc;
  else USARTD1.CTRLA &= ~USART_TXCINTLVL_MED_gc;
}


static void _set_rxc_interrupt(bool enable) {
  if (enable) USARTD1.CTRLA |= USART_RXCINTLVL_MED_gc;
  else USARTD1.CTRLA &= ~USART_RXCINTLVL_MED_gc;
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
  case 0: { // Update mode
    uint8_t state;
    switch (ha.mode) {
    case SPINDLE_CW: state = HUANYANG_FORWARD; break;
    case SPINDLE_CCW: state = HUANYANG_REVERSE; break;
    default: state = HUANYANG_STOP; break;
    }

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

  default: return false;
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
  case 4: ha.dc_voltage = CTRL_STATUS_RESPONSE(ha.response); break;
  case 5: ha.ac_voltage = CTRL_STATUS_RESPONSE(ha.response); break;
  case 6: ha.temperature = CTRL_STATUS_RESPONSE(ha.response); break;
  default: break;
  }

  // Setup next command
  uint8_t var;
  switch (index) {
  case 0: var = HUANYANG_ACTUAL_FREQ; break;
  case 1: var = HUANYANG_ACTUAL_CURRENT; break;
  case 2: var = HUANYANG_ACTUAL_RPM; break;
  case 3: var = HUANYANG_DC_VOLTAGE; break;
  case 4: var = HUANYANG_AC_VOLTAGE; break;
  case 5: var = HUANYANG_TEMPERATURE; break;
  default: return false;
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
    printf(PSTR("huanyang: invalid CRC, expected=0x%04u got=0x%04u\n"),
           expected, computed);
    return false;
  }

  // Check return function code matches sent
  if (ha.command[1] != ha.response[1]) {
    printf_P(PSTR("huanyang: invalid function code, expected=%u got=%u\n"),
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

  if (ha.debug) printf_P(PSTR("huanyang: retry %d\n"), ha.retry);
}


// Data register empty interrupt
ISR(USARTD1_DRE_vect) {
  USARTD1.DATA = ha.command[ha.current_offset++];

  if (ha.current_offset == ha.command_length) {
    _set_dre_interrupt(false);
    _set_txc_interrupt(true);
    ha.current_offset = 0;
  }
}


/// Transmit complete interrupt
ISR(USARTD1_TXC_vect) {
  _set_txc_interrupt(false);
  _set_rxc_interrupt(true);
  _set_write(false); // RS485 read mode
  ha.last = rtc_get_time(); // Set timeout timer
}


// Data received interrupt
ISR(USARTD1_RXC_vect) {
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

  // rs485_ro
  PORTD.DIRCLR = 1 << 6; // Input

  // rs485_di
  PORTD.OUTSET = 1 << 7; // High
  PORTD.DIRSET = 1 << 7; // Output

  // rs485_re
  PORTB.OUTSET = 1 << 4; // High
  PORTB.DIRSET = 1 << 4; // Output

  // rs485_de
  PORTB.OUTSET = 1 << 5; // High
  PORTB.DIRSET = 1 << 5; // Output

  _set_baud(3325, 0b1101); // 9600 @ 32MHz with 2x USART

  // No parity, 8 data bits, 1 stop bit
  USARTD1.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc |
    USART_CHSIZE_8BIT_gc;

  // Configure receiver and transmitter
  USARTD1.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;

  ha.id = HUANYANG_ID;
  huanyang_reset();
}


void huanyang_set(cmSpindleMode_t mode, float speed) {
  if (ha.mode != mode || ha.speed != speed) {
    if (ha.debug)
      printf_P(PSTR("huanyang: mode=%d, speed=%0.2f\n"), mode, speed);

    ha.mode = mode;
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
  cmSpindleMode_t mode = ha.mode;
  float speed = ha.speed;
  bool debug = ha.debug;

  // Clear state
  memset(&ha, 0, sizeof(ha));

  // Restore settings
  ha.id = id;
  ha.mode = mode;
  ha.speed = speed;
  ha.debug = debug;
  ha.changed = true;

  _next_state();
}


void huanyang_rtc_callback() {
  if (ha.last && HUANYANG_TIMEOUT < rtc_get_time() - ha.last) {
    if (ha.retry < HUANYANG_RETRIES) _retry_command();
    else {
      if (ha.debug) printf_P(PSTR("huanyang: timedout\n"));

      if (ha.debug && ha.current_offset) {
        printf_P(PSTR("huanyang: sent 0x"));

        for (uint8_t i = 0; i < ha.command_length; i++)
          printf("%02x", ha.command[i]);

        printf_P(PSTR(" received 0x"));
        for (uint8_t i = 0; i < ha.current_offset; i++)
          printf("%02x", ha.response[i]);

        printf_P(PSTR(" expected %u bytes"), ha.response_length);

        putchar('\n');
      }

      huanyang_reset();
    }
  }
}



uint8_t get_huanyang_id(int index) {
  return ha.id;
}

void set_huanyang_id(int index, uint8_t value) {
  ha.id = value;
}


bool get_huanyang_debug(int index) {
  return ha.debug;
}

void set_huanyang_debug(int index, uint8_t value) {
  ha.debug = value;
}


bool get_huanyang_connected(int index) {
  return ha.connected;
}


float get_huanyang_freq(int index) {
  return ha.actual_freq;
}


float get_huanyang_current(int index) {
  return ha.actual_current;
}


float get_huanyang_rpm(int index) {
  return ha.actual_rpm;
}


float get_huanyang_dcv(int index) {
  return ha.dc_voltage;
}


float get_huanyang_acv(int index) {
  return ha.ac_voltage;
}


float get_huanyang_temp(int index) {
  return ha.temperature;
}


float get_huanyang_max_freq(int index) {
  return ha.max_freq;
}


float get_huanyang_min_freq(int index) {
  return ha.min_freq;
}


uint16_t get_huanyang_rated_rpm(int index) {
  return ha.rated_rpm;
}


float get_huanyang_status(int index) {
  return ha.status;
}
