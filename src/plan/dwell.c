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

#include "dwell.h"

#include "buffer.h"
#include "machine.h"
#include "stepper.h"


// Dwells are performed by passing a dwell move to the stepper drivers.


/// Dwell execution
static stat_t _exec_dwell(mpBuf_t *bf) {
  st_prep_dwell(bf->ms.move_time); // in seconds

  // free buffer & perform cycle_end if planner is empty
  mach_advance_buffer();

  return STAT_OK;
}


/// Queue a dwell
stat_t mp_dwell(float seconds) {
  mpBuf_t *bf = mp_get_write_buffer();

  // never supposed to fail
  if (!bf) return CM_ALARM(STAT_BUFFER_FULL_FATAL);

  bf->bf_func = _exec_dwell;  // register callback to dwell start
  bf->ms.move_time = seconds; // in seconds, not minutes
  bf->move_state = MOVE_NEW;

  // must be final operation before exit
  mp_commit_write_buffer(mach_get_line(), MOVE_TYPE_DWELL);

  return STAT_OK;
}
