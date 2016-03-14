/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include "planner.h"
#include "stepper.h"
#include "util.h"

#include <stdbool.h>
#include <math.h>

/* Feedhold is executed as cm.hold_state transitions executed inside
 * _exec_aline() and main loop callbacks to these functions:
 * mp_plan_hold_callback() and mp_end_hold().
 *
 * Holds work like this:
 *
 * - Hold is asserted by calling cm_feedhold() (usually invoked via a
 *   ! char) If hold_state is OFF and motion_state is RUNning it sets
 *   hold_state to SYNC and motion_state to HOLD.
 *
 * - Hold state == SYNC tells the aline exec routine to execute the
 *   next aline segment then set hold_state to PLAN. This gives the
 *   planner sufficient time to replan the block list for the hold
 *   before the next aline segment needs to be processed.
 *
 * - Hold state == PLAN tells the planner to replan the mr buffer, the
 *   current run buffer (bf), and any subsequent bf buffers as
 *   necessary to execute a hold. Hold planning replans the planner
 *   buffer queue down to zero and then back up from zero. Hold state
 *   is set to DECEL when planning is complete.
 *
 * - Hold state == DECEL persists until the aline execution runs to
 *   zero velocity, at which point hold state transitions to HOLD.
 *
 * - Hold state == HOLD persists until the cycle is restarted. A cycle
 *   start is an asynchronous event that sets the cycle_start_flag
 *   TRUE. It can occur any time after the hold is requested - either
 *   before or after motion stops.
 *
 * - mp_end_hold() is executed from cm_feedhold_sequencing_callback()
 *   once the hold state == HOLD and a cycle_start has been
 *   requested.This sets the hold state to OFF which enables
 *   _exec_aline() to continue processing. Move execution begins with
 *   the first buffer after the hold.
 *
 * Terms used:
 *  - mr is the runtime buffer. It was initially loaded from the bf
 *    buffer
 *  - bp+0 is the "companion" bf buffer to the mr buffer.
 *  - bp+1 is the bf buffer following bp+0. This runs through bp+N
 *  - bp (by itself) just refers to the current buffer being
 *    adjusted / replanned
 *
 * Details: Planning re-uses bp+0 as an "extra" buffer. Normally bp+0
 *   is returned to the buffer pool as it is redundant once mr is
 *   loaded. Use the extra buffer to split the move in two where the
 *   hold decelerates to zero. Use one buffer to go to zero, the other
 *   to replan up from zero. All buffers past that point are
 *   unaffected other than that they need to be replanned for
 *   velocity.
 *
 * Note: There are multiple opportunities for more efficient
 *   organization of code in this module, but the code is so
 *   complicated I just left it organized for clarity and hoped for
 *   the best from compiler optimization.
 */


/// Resets all blocks in the planning list to be replannable
static void _reset_replannable_list() {
  mpBuf_t *bf = mp_get_first_buffer();
  if (!bf) return;

  mpBuf_t *bp = bf;

  do {
    bp->replannable = true;
  } while ((bp = mp_get_next_buffer(bp)) != bf && bp->move_state != MOVE_OFF);
}


static float _compute_next_segment_velocity() {
  if (mr.section == SECTION_BODY) return mr.segment_velocity;
#ifdef __JERK_EXEC
  return mr.segment_velocity; // an approximation
#else
  return mr.segment_velocity + mr.forward_diff_5;
#endif
}


/// replan block list to execute hold
stat_t mp_plan_hold_callback() {
  if (cm.hold_state != FEEDHOLD_PLAN)
    return STAT_NOOP; // not planning a feedhold

  mpBuf_t *bp; // working buffer pointer
  if (!(bp = mp_get_run_buffer())) return STAT_NOOP; // Oops! nothing's running

  uint8_t mr_flag = true;    // used to tell replan to account for mr buffer Vx
  float mr_available_length; // length left in mr buffer for deceleration
  float braking_velocity;    // velocity left to shed to brake to zero
  float braking_length;      // distance to brake to zero from braking_velocity

  // examine and process mr buffer
  mr_available_length = get_axis_vector_length(mr.target, mr.position);

  // compute next_segment velocity
  braking_velocity = _compute_next_segment_velocity();
  // bp is OK to use here
  braking_length = mp_get_target_length(braking_velocity, 0, bp);

  // Hack to prevent Case 2 moves for perfect-fit decels. Happens in
  // homing situations The real fix: The braking velocity cannot
  // simply be the mr.segment_velocity as this is the velocity of the
  // last segment, not the one that's going to be executed next.  The
  // braking_velocity needs to be the velocity of the next segment
  // that has not yet been computed. In the mean time, this hack will
  // work.
  if (braking_length > mr_available_length && fp_ZERO(bp->exit_velocity))
    braking_length = mr_available_length;

  // Case 1: deceleration fits entirely into the length remaining in mr buffer
  if (braking_length <= mr_available_length) {
    // set mr to a tail to perform the deceleration
    mr.exit_velocity = 0;
    mr.tail_length = braking_length;
    mr.cruise_velocity = braking_velocity;
    mr.section = SECTION_TAIL;
    mr.section_state = SECTION_NEW;

    // re-use bp+0 to be the hold point and to run the remaining block length
    bp->length = mr_available_length - braking_length;
    bp->delta_vmax = mp_get_target_velocity(0, bp->length, bp);
    bp->entry_vmax = 0;                  // set bp+0 as hold point
    bp->move_state = MOVE_NEW;           // tell _exec to re-use the bf buffer

    _reset_replannable_list();           // make it replan all the blocks
    mp_plan_block_list(mp_get_last_buffer(), &mr_flag);
    cm.hold_state = FEEDHOLD_DECEL;      // set state to decelerate and exit

    return STAT_OK;
  }

  // Case 2: deceleration exceeds length remaining in mr buffer
  // First, replan mr to minimum (but non-zero) exit velocity

  mr.section = SECTION_TAIL;
  mr.section_state = SECTION_NEW;
  mr.tail_length = mr_available_length;
  mr.cruise_velocity = braking_velocity;
  mr.exit_velocity =
    braking_velocity - mp_get_target_velocity(0, mr_available_length, bp);

  // Find where deceleration reaches zero. This could span multiple buffers.
  braking_velocity = mr.exit_velocity;      // adjust braking velocity downward
  bp->move_state = MOVE_NEW;                // tell _exec to re-use buffer

  // a safety to avoid wraparound
  for (uint8_t i = 0; i < PLANNER_BUFFER_POOL_SIZE; i++) {
    mp_copy_buffer(bp, bp->nx);             // copy bp+1 into bp+0, and onward

    if (bp->move_type != MOVE_TYPE_ALINE) { // skip any non-move buffers
      bp = mp_get_next_buffer(bp);          // point to next buffer
      continue;
    }

    bp->entry_vmax = braking_velocity;      // velocity we need to shed
    braking_length = mp_get_target_length(braking_velocity, 0, bp);

    if (braking_length > bp->length) {      // decel does not fit in bp buffer
      bp->exit_vmax =
        braking_velocity - mp_get_target_velocity(0, bp->length, bp);
      braking_velocity = bp->exit_vmax;     // braking velocity for next buffer
      bp = mp_get_next_buffer(bp);          // point to next buffer
      continue;
    }

    break;
  }

  // Deceleration now fits in the current bp buffer
  // Plan the first buffer of the pair as the decel, the second as the accel
  bp->length = braking_length;
  bp->exit_vmax = 0;

  bp = mp_get_next_buffer(bp);    // point to the acceleration buffer
  bp->entry_vmax = 0;
  bp->length -= braking_length;   // identical buffers and hence their lengths
  bp->delta_vmax = mp_get_target_velocity(0, bp->length, bp);
  bp->exit_vmax = bp->delta_vmax;

  _reset_replannable_list();      // replan all the blocks
  mp_plan_block_list(mp_get_last_buffer(), &mr_flag);
  cm.hold_state = FEEDHOLD_DECEL; // set state to decelerate and exit

  return STAT_OK;
}


/// End a feedhold, release the hold and restart block list
stat_t mp_end_hold() {
  if (cm.hold_state == FEEDHOLD_END_HOLD) {
    cm.hold_state = FEEDHOLD_OFF;

    if (!mp_get_run_buffer()) {    // 0 means nothing's running
      cm_set_motion_state(MOTION_STOP);
      return STAT_NOOP;
    }

    cm.motion_state = MOTION_RUN;
    st_request_exec_move();        // restart the steppers
  }

  return STAT_OK;
}
