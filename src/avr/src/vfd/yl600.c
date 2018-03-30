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

#include <math.h>


#define P(H, L) ((H) << 8 | (L))


enum {
  P00_00_MAIN_FREQ          = P(0, 0),
  P00_01_START_STOP_SOURCE  = P(0, 1),
  P00_04_HIGEST_OUTPUT_FREQ = P(0, 4),
  P01_00_DIRECTION          = P(1, 0),
  P07_00_FREQ_1             = P(7, 0),
  P07_08_FREQ_SOURCE_1      = P(7, 8),
};


static struct {
  uint8_t state;
  bool changed;
  bool shutdown;

  float speed;
  uint16_t max_freq;
  float actual_speed;
} s = {0};


static void _next_command();


static uint16_t _speed2freq(float speed) {return fabs(speed) * s.max_freq;}
static float _freq2speed(uint16_t freq) {return (float)freq / s.max_freq;}


static void _next_state(bool ok) {
  if (!ok) s.state = 0;

  else if (s.changed) {
    s.changed = false;
    s.state = 0;

  } else if (++s.state == 5) s.state = 4;

  _next_command();
}


static void _read_cb(bool ok, uint16_t addr, uint16_t value) {
  if (ok)
    switch (addr) {
    case P00_04_HIGEST_OUTPUT_FREQ: s.max_freq = value; break;
    case P07_00_FREQ_1: s.actual_speed = _freq2speed(value); break;
    }

  _next_state(ok);
}


static void _write_cb(bool ok, uint16_t addr) {_next_state(ok);}


static void _next_command() {
  if (s.shutdown) {
    modbus_deinit();
    return;
  }

  // TODO spin direction
  switch (s.state) {
  case 0: modbus_read(P00_04_HIGEST_OUTPUT_FREQ, _read_cb); break;
  case 1: modbus_write(P00_01_START_STOP_SOURCE, 1, _write_cb); break;
  case 2: modbus_write(P07_08_FREQ_SOURCE_1, 1, _write_cb); break;
  case 3: modbus_write(P07_00_FREQ_1, _speed2freq(s.speed), _write_cb); break;
  case 4: modbus_read(P07_00_FREQ_1, _read_cb); break;
  }
}


void yl600_init() {
  modbus_init();
  s.shutdown = false;
  _next_command();
}


void yl600_deinit() {s.shutdown = true;}


void yl600_set(float speed) {
  if (s.speed != speed) {
    s.speed = speed;
    s.changed = true;
  }
}


float yl600_get() {return s.actual_speed;}
void yl600_stop() {yl600_set(0);}
