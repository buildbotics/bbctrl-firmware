/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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

#include "command.h"

#include "usart.h"
#include "hardware.h"
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

#include <util/atomic.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#define RING_BUF_NAME sync_q
#define RING_BUF_TYPE uint8_t
#define RING_BUF_INDEX_TYPE volatile uint16_t
#define RING_BUF_SIZE SYNC_QUEUE_SIZE
#define RING_BUF_ATOMIC_COPY 1
#include "ringbuf.def"


static struct {
  bool active;
  uint16_t id;
  uint32_t last_empty;
  volatile uint16_t count;
  float position[AXES];
} cmd = {0,};


// Define command callbacks
#define CMD(CODE, NAME, SYNC)                   \
  stat_t command_##NAME(char *);                \
  IF(SYNC)(unsigned command_##NAME##_size();)   \
  IF(SYNC)(void command_##NAME##_exec(void *);)
#include "command.def"
#undef CMD


// Name
#define CMD(CODE, NAME, SYNC)                                   \
  static const char command_##NAME##_name[] PROGMEM = #NAME;
#include "command.def"
#undef CMD


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
  }

  return 0;
}


static void _exec_cb(char code, uint8_t *data) {
  switch (code) {
#define CMD(CODE, NAME, SYNC, ...)                                      \
    IF(SYNC)(case COMMAND_##NAME: command_##NAME##_exec(data); break;)
#include "command.def"
#undef CMD
  }
}


static void _i2c_cb(uint8_t *data, uint8_t length) {
  stat_t status = _dispatch((char *)data);
  if (status) STATUS_ERROR(status, "i2c: %s", data);
}


void command_init() {i2c_set_read_callback(_i2c_cb);}
bool command_is_active() {return cmd.active;}
unsigned command_get_count() {return cmd.count;}


void command_print_json() {
  bool first = true;
  static const char fmt[] PROGMEM = "\"%c\":{\"name\":\"%" PRPSTR "\"}";

#define CMD(CODE, NAME, SYNC)                                           \
  if (first) first = false; else putchar(',');                          \
  printf_P(fmt, CODE, command_##NAME##_name);

#include "command.def"
#undef CMD
}


void command_flush_queue() {
  sync_q_init();
  cmd.count = 0;
  command_reset_position();
}


void command_push(char code, void *_data) {
  uint8_t *data = (uint8_t *)_data;
  unsigned size = _size(code);

  if (!_is_synchronous(code)) estop_trigger(STAT_Q_INVALID_PUSH);
  if (sync_q_space() <= size) estop_trigger(STAT_Q_OVERRUN);

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
    else if (state_is_resuming() || sync_q_space() <= _size(*block))
      return false; // Wait
  }

  // Dispatch non-empty commands
  if (*block && status == STAT_OK) {
    status = _dispatch(block);
    if (status == STAT_OK) cmd.active = true; // Disables LCD booting message
  }

  switch (status) {
  case STAT_OK: break;
  case STAT_NOP: break;
  case STAT_MACHINE_ALARMED: STATUS_WARNING(status, ""); break;
  default: STATUS_ERROR(status, "%s", block); break;
  }

  block = 0; // Command consumed

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


char command_peek() {return (char)(cmd.count ? sync_q_peek() : 0);}


uint8_t *command_next() {
  if (!cmd.count) return 0;
  cmd.count--;

  if (sync_q_empty()) estop_trigger(STAT_Q_UNDERRUN);

  static uint8_t data[INPUT_BUFFER_LEN];

  data[0] = sync_q_next();

  if (!_is_synchronous((char)data[0])) estop_trigger(STAT_INVALID_QCMD);

  unsigned size = _size((char)data[0]);
  for (unsigned i = 0; i < size; i++)
    data[i + 1] = sync_q_next();

  return data;
}


// Returns true if command queued
// Called by exec.c from low-level interrupt
bool command_exec() {
  if (!cmd.count) {
    cmd.last_empty = rtc_get_time();
    state_idle();
    return false;
  }

  // On restart wait a bit to give queue a chance to fill
  if (cmd.count < EXEC_FILL_TARGET &&
      !rtc_expired(cmd.last_empty + EXEC_DELAY)) return false;

  uint8_t *data = command_next();
  state_running();

  _exec_cb((char)*data, data + 1);

  return true;
}


// Var callbacks
uint16_t get_id() {return cmd.id;}
void set_id(uint16_t id) {cmd.id = id;}
