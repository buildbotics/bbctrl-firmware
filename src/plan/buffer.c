/*
 * buffer.c - Planner buffers
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
 * Copyright (c) 2012 - 2015 Rob Giseburt
 *
 * This file ("the software") is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License, version 2 as published by the Free Software
 * Foundation. You should have received a copy of the GNU General
 * Public License, version 2 along with the software.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software
 * library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with  other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU General
 * Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT
 * WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Planner buffers are used to queue and operate on Gcode blocks. Each
 * buffer contains one Gcode block which may be a move, and M code, or
 * other command that must be executed synchronously with movement.
 *
 * Buffers are in a circularly linked list managed by a WRITE pointer
 * and a RUN pointer.  New blocks are populated by (1) getting a write
 * buffer, (2) populating the buffer, then (3) placing it in the queue
 * (queue write buffer). If an exception occurs during population you
 * can unget the write buffer before queuing it, which returns it to
 * the pool of available buffers.
 *
 * The RUN buffer is the buffer currently executing. It may be
 * retrieved once for simple commands, or multiple times for
 * long-running commands like moves. When the command is complete the
 * run buffer is returned to the pool by freeing it.
 *
 * Notes:
 *    The write buffer pointer only moves forward on _queue_write_buffer, and
 *    the read buffer pointer only moves forward on free_read calls.
 *    (test, get and unget have no effect)
 */

#include "planner.h"
#include "stepper.h"

#include <string.h>

/// buffer incr & wrap
#define _bump(a) ((a < PLANNER_BUFFER_POOL_SIZE - 1) ? a + 1 : 0)


/// Returns # of available planner buffers
uint8_t mp_get_planner_buffers_available() {return mb.buffers_available;}


/// Initializes or resets buffers
void mp_init_buffers() {
  mpBuf_t *pv;

  memset(&mb, 0, sizeof(mb));      // clear all values, pointers and status

  mb.w = &mb.bf[0];                // init write and read buffer pointers
  mb.q = &mb.bf[0];
  mb.r = &mb.bf[0];
  pv = &mb.bf[PLANNER_BUFFER_POOL_SIZE - 1];

  // setup ring pointers
  for (uint8_t i = 0; i < PLANNER_BUFFER_POOL_SIZE; i++) {
    mb.bf[i].nx = &mb.bf[_bump(i)];
    mb.bf[i].pv = pv;
    pv = &mb.bf[i];
  }

  mb.buffers_available = PLANNER_BUFFER_POOL_SIZE;
}


/// Get pointer to next available write buffer
/// Returns pointer or 0 if no buffer available.
mpBuf_t *mp_get_write_buffer() {
  // get & clear a buffer
  if (mb.w->buffer_state == MP_BUFFER_EMPTY) {
    mpBuf_t *w = mb.w;
    mpBuf_t *nx = mb.w->nx;                   // save linked list pointers
    mpBuf_t *pv = mb.w->pv;
    memset(mb.w, 0, sizeof(mpBuf_t));         // clear all values
    w->nx = nx;                               // restore pointers
    w->pv = pv;
    w->buffer_state = MP_BUFFER_LOADING;
    mb.buffers_available--;
    mb.w = w->nx;

    return w;
  }

  return 0;
}


/// Free write buffer if you decide not to commit it.
void mp_unget_write_buffer() {
  mb.w = mb.w->pv;                            // queued --> write
  mb.w->buffer_state = MP_BUFFER_EMPTY;       // not loading anymore
  mb.buffers_available++;
}


/* Commit the next write buffer to the queue
 * Advances write pointer & changes buffer state
 *
 * WARNING: The routine calling mp_commit_write_buffer() must not use the write
 * buffer once it has been queued. Action may start on the buffer immediately,
 * invalidating its contents
 */
void mp_commit_write_buffer(const uint8_t move_type) {
  mb.q->move_type = move_type;
  mb.q->move_state = MOVE_NEW;
  mb.q->buffer_state = MP_BUFFER_QUEUED;
  mb.q = mb.q->nx; // advance the queued buffer pointer

  st_request_exec_move(); // requests an exec if the runtime is not busy
}


/* Get pointer to the next or current run buffer
 * Returns a new run buffer if prev buf was ENDed
 * Returns same buf if called again before ENDing
 * Returns 0 if no buffer available
 * The behavior supports continuations (iteration)
 */
mpBuf_t *mp_get_run_buffer() {
  // CASE: fresh buffer; becomes running if queued or pending
  if ((mb.r->buffer_state == MP_BUFFER_QUEUED) ||
      (mb.r->buffer_state == MP_BUFFER_PENDING))
    mb.r->buffer_state = MP_BUFFER_RUNNING;

  // CASE: asking for the same run buffer for the Nth time
  if (mb.r->buffer_state == MP_BUFFER_RUNNING)
    return mb.r; // return same buffer

  return 0; // CASE: no queued buffers. fail it.
}


/* Release the run buffer & return to buffer pool.
 * Returns true if queue is empty, false otherwise.
 * This is useful for doing queue empty / end move functions.
 */
uint8_t mp_free_run_buffer() {           // EMPTY current run buf & adv to next
  mp_clear_buffer(mb.r);                 // clear it out (& reset replannable)
  mb.r = mb.r->nx;                       // advance to next run buffer

  if (mb.r->buffer_state == MP_BUFFER_QUEUED) // only if queued...
    mb.r->buffer_state = MP_BUFFER_PENDING;   // pend next buffer

  mb.buffers_available++;

  return mb.w == mb.r; // return true if the queue emptied
}


///  Returns pointer to first buffer, i.e. the running block
mpBuf_t *mp_get_first_buffer() {
  return mp_get_run_buffer();    // returns buffer or 0 if nothing's running
}


/// Returns pointer to last buffer, i.e. last block (zero)
mpBuf_t *mp_get_last_buffer() {
  mpBuf_t *bf = mp_get_run_buffer();
  mpBuf_t *bp;

  for (bp = bf; bp && bp->nx != bf; bp = mp_get_next_buffer(bp))
    if (bp->nx->move_state == MOVE_OFF) break;

  return bp;
}


/// Zeroes the contents of the buffer
void mp_clear_buffer(mpBuf_t *bf) {
  mpBuf_t *nx = bf->nx;            // save pointers
  mpBuf_t *pv = bf->pv;
  memset(bf, 0, sizeof(mpBuf_t));
  bf->nx = nx;                     // restore pointers
  bf->pv = pv;
}


///  Copies the contents of bp into bf - preserves links
void mp_copy_buffer(mpBuf_t *bf, const mpBuf_t *bp) {
  mpBuf_t *nx = bf->nx;            // save pointers
  mpBuf_t *pv = bf->pv;
  memcpy(bf, bp, sizeof(mpBuf_t));
  bf->nx = nx;                     // restore pointers
  bf->pv = pv;
}
