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

#include "vfd_test.h"
#include "modbus.h"
#include "status.h"
#include "command.h"
#include "type.h"
#include "exec.h"

#include <string.h>


static struct {
  bool enabled;
  bool waiting;
  uint16_t read_response;
  deinit_cb_t deinit_cb;
} vt;


static void _shutdown() {
  if (vt.waiting || vt.enabled) return;
  modbus_deinit();
  if (vt.deinit_cb) vt.deinit_cb();
}


void vfd_test_init() {
  modbus_init();
  vt.enabled = true;
  vt.waiting = false;
}


void vfd_test_deinit(deinit_cb_t cb) {
  vt.enabled = false;
  vt.deinit_cb = cb;
  _shutdown();
}


void vfd_test_set(float speed) {}
float vfd_test_get() {return 0;}
void vfd_test_stop() {}


// Variable callbacks
uint16_t get_modbus_response() {return vt.read_response;}


// Command callbacks
static stat_t _modbus_cmd_exec() {
  if (vt.waiting) return STAT_NOP;
  exec_set_cb(0);
  return STAT_AGAIN;
}


static void _modbus_rw_cb(bool ok, uint16_t addr, uint16_t value) {
  vt.read_response = value; // Always zero on error
  vt.waiting = false;
  _shutdown();
}


stat_t command_modbus_read(char *cmd) {
  stat_t status = STAT_OK;
  uint16_t addr = type_parse_u16(cmd + 1, &status);
  if (status == STAT_OK) command_push(*cmd, &addr);

  return status;
}


unsigned command_modbus_read_size() {return sizeof(uint16_t);}


void command_modbus_read_exec(void *data) {
  if (!vt.enabled) return;

  uint16_t addr = *(uint16_t *)data;

  vt.waiting = true;
  modbus_read(addr, _modbus_rw_cb);
  exec_set_cb(_modbus_cmd_exec);
}



stat_t command_modbus_write(char *cmd) {
  // Get value
  char *value = strchr(cmd + 1, '=');
  if (!value) return STAT_INVALID_COMMAND;
  *value++ = 0;

  stat_t status = STAT_OK;
  uint16_t buffer[2];
  buffer[0] = type_parse_u16(cmd + 1, &status);
  if (status == STAT_OK) buffer[1] = type_parse_u16(value, &status);

  if (status == STAT_OK) command_push(*cmd, buffer);

  return status;
}


unsigned command_modbus_write_size() {return 2 * sizeof(uint16_t);}


void command_modbus_write_exec(void *data) {
  if (!vt.enabled) return;

  uint16_t addr = ((uint16_t *)data)[0];
  uint16_t value = ((uint16_t *)data)[1];

  vt.waiting = true;
  modbus_write(addr, value, _modbus_rw_cb);
  exec_set_cb(_modbus_cmd_exec);
}
