/*
 * dwell.c
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

#include "planner.h"
#include "canonical_machine.h"
#include "stepper.h"


// Dwells are performed by passing a dwell move to the stepper drivers.
// When the stepper driver sees a dwell it times the dwell on a separate
// timer than the stepper pulse timer.


/// Dwell execution
static stat_t _exec_dwell(mpBuf_t *bf) {
  // convert seconds to uSec
  st_prep_dwell((uint32_t)(bf->gm.move_time * 1000000));
  // free buffer & perform cycle_end if planner is empty
  if (mp_free_run_buffer()) cm_cycle_end();

  return STAT_OK;
}


/// Queue a dwell
stat_t mp_dwell(float seconds) {
  mpBuf_t *bf;

  if (!(bf = mp_get_write_buffer())) // get write buffer
    return cm_hard_alarm(STAT_BUFFER_FULL_FATAL); // never supposed to fail

  bf->bf_func = _exec_dwell;  // register callback to dwell start
  bf->gm.move_time = seconds; // in seconds, not minutes
  bf->move_state = MOVE_NEW;
  // must be final operation before exit
  mp_commit_write_buffer(MOVE_TYPE_DWELL);

  return STAT_OK;
}
