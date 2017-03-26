/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2012 - 2015 Rob Giseburt
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

/* Planner buffers are used to queue and operate on Gcode blocks. Each
 * buffer contains one Gcode block which may be a move, M code or
 * other command that must be executed synchronously with movement.
 */

#include "buffer.h"
#include "state.h"
#include "rtc.h"
#include "util.h"

#include <string.h>
#include <math.h>
#include <stdio.h>


typedef struct {
  uint8_t space;
  mp_buffer_t *tail;
  mp_buffer_t *head;
  mp_buffer_t bf[PLANNER_BUFFER_POOL_SIZE];
} buffer_pool_t;


buffer_pool_t mb;


/// Zeroes the contents of a buffer
static void _clear_buffer(mp_buffer_t *bf) {
  mp_buffer_t *next = bf->next;            // save pointers
  mp_buffer_t *prev = bf->prev;
  memset(bf, 0, sizeof(mp_buffer_t));
  bf->next = next;                         // restore pointers
  bf->prev = prev;
}


static void _push() {
  if (!mb.space) {
    ALARM(STAT_INTERNAL_ERROR);
    return;
  }

  mb.tail = mb.tail->next;
  mb.space--;
}


static void _pop() {
  if (mb.space == PLANNER_BUFFER_POOL_SIZE) {
    ALARM(STAT_INTERNAL_ERROR);
    return;
  }

  mb.head = mb.head->next;
  mb.space++;
}


/// Initializes or resets buffers
void mp_queue_init() {
  memset(&mb, 0, sizeof(mb));     // clear all values

  mb.tail = mb.head = &mb.bf[0];  // init head and tail
  mb.space = PLANNER_BUFFER_POOL_SIZE;

  // Setup ring pointers
  for (int i = 0; i < mb.space; i++) {
    mb.bf[i].next = &mb.bf[i + 1];
    mb.bf[i].prev = &mb.bf[i - 1];
  }

  mb.bf[0].prev = &mb.bf[mb.space -1];  // Fix first->prev
  mb.bf[mb.space - 1].next = &mb.bf[0]; // Fix last->next

  mp_state_idle();
}


uint8_t mp_queue_get_room() {
  return mb.space < PLANNER_BUFFER_HEADROOM ?
    0 : mb.space - PLANNER_BUFFER_HEADROOM;
}


uint8_t mp_queue_get_fill() {
  return PLANNER_BUFFER_POOL_SIZE - mb.space;
}


bool mp_queue_is_empty() {return mb.tail == mb.head;}


/// Get pointer to next buffer, waiting until one is available.
mp_buffer_t *mp_queue_get_tail() {
  while (!mb.space) continue; // Wait for a buffer
  return mb.tail;
}


/*** Commit the next buffer to the queue.
 *
 * WARNING: The routine calling mp_queue_push() must not use the write
 * buffer once it has been queued.  Action may start on the buffer immediately,
 * invalidating its contents
 */
void mp_queue_push(buffer_cb_t cb, uint32_t line) {
  mp_buffer_validate(mb.tail);
  mp_state_running();

  mb.tail->ts = rtc_get_time();
  mb.tail->cb = cb;
  mb.tail->line = line;
  mb.tail->state = BUFFER_NEW;

  _push();
}


mp_buffer_t *mp_queue_get_head() {
  return mp_queue_is_empty() ? 0 : mb.head;
}


/// Clear and release buffer to pool
void mp_queue_pop() {
  _clear_buffer(mb.head);
  _pop();
}


#ifdef DEBUG
void mp_queue_dump() {
  mp_buffer_t *bf = mp_queue_get_head();
  if (!bf) return;
  mp_buffer_t *bp = bf;

  do {
    if (bp != bf) putchar(',');
    mp_buffer_print(bp);
    bp = mp_buffer_next(bp);
  } while (bp != bf && bp->state != BUFFER_OFF);

  if (bp != bf) mp_buffer_print(bp);

  putchar('\n');
}


void mp_buffer_print(const mp_buffer_t *bf) {
  printf("{\n"
         "  ts:               %d,\n"
         "  line:             %d,\n"
         "  state:            %d,\n"
         "  replannable:      %s,\n"
         "  hold:             %s,\n"
         "  value:            %0.2f,\n"
         "  target:           [%0.2f, %0.2f, %0.2f, %0.2f],\n"
         "  unit:             [%0.2f, %0.2f, %0.2f, %0.2f],\n"
         "  length:           %0.2f,\n"
         "  head_length:      %0.2f,\n"
         "  body_length:      %0.2f,\n"
         "  tail_length:      %0.2f,\n"
         "  entry_velocity:   %0.2f,\n"
         "  cruise_velocity:  %0.2f,\n"
         "  exit_velocity:    %0.2f,\n"
         "  braking_velocity: %0.2f,\n"
         "  entry_vmax:       %0.2f,\n"
         "  cruise_vmax:      %0.2f,\n"
         "  exit_vmax:        %0.2f,\n"
         "  delta_vmax:       %0.2f,\n"
         "  jerk:             %0.2f,\n"
         "  cbrt_jerk:        %0.2f\n"
         "}", bf->ts, bf->line, bf->state, bf->replannable ? "true" : "false",
         bf->hold ? "true" : "false", bf->value, bf->target[0], bf->target[1],
         bf->target[2], bf->target[3], bf->unit[0], bf->unit[1], bf->unit[2],
         bf->unit[3], bf->length, bf->head_length, bf->body_length,
         bf->tail_length, bf->entry_velocity, bf->cruise_velocity,
         bf->exit_velocity, bf->braking_velocity, bf->entry_vmax,
         bf->cruise_vmax, bf->exit_vmax, bf->delta_vmax, bf->jerk,
         bf->cbrt_jerk);
}
#endif // DEBUG


void mp_buffer_validate(const mp_buffer_t *bp) {
  ASSERT(bp);

  ASSERT(isfinite(bp->value));

  ASSERT(isfinite(bp->target[0]) && isfinite(bp->target[1]) &&
         isfinite(bp->target[2]) && isfinite(bp->target[3]));
  ASSERT(isfinite(bp->unit[0]) && isfinite(bp->unit[1]) &&
         isfinite(bp->unit[2]) && isfinite(bp->unit[3]));

  ASSERT(isfinite(bp->length));
  ASSERT(isfinite(bp->head_length));
  ASSERT(isfinite(bp->body_length));
  ASSERT(isfinite(bp->tail_length));

  ASSERT(isfinite(bp->entry_velocity));
  ASSERT(isfinite(bp->cruise_velocity));
  ASSERT(isfinite(bp->exit_velocity));
  ASSERT(isfinite(bp->braking_velocity));

  ASSERT(isfinite(bp->jerk));
  ASSERT(isfinite(bp->cbrt_jerk));
}
