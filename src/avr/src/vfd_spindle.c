/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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

#include "vfd_spindle.h"
#include "modbus.h"
#include "rtc.h"
#include "config.h"
#include "pgmspace.h"

#include <util/atomic.h>

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
  REG_FREQ_ACTECH_READ,

  REG_STATUS_READ,

  REG_DISCONNECT_WRITE,
} vfd_reg_type_t;


typedef struct {
  vfd_reg_type_t type;
  uint16_t addr;
  uint16_t value;
  uint8_t fails;
} vfd_reg_t;


#define P(H, L) ((H) << 8 | (L))


// NOTE, Modbus reg = AC Tech reg + 1
const vfd_reg_t ac_tech_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,    48,   19}, // Password unlock
  {REG_CONNECT_WRITE,     1,  512}, // Manual mode
  {REG_MAX_FREQ_READ,    62,    0}, // Max frequency
  {REG_FREQ_SET,         40,    0}, // Frequency
  {REG_STOP_WRITE,        1,    4}, // Stop drive
  {REG_FWD_WRITE,         1,  128}, // Forward
  {REG_FWD_WRITE,         1,    8}, // Start drive
  {REG_REV_WRITE,         1,   64}, // Reverse
  {REG_REV_WRITE,         1,    8}, // Start drive
  {REG_FREQ_ACTECH_READ, 24,    0}, // Actual freq
  {REG_DISCONNECT_WRITE,  1,    4}, // Stop on disconnect
  {REG_DISCONNECT_WRITE,  1,    2}, // Lock controls and parameters
  {REG_DISABLED},
};


const vfd_reg_t nowforever_regs[] PROGMEM = {
  {REG_MAX_FREQ_READ,    0x007, 0}, // Max frequency
  {REG_FREQ_SET,         0x901, 0}, // Frequency
  {REG_STOP_WRITE,       0x900, 0}, // Stop drive
  {REG_FWD_WRITE,        0x900, 1}, // Forward
  {REG_REV_WRITE,        0x900, 3}, // Reverse
  {REG_FREQ_READ,        0x502, 0}, // Output freq
  {REG_STATUS_READ,      0x300, 0}, // Status
  {REG_DISCONNECT_WRITE, 0x900, 0}, // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t delta_vfd015m21a_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,    0x2002,  2}, // Reset fault
  {REG_MAX_FREQ_READ,         3,  0}, // Max frequency
  {REG_FREQ_SET,         0x2001,  0}, // Frequency
  {REG_STOP_WRITE,       0x2000,  1}, // Stop drive
  {REG_FWD_WRITE,        0x2000, 18}, // Forward
  {REG_REV_WRITE,        0x2000, 34}, // Reverse
  {REG_FREQ_READ,        0x2103,  0}, // Output freq
  {REG_STATUS_READ,      0x2100,  0}, // Status
  {REG_DISCONNECT_WRITE, 0x2000,  1}, // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t yl600_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,    0x2000, 128}, // Reset all errors
  {REG_MAX_FREQ_READ,    0x0004,   0}, // Max frequency
  {REG_FREQ_SET,         0x2001,   0}, // Frequency
  {REG_STOP_WRITE,       0x2000,   1}, // Stop drive
  {REG_FWD_WRITE,        0x2000,  18}, // Forward
  {REG_REV_WRITE,        0x2000,  34}, // Reverse
  {REG_FREQ_READ,        0x200b,   0}, // Output freq
  {REG_STATUS_READ,      0x2008,   0}, // Status
  {REG_DISCONNECT_WRITE, 0x2000,   1}, // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t fr_d700_regs[] PROGMEM = {
  {REG_MAX_FREQ_READ,  1000, 0}, // Max frequency
  {REG_FREQ_SET,         13, 0}, // Frequency
  {REG_STOP_WRITE,        8, 1}, // Stop drive
  {REG_FWD_WRITE,         8, 2}, // Forward
  {REG_REV_WRITE,         8, 4}, // Reverse
  {REG_FREQ_READ,       200, 0}, // Output freq
  {REG_DISCONNECT_WRITE,  8, 1}, // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t sunfar_e300_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,    0x1001, 32}, // Reset all errors
  {REG_MAX_FREQ_READ,    0xf004,  0}, // Max frequency F0.4
  {REG_FREQ_SET,         0x1002,  0}, // Frequency
  {REG_STOP_WRITE,       0x1001,  3}, // Stop drive
  {REG_FWD_WRITE,        0x1001,  1}, // Forward
  {REG_REV_WRITE,        0x1001,  2}, // Reverse
  {REG_FREQ_READ,        0xd000,  0}, // Output freq d.0
  {REG_STATUS_READ,      0x2000,  0}, // Status
  {REG_DISCONNECT_WRITE, 0x1001,  3}, // Stop on disconnect
  {REG_DISABLED},
};


// Register value for Modbus is one less than in the datasheet
const vfd_reg_t omron_mx2_regs[] PROGMEM = {
  {REG_CONNECT_WRITE,  0x1200,     3}, // A001 Frequency reference modbus
  {REG_CONNECT_WRITE,  0x1201,     3}, // A002 Run command modbus
  {REG_MAX_FREQ_FIXED,      0, 40000}, // TODO Want to use A004 max frequency
  {REG_FREQ_SET,       0x0001,     0}, // F001 Frequency
  {REG_STOP_WRITE,     0x1f00,     0}, // Stop drive
  {REG_FWD_WRITE,      0x1f00,     2}, // Forward
  {REG_REV_WRITE,      0x1f00,     6}, // Reverse
  {REG_FREQ_READ,      0x1001,     0}, // D001 Output freq
  {REG_STATUS_READ,    0x0004,     0}, // Status A
  {REG_DISABLED},
};


const vfd_reg_t v70_regs[] PROGMEM = {
  {REG_MAX_FREQ_READ,    0x0005, 0}, // Maximum operating frequency
  {REG_FREQ_SET,         0x0201, 0}, // Set frequency in 0.1Hz
  {REG_STOP_WRITE,       0x0200, 0}, // Stop
  {REG_FWD_WRITE,        0x0200, 1}, // Run forward
  {REG_REV_WRITE,        0x0200, 5}, // Run reverse
  {REG_FREQ_READ,        0x0220, 0}, // Read operating frequency
  {REG_STATUS_READ,      0x0210, 0}, // Read status
  {REG_DISCONNECT_WRITE, 0x0200, 0}, // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t dmm_dyn4_regs[] PROGMEM = {
  {REG_MAX_FREQ_FIXED,   0x00, 6000}, // Maximum operating frequency
  {REG_FREQ_SIGN_SET,    0x0e, 0},    // Set frequency in 0.1Hz
  {REG_STOP_WRITE,       0x0e, 0},    // Stop
  {REG_FREQ_SIGN_READ,   0x14, 0},    // Read operating frequency
  {REG_STATUS_READ,      0x02, 0},    // Read status
  {REG_DISCONNECT_WRITE, 0x0e, 0},    // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t galt_g200_regs[] PROGMEM = {
  {REG_MAX_FREQ_READ,    0x0003, 0}, // Read max operating frequency in 0.01Hz
  {REG_FREQ_SET,         0x2001, 0}, // Set frequency in 0.01Hz
  {REG_FREQ_READ,        0x3000, 0}, // Read frequency with 0.01Hz
  {REG_FWD_WRITE,        0x2000, 1}, // Run forward
  {REG_REV_WRITE,        0x2000, 2}, // Run reverse
  {REG_STOP_WRITE,       0x2000, 5}, // Stop
  {REG_STATUS_READ,      0x2100, 0}, // Read status
  {REG_DISCONNECT_WRITE, 0x2000, 5}, // Stop on disconnect
  {REG_DISABLED},
};


const vfd_reg_t teco_e510_regs[] PROGMEM = {
  {REG_MAX_FREQ_READ, 0x0102, 0}, // Read max frequency
  {REG_FREQ_SET,      0x2502, 0}, // Set frequency
  {REG_FREQ_READ,     0x2524, 0}, // Read frequency
  {REG_FWD_WRITE,     0x2501, 1}, // Run forward
  {REG_REV_WRITE,     0x2501, 2}, // Run reverse
  {REG_STOP_WRITE,    0x2501, 0}, // Stop
  {REG_STATUS_READ,   0x2520, 0}, // Read status
  {REG_DISABLED},
};


// Same as OMRON MX2
#define wj200_regs omron_mx2_regs


static vfd_reg_t regs[VFDREG];
static vfd_reg_t custom_regs[VFDREG];

static struct {
  vfd_reg_type_t state;
  int8_t reg;
  uint8_t read_count;
  bool changed;
  bool shutdown;

  float power;
  uint16_t max_freq;
  bool user_multi_write;
  float actual_power;
  uint16_t status;

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
  case REG_MAX_FREQ_FIXED:
    if (!vfd.power) vfd.state = REG_STOP_WRITE;
    else vfd.state = REG_FREQ_SET;
    break;

  case REG_FREQ_SIGN_SET:
    if (vfd.power < 0) vfd.state = REG_REV_WRITE;
    else if (0 < vfd.power) vfd.state = REG_FWD_WRITE;
    else vfd.state = REG_STOP_WRITE;
    break;

  case REG_STOP_WRITE: case REG_FWD_WRITE: case REG_REV_WRITE:
    vfd.state = REG_FREQ_READ;
    break;

  case REG_STATUS_READ:
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
      vfd.read_count = 0;
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
    if (regs[vfd.reg].fails < 255) regs[vfd.reg].fails++;
    if (vfd.shutdown) _disconnected();
    else _connect();
    return;
  }

  // Handle read result
  vfd.read_count++;

  switch (regs[vfd.reg].type) {
  case REG_MAX_FREQ_READ: vfd.max_freq = value; break;
  case REG_FREQ_READ: vfd.actual_power = value / (float)vfd.max_freq; break;

  case REG_FREQ_SIGN_READ:
    vfd.actual_power = (int16_t)value / (float)vfd.max_freq;
    break;

  case REG_FREQ_ACTECH_READ:
    if (vfd.read_count == 2) vfd.actual_power = value / (float)vfd.max_freq;
    if (vfd.read_count < 6) return;
    break;

  case REG_STATUS_READ: vfd.status = value; break;

  default: break;
  }

  // Next
  _next_reg();
}


static bool _use_multi_write() {
  switch (spindle_get_type()) {
  case SPINDLE_TYPE_CUSTOM:     return vfd.user_multi_write;
  case SPINDLE_TYPE_NOWFOREVER: return true;
  default:                      return false;
  }
}


static bool _exec_command() {
  if (vfd.wait) return true;

  vfd_reg_t reg = regs[vfd.reg];
  uint16_t words = 1;
  bool read = false;
  bool write = false;

  switch (reg.type) {
  case REG_DISABLED: break;

  case REG_MAX_FREQ_FIXED: vfd.max_freq = reg.value; break;

  case REG_FREQ_SET:
    write = true;
    reg.value = fabs(vfd.power) * vfd.max_freq;
    break;

  case REG_FREQ_SIGN_SET:
    write = true;
    reg.value = vfd.power * vfd.max_freq;
    break;

  case REG_CONNECT_WRITE:
  case REG_STOP_WRITE:
  case REG_FWD_WRITE:
  case REG_REV_WRITE:
  case REG_DISCONNECT_WRITE:
    write = true;
    break;

  case REG_FREQ_ACTECH_READ:
    words = 6;

  case REG_FREQ_READ:
  case REG_FREQ_SIGN_READ:
  case REG_MAX_FREQ_READ:
  case REG_STATUS_READ:
    read = true;
    break;
  }

  if (read) modbus_read(reg.addr, words, _modbus_cb);
  else if (write) (_use_multi_write() ? modbus_multi_write : modbus_write)
                    (reg.addr, reg.value, _modbus_cb);
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
  for (int i = 0; i < VFDREG; i++) regs[i].fails = 0;
  modbus_init();

  switch (spindle_get_type()) {
  case SPINDLE_TYPE_CUSTOM:  memcpy(regs, custom_regs, sizeof(regs)); break;
  case SPINDLE_TYPE_AC_TECH:          _load(ac_tech_regs);            break;
  case SPINDLE_TYPE_NOWFOREVER:       _load(nowforever_regs);         break;
  case SPINDLE_TYPE_DELTA_VFD015M21A: _load(delta_vfd015m21a_regs);   break;
  case SPINDLE_TYPE_YL600:            _load(yl600_regs);              break;
  case SPINDLE_TYPE_FR_D700:          _load(fr_d700_regs);            break;
  case SPINDLE_TYPE_SUNFAR_E300:      _load(sunfar_e300_regs);        break;
  case SPINDLE_TYPE_OMRON_MX2:        _load(omron_mx2_regs);          break;
  case SPINDLE_TYPE_V70:              _load(v70_regs);                break;
  case SPINDLE_TYPE_WJ200:            _load(wj200_regs);              break;
  case SPINDLE_TYPE_DMM_DYN4:         _load(dmm_dyn4_regs);           break;
  case SPINDLE_TYPE_GALT_G200:        _load(galt_g200_regs);          break;
  case SPINDLE_TYPE_TECO_E510:        _load(teco_e510_regs);          break;
  default: break;
  }

  _connect();
}


void vfd_spindle_deinit(deinit_cb_t cb) {
  vfd.shutdown = true;
  vfd.deinit_cb = cb;
}


void vfd_spindle_set(float power) {
  if (vfd.power != power)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      vfd.power = power;
      vfd.changed = true;
    }
}


float vfd_spindle_get() {return vfd.actual_power;}
uint16_t vfd_get_status() {return vfd.status;}


void vfd_spindle_rtc_callback() {
  if (!vfd.wait || !rtc_expired(vfd.wait)) return;
  vfd.wait = 0;
  _next_reg();
}


// Variable callbacks
uint16_t get_vfd_max_freq() {return vfd.max_freq;}
void set_vfd_max_freq(uint16_t max_freq) {vfd.max_freq = max_freq;}
bool get_vfd_multi_write() {return vfd.user_multi_write;}
void set_vfd_multi_write(bool value) {vfd.user_multi_write = value;}
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


uint8_t get_vfd_reg_fails(int reg) {return regs[reg].fails;}


void set_vfd_reg_fails(int reg, uint8_t value) {
  regs[reg].fails = value;
}
