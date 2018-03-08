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

#include "command.h"

#include "usart.h"
#include "hardware.h"
#include "report.h"
#include "vars.h"
#include "estop.h"
#include "i2c.h"
#include "config.h"
#include "pgmspace.h"
#include "state.h"
#include "exec.h"
#include "base64.h"
#include "rtc.h"
#include "stepper.h"
#include "cpp_magic.h"

#ifdef __AVR__
#include <util/atomic.h>
#else
#define ATOMIC_BLOCK(x)
#define ATOMIC_RESTORESTATE
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#ifdef __AVR__
#define RING_BUF_ATOMIC_COPY(TO, FROM)          \
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) TO = FROM
#endif

#define RING_BUF_NAME sync_q
#define RING_BUF_TYPE uint8_t
#define RING_BUF_INDEX_TYPE volatile uint16_t
#define RING_BUF_SIZE SYNC_QUEUE_SIZE
#include "ringbuf.def"


static struct {
  bool active;
  uint32_t id;
  uint32_t last_empty;
  unsigned count;
  stat_t parse_error;
  int col;
  float position[AXES];
} cmd = {0,};


// Define command callbacks
#define CMD(CODE, NAME, SYNC, ...)              \
  stat_t command_##NAME(char *);                \
  IF(SYNC)(unsigned command_##NAME##_size();)   \
  IF(SYNC)(void command_##NAME##_exec(void *);)
#include "command.def"
#undef CMD


// Help & name
#define CMD(CODE, NAME, SYNC, HELP)                          \
  static const char command_##NAME##_name[] PROGMEM = #NAME; \
  static const char command_##NAME##_help[] PROGMEM = HELP;
#include "command.def"
#undef CMD


static uint16_t _space() {return sync_q_space();}


static bool _is_synchronous(char code) {
  switch (code) {
#define CMD(CODE, NAME, SYNC, ...) case COMMAND_##NAME: return SYNC;
#include "command.def"
#undef CMD
  }
  return false;
}


static stat_t _dispatch(char *s) {
  switch (*s) {
#define CMD(CODE, NAME, SYNC, ...)              \
    case COMMAND_##NAME: return command_##NAME(s);
#include "command.def"
#undef CMD
  }

  return STAT_INVALID_COMMAND;
}


static unsigned _size(char code) {
  switch (code) {
#define CMD(CODE, NAME, SYNC, ...)                                  \
    IF(SYNC)(case COMMAND_##NAME: return command_##NAME##_size();)
#include "command.def"
#undef CMD
  default: break;
  }

  return 0;
}


static void _exec_cb(char code, uint8_t *data) {
  switch (code) {
#define CMD(CODE, NAME, SYNC, ...)                                      \
    IF(SYNC)(case COMMAND_##NAME: command_##NAME##_exec(data); break;)
#include "command.def"
#undef CMD
  default: break;
  }
}


static void _i2c_cb(uint8_t *data, uint8_t length) {_dispatch((char *)data);}


void command_init() {i2c_set_read_callback(_i2c_cb);}
bool command_is_active() {return cmd.active;}
unsigned command_get_count() {return cmd.count;}


void command_print_json() {
  bool first = true;
  static const char fmt[] PROGMEM =
    "\"%c\":{\"name\":\"%" PRPSTR "\",\"help\":\"%" PRPSTR "\"}";

#define CMD(CODE, NAME, SYNC, HELP)                                     \
  if (first) first = false; else putchar(',');                          \
  printf_P(fmt, CODE, command_##NAME##_name, command_##NAME##_help);

#include "command.def"
#undef CMD
}


void command_flush_queue() {
  sync_q_init();
  cmd.count = 0;
}


void command_push(char code, void *_data) {
  uint8_t *data = (uint8_t *)_data;
  unsigned size = _size(code);

  sync_q_push(code);
  for (unsigned i = 0; i < size; i++) sync_q_push(*data++);

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) cmd.count++;
}


bool command_callback() {
  static char *block = 0;

  if (!block) block = usart_readline();
  if (!block) return false; // No command

  stat_t status = STAT_OK;

  // Special processing for synchronous commands
  if (_is_synchronous(*block)) {
    if (estop_triggered()) status = STAT_MACHINE_ALARMED;
    else if (state_is_flushing()) status = STAT_NOP; // Flush command
    else if (state_is_resuming() || _space() < _size(*block))
      return false; // Wait
  }

  // Dispatch non-empty commands
  if (*block && status == STAT_OK) {
    status = _dispatch(block);
    if (status == STAT_OK) cmd.active = true; // Disables LCD booting message
  }

  block = 0; // Command consumed

  // Reporting
  report_request();

  switch (status) {
  case STAT_OK: break;
  case STAT_NOP: break;
  case STAT_MACHINE_ALARMED: STATUS_WARNING(status, ""); break;
  default: STATUS_ERROR(status, ""); break;
  }

  return true;
}


void command_set_axis_position(int axis, const float p) {
  cmd.position[axis] = p;
}


void command_set_position(const float position[AXES]) {
  memcpy(cmd.position, position, sizeof(cmd.position));
}


void command_get_position(float position[AXES]) {
  memcpy(position, cmd.position, sizeof(cmd.position));
}


void command_reset_position() {
  float position[AXES];
  exec_get_position(position);
  command_set_position(position);
}


// Returns true if command queued
// Called from interrupt
bool command_exec() {
  if (!cmd.count) {
    cmd.last_empty = rtc_get_time();
    exec_set_velocity(0);
    state_idle();
    return false;
  }

  // On restart wait a bit to give queue a chance to fill
  if (!exec_get_velocity() && cmd.count < EXEC_FILL_TARGET &&
      !rtc_expired(cmd.last_empty + EXEC_DELAY)) return false;

  if (state_get() == STATE_HOLDING) return false;

  char code = (char)sync_q_next();
  unsigned size = _size(code);

  static uint8_t data[INPUT_BUFFER_LEN];
  for (unsigned i = 0; i < size; i++)
    data[i] = sync_q_next();

  cmd.count--;
  state_running();

  _exec_cb(code, data);
  report_request();

  return true;
}


// Var callbacks
uint32_t get_id() {return cmd.id;}
void set_id(uint32_t id) {cmd.id = id;}
