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

#include "queue.h"

#include <string.h>


typedef struct {
  action_t action;

  union {
    float f;
    int32_t i;
    bool b;
    struct {
      int16_t l;
      int16_t r;
    } p;
  } value;
} buffer_t;


#define RING_BUF_NAME q
#define RING_BUF_TYPE buffer_t
#define RING_BUF_SIZE QUEUE_SIZE
#include "ringbuf.def"


void queue_init() {
  q_init();
}


void queue_flush() {q_init();}
bool queue_is_empty() {return q_empty();}
int queue_get_room() {return q_space();}
int queue_get_fill() {return q_fill();}


void queue_push(action_t action) {
  buffer_t b = {action};
  q_push(b);
}


void queue_push_float(action_t action, float value) {
  buffer_t b = {action, {.f = value}};
  q_push(b);
}


void queue_push_int(action_t action, int32_t value) {
  buffer_t b = {action, {.i = value}};
  q_push(b);
}


void queue_push_bool(action_t action, bool value) {
  buffer_t b = {action, {.b = value}};
  q_push(b);
}


void queue_push_pair(action_t action, int16_t left, int16_t right) {
  buffer_t b = {action};
  b.value.p.l = left;
  b.value.p.r = right;
  q_push(b);
}


void queue_pop() {q_pop();}
action_t queue_head() {return q_peek().action;}
float queue_head_float() {return q_peek().value.f;}
int32_t queue_head_int() {return q_peek().value.i;}
bool queue_head_bool() {return q_peek().value.b;}
int16_t queue_head_left() {return q_peek().value.p.l;}
int16_t queue_head_right() {return q_peek().value.p.r;}
