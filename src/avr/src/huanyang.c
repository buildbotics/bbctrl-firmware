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

#include "huanyang.h"
#include "config.h"
#include "modbus.h"
#include "report.h"

#include <string.h>
#include <math.h>


/*
  Huanyang is not quite Modbus compliant.

  Message format is:

    [id][func][length][data][checksum]

  Where:

     id       - 1-byte Peer ID
     func     - 1-byte One of hy_func_t
     length   - 1-byte Length data in bytes
     data     - length bytes - Command arguments
     checksum - 16-bit CRC: x^16 + x^15 + x^2 + 1 (0xa001) initial: 0xffff
*/


// See VFD manual pg56 3.1.3
typedef enum {
  HUANYANG_FUNC_READ = 1, // [len=1][hy_addr_t]
  HUANYANG_FUNC_WRITE,    // [len=3][hy_addr_t][data]
  HUANYANG_CTRL_WRITE,    // [len=1][hy_ctrl_state_t]
  HUANYANG_CTRL_READ,     // [len=1][hy_ctrl_addr_t]
  HUANYANG_FREQ_WRITE,    // [len=2][freq]
  HUANYANG_RESERVED_1,
  HUANYANG_RESERVED_2,
  HUANYANG_LOOP_TEST,
} hy_func_t;


// Sent in HUANYANG_CTRL_WRITE
// See VFD manual pg57 3.1.3.c
typedef enum {
  HUANYANG_RUN         = 1 << 0,
  HUANYANG_FORWARD     = 1 << 1,
  HUANYANG_REVERSE     = 1 << 2,
  HUANYANG_STOP        = 1 << 3,
  HUANYANG_REV_FWD     = 1 << 4,
  HUANYANG_JOG         = 1 << 5,
  HUANYANG_JOG_FORWARD = 1 << 6,
  HUANYANG_JOG_REVERSE = 1 << 7,
} hy_ctrl_state_t;


// Returned by HUANYANG_CTRL_WRITE
// See VFD manual pg57 3.1.3.c
typedef enum {
  HUANYANG_STATUS_RUN         = 1 << 0,
  HUANYANG_STATUS_JOG         = 1 << 1,
  HUANYANG_STATUS_COMMAND_REV = 1 << 2,
  HUANYANG_STATUS_RUNNING     = 1 << 3,
  HUANYANG_STATUS_JOGGING     = 1 << 4,
  HUANYANG_STATUS_JOGGING_REV = 1 << 5,
  HUANYANG_STATUS_BRAKING     = 1 << 6,
  HUANYANG_STATUS_TRACK_START = 1 << 7,
} hy_ctrl_status_t;


// Sent in HUANYANG_CTRL_READ
// See VFD manual pg57 3.1.3.d
typedef enum {
  HUANYANG_TARGET_FREQ,
  HUANYANG_ACTUAL_FREQ,
  HUANYANG_ACTUAL_CURRENT,
  HUANYANG_ACTUAL_RPM,
  HUANYANG_DCV,
  HUANYANG_ACV,
  HUANYANG_COUNTER,
  HUANYANG_TEMPERATURE,
} hy_ctrl_addr_t;


static struct {
  uint8_t id;

  uint8_t state;
  hy_func_t func;
  uint8_t data[4];
  uint8_t bytes;
  uint8_t response;

  bool shutdown;
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
} hy = {1}; // Default ID


static void _func_read(hy_addr_t addr) {
  hy.func     = HUANYANG_FUNC_READ;
  hy.bytes    = 2;
  hy.response = 4;
  hy.data[0]  = 1;
  hy.data[1]  = addr;
}


static void _ctrl_write(hy_ctrl_state_t state) {
  hy.func     = HUANYANG_CTRL_WRITE;
  hy.bytes    = 2;
  hy.response = 2;
  hy.data[0]  = 1;
  hy.data[1]  = state;
}


static void _ctrl_read(hy_ctrl_addr_t addr) {
  hy.func     = HUANYANG_CTRL_READ;
  hy.bytes    = 2;
  hy.response = 4;
  hy.data[0]  = 1;
  hy.data[1]  = addr;
}


static void _freq_write(uint16_t freq) {
  hy.func     = HUANYANG_FREQ_WRITE;
  hy.bytes    = 3;
  hy.response = 3;
  hy.data[0]  = 2;
  hy.data[1]  = freq >> 8;
  hy.data[2]  = freq;
}


static void _func_read_response(hy_addr_t addr, uint16_t value) {
  switch (addr) {
  case HY_PD005_MAX_FREQUENCY:         hy.max_freq  = value * 0.01; break;
  case HY_PD011_FREQUENCY_LOWER_LIMIT: hy.min_freq  = value * 0.01; break;
  case HY_PD144_RATED_MOTOR_RPM:       hy.rated_rpm = value;        break;
  default: break;
  }
}


static void _ctrl_read_response(hy_ctrl_addr_t addr, uint16_t value) {
  switch (addr) {
  case HUANYANG_ACTUAL_FREQ:    hy.actual_freq    = value * 0.01; break;
  case HUANYANG_ACTUAL_CURRENT: hy.actual_current = value * 0.01; break;
  case HUANYANG_ACTUAL_RPM:     hy.actual_rpm     = value;        break;
  case HUANYANG_TEMPERATURE:    hy.temperature    = value;        break;
  default: break;
  }
}


static uint16_t _read_word(const uint8_t *data) {
  return (uint16_t)data[0] << 8 | data[1];
}


static void _handle_response(hy_func_t func, const uint8_t *data) {
  switch (func) {
  case HUANYANG_FUNC_READ:
    _func_read_response((hy_addr_t)*data, _read_word(data + 1));
    break;

  case HUANYANG_FUNC_WRITE: break;
  case HUANYANG_CTRL_WRITE: hy.status = *data; break;

  case HUANYANG_CTRL_READ:
    _ctrl_read_response((hy_ctrl_addr_t)*data, _read_word(data + 1));
    break;

  case HUANYANG_FREQ_WRITE: break;
  default: break;
  }
}


static void _next_command();


static void _reset(bool halt) {
  // Save settings
  uint8_t id = hy.id;
  float speed = hy.speed;

  // Clear state
  memset(&hy, 0, sizeof(hy));

  // Restore settings
  hy.id = id;
  hy.speed = speed;
  hy.changed = true;

  if (!halt) _next_command();
}


static void _modbus_cb(uint8_t slave, uint8_t func, uint8_t bytes,
                       const uint8_t *data) {
  if (!data) _reset(true);

  else if (bytes == *data + 1) {
    _handle_response((hy_func_t)func, data + 1);

    if (func == HUANYANG_CTRL_WRITE && hy.shutdown) {
      _reset(true);
      modbus_deinit();
      return;
    }

    // Next command
    if (++hy.state == 9) {
      hy.state = 5;
      report_request();
    }

    // Update freq
    if (hy.shutdown || (hy.changed && 4 < hy.state)) hy.state = 0;
  }

  _next_command();
}


static void _next_command() {
  switch (hy.state) {
  case 0: { // Update direction
    hy_ctrl_state_t state = HUANYANG_STOP;
    if (!hy.shutdown) {
      if (0 < hy.speed) state = HUANYANG_RUN;
      else if (hy.speed < 0)
        state = (hy_ctrl_state_t)(HUANYANG_RUN | HUANYANG_REV_FWD);
    }

    _ctrl_write(state);
    break;
  }

  case 1: _func_read(HY_PD005_MAX_FREQUENCY); break;
  case 2: _func_read(HY_PD011_FREQUENCY_LOWER_LIMIT); break;
  case 3: _func_read(HY_PD144_RATED_MOTOR_RPM); break;

  case 4: { // Update freqency
    // Compute frequency in Hz
    float freq = fabs(hy.speed * 50 / hy.rated_rpm);

    // Clamp frequency
    if (hy.max_freq < freq) freq = hy.max_freq;
    if (freq < hy.min_freq) freq = hy.min_freq;

    // Frequency write command
    _freq_write(freq * 100);
    hy.changed = false;
    break;
  }

  case 5: _ctrl_read(HUANYANG_ACTUAL_FREQ);    break;
  case 6: _ctrl_read(HUANYANG_ACTUAL_CURRENT); break;
  case 7: _ctrl_read(HUANYANG_ACTUAL_RPM);     break;
  case 8: _ctrl_read(HUANYANG_TEMPERATURE);    break;
  }

  // Send command
  modbus_func(hy.id, hy.func, hy.bytes, hy.data, hy.response, _modbus_cb);
}


void hy_init() {
  modbus_init();
  _reset(false);
}


void hy_deinit() {hy.shutdown = true;}


void hy_set(float speed) {
  if (hy.speed != speed) {
    hy.speed = speed;
    hy.changed = true;
  }
}


void hy_stop() {
  hy_set(0);
  _reset(false);
}


uint8_t get_hy_id() {return hy.id;}
void set_hy_id(uint8_t value) {hy.id = value;}
float get_hy_freq() {return hy.actual_freq;}
float get_hy_current() {return hy.actual_current;}
uint16_t get_hy_rpm() {return hy.actual_rpm;}
uint16_t get_hy_temp() {return hy.temperature;}
float get_hy_max_freq() {return hy.max_freq;}
float get_hy_min_freq() {return hy.min_freq;}
uint16_t get_hy_rated_rpm() {return hy.rated_rpm;}
float get_hy_status() {return hy.status;}
