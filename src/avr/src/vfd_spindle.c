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

#include "vfd_spindle.h"
#include "modbus.h"
#include "rtc.h"
#include "config.h"
#include "pgmspace.h"

#include <string.h>
#include <math.h>
#include <stdint.h>


typedef enum {
  REG_DISABLED,

  REG_CONNECT_WRITE,

  REG_MAX_FREQ_READ,
  REG_MAX_FREQ_FIXED,

  REG_FREQ_SET,
  REG_FREQ_SIGN_SET,

  REG_STOP_WRITE,
  REG_FWD_WRITE,
  REG_REV_WRITE,

  REG_FREQ_READ,
  REG_FREQ_SIGN_READ,

  REG_DISCONNECT_WRITE,
} vfd_reg_type_t;


typedef struct {
  vfd_reg_type_t type;
  uint16_t addr;
  uint16_t value;
} vfd_reg_t;


#define P(H, L) ((H) << 8 | (L))


const vfd_reg_t yl600_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,    P(7, 8), 1}, // P07_08_FREQ_SOURCE_1
  {REG_CONNECT_WRITE,    P(0, 1), 1}, // P00_01_START_STOP_SOURCE
  {REG_MAX_FREQ_READ,    P(0, 4), 0}, // P00_04_HIGEST_OUTPUT_FREQ
  {REG_FREQ_SET,         P(7, 0), 0}, // P07_00_FREQ_1
//  {REG_STOP_WRITE,       P(?, ?), ?}, //
//  {REG_FWD_WRITE,        P(1, 0), ?}, // P01_00_DIRECTION
//  {REG_REV_WRITE,        P(?, ?), ?}, //
  {REG_FREQ_READ,        P(0, 0), 0}, // P00_00_MAIN_FREQ
  {REG_DISCONNECT_WRITE, P(0, 1), 0}, // P00_01_START_STOP_SOURCE
  {REG_DISCONNECT_WRITE, P(7, 8), 0}, // P07_08_FREQ_SOURCE_1
  {REG_DISABLED},
};


// NOTE, Modbus reg = AC Tech reg + 1
const vfd_reg_t ac_tech_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,    49,   19}, // Password unlock
  {REG_CONNECT_WRITE,     2,  512}, // Manual mode
  {REG_MAX_FREQ_READ,    63,    0}, // Max frequency
  {REG_FREQ_SET,         41,    0}, // Frequency
  {REG_STOP_WRITE,        2,    4}, // Stop drive
  {REG_FWD_WRITE,         2,  128}, // Forward
  {REG_FWD_WRITE,         2,    8}, // Start drive
  {REG_REV_WRITE,         2,   64}, // Reverse
  {REG_REV_WRITE,         2,    8}, // Start drive
  {REG_FREQ_READ,        26,    0}, // Actual speed
  {REG_DISCONNECT_WRITE,  2,    2}, // Lock controls and parameters
  {REG_DISABLED},
};


static vfd_reg_t regs[VFDREG];
static vfd_reg_t custom_regs[VFDREG];

static struct {
  vfd_reg_type_t state;
  int8_t reg;
  bool changed;
  bool shutdown;

  float speed;
  uint16_t max_freq;
  float actual_speed;

  uint32_t wait;
  deinit_cb_t deinit_cb;
} vfd;


static void _disconnected() {
  modbus_deinit();
  if (vfd.deinit_cb) vfd.deinit_cb();
  vfd.deinit_cb = 0;
}


static bool _next_state() {
  switch (vfd.state) {
  case REG_FREQ_SIGN_SET:
    if (vfd.speed < 0) vfd.state = REG_REV_WRITE;
    else if (0 < vfd.speed) vfd.state = REG_FWD_WRITE;
    else vfd.state = REG_STOP_WRITE;
    break;

  case REG_STOP_WRITE: case REG_FWD_WRITE: case REG_REV_WRITE:
    vfd.state = REG_FREQ_READ;
    break;

  case REG_FREQ_SIGN_READ:
    if (vfd.shutdown) vfd.state = REG_DISCONNECT_WRITE;

    else if (vfd.changed) {
      // Update frequency and state
      vfd.changed = false;
      vfd.state = REG_MAX_FREQ_READ;

    } else {
      // Continue querying after delay
      vfd.state = REG_FREQ_READ;
      vfd.wait = rtc_get_time() + VFD_QUERY_DELAY;
      return false;
    }
    break;

  case REG_DISCONNECT_WRITE:
    _disconnected();
    return false;

  default:
    vfd.state = (vfd_reg_type_t)(vfd.state + 1);
  }

  return true;
}


static bool _exec_command();


static void _next_reg() {
  while (true) {
    vfd.reg++;

    if (vfd.reg == VFDREG) {
      vfd.reg = -1;
      if (!_next_state()) break;

    } else if (regs[vfd.reg].type == vfd.state && _exec_command()) break;
  }
}


static void _connect() {
  vfd.state = REG_CONNECT_WRITE;
  vfd.reg = -1;
  _next_reg();
}


static void _modbus_cb(bool ok, uint16_t addr, uint16_t value) {
  // Handle error
  if (!ok) {
    if (vfd.shutdown) _disconnected();
    else _connect();
    return;
  }

  // Handle read result
  switch (regs[vfd.reg].type) {
  case REG_MAX_FREQ_READ: vfd.max_freq = value; break;
  case REG_FREQ_READ: vfd.actual_speed = value / (float)vfd.max_freq; break;

  case REG_FREQ_SIGN_READ:
    vfd.actual_speed = (int16_t)value / (float)vfd.max_freq;
    break;

  default: break;
  }

  // Next
  _next_reg();
}


static bool _exec_command() {
  if (vfd.wait) return true;

  vfd_reg_t reg = regs[vfd.reg];
  bool read = false;
  bool write = false;

  switch (reg.type) {
  case REG_DISABLED: break;

  case REG_MAX_FREQ_FIXED: vfd.max_freq = reg.value; break;

  case REG_FREQ_SET:
    write = true;
    reg.value = fabs(vfd.speed) * vfd.max_freq;
    break;

  case REG_FREQ_SIGN_SET:
    write = true;
    reg.value = vfd.speed * vfd.max_freq;
    break;

  case REG_CONNECT_WRITE:
  case REG_STOP_WRITE:
  case REG_FWD_WRITE:
  case REG_REV_WRITE:
  case REG_DISCONNECT_WRITE:
    write = true;
    break;

  case REG_FREQ_READ:
  case REG_FREQ_SIGN_READ:
  case REG_MAX_FREQ_READ:
    read = true;
    break;
  }

  if (read) modbus_read(reg.addr, _modbus_cb);
  else if (write) modbus_write(reg.addr, reg.value, _modbus_cb);
  else return false;

  return true;
}


static void _load(const vfd_reg_t *_regs) {
  memset(&regs, 0, sizeof(regs));

  for (int i = 0; i < VFDREG; i++) {
    regs[i].type = (vfd_reg_type_t)pgm_read_byte(&_regs[i].type);
    if (!regs[i].type) break;
    regs[i].addr = pgm_read_word(&_regs[i].addr);
    regs[i].value = pgm_read_word(&_regs[i].value);
  }
}


void vfd_spindle_init() {
  memset(&vfd, 0, sizeof(vfd));
  modbus_init();

  switch (spindle_get_type()) {
  case SPINDLE_TYPE_CUSTOM:  memcpy(regs, custom_regs, sizeof(regs)); break;
  case SPINDLE_TYPE_YL600:   _load(yl600_regs);   break;
  case SPINDLE_TYPE_AC_TECH: _load(ac_tech_regs); break;
  default: break;
  }

  _connect();
}


void vfd_spindle_deinit(deinit_cb_t cb) {
  vfd.shutdown = true;
  vfd.deinit_cb = cb;
}


void vfd_spindle_set(float speed) {
  if (vfd.speed != speed) {
    vfd.speed = speed;
    vfd.changed = true;
  }
}


float vfd_spindle_get() {return vfd.actual_speed;}
void vfd_spindle_stop() {vfd_spindle_set(0);}


void vfd_spindle_rtc_callback() {
  if (!vfd.wait || !rtc_expired(vfd.wait)) return;
  vfd.wait = 0;
  _next_reg();
}


// Variable callbacks
uint16_t get_vfd_max_freq() {return vfd.max_freq;}
void set_vfd_max_freq(uint16_t max_freq) {vfd.max_freq = max_freq;}
uint8_t get_vfd_reg_type(int reg) {return regs[reg].type;}


void set_vfd_reg_type(int reg, uint8_t type) {
  custom_regs[reg].type = (vfd_reg_type_t)type;
  if (spindle_get_type() == SPINDLE_TYPE_CUSTOM)
    regs[reg].type = custom_regs[reg].type;
  vfd.changed = true;
}


uint16_t get_vfd_reg_addr(int reg) {return regs[reg].addr;}


void set_vfd_reg_addr(int reg, uint16_t addr) {
  custom_regs[reg].addr = addr;
  if (spindle_get_type() == SPINDLE_TYPE_CUSTOM)
    regs[reg].addr = custom_regs[reg].addr;
  vfd.changed = true;
}


uint16_t get_vfd_reg_val(int reg) {return regs[reg].value;}


void set_vfd_reg_val(int reg, uint16_t value) {
  custom_regs[reg].value = value;
  if (spindle_get_type() == SPINDLE_TYPE_CUSTOM)
    regs[reg].value = custom_regs[reg].value;
  vfd.changed = true;
}
