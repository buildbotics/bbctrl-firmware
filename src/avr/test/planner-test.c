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

#include "axis.h"
#include "machine.h"
#include "command.h"
#include "plan/arc.h"
#include "plan/planner.h"
#include "plan/exec.h"
#include "plan/state.h"

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
  mp_init();                      // motion planning
  machine_init();                 // gcode machine
  axis_map_motors();

  stat_t status = STAT_OK;

  while (true) {
    mach_arc_callback();          // arc generation runs

    bool reading = !feof(stdin);

    if (reading && mp_queue_get_room()) {
      mp_state_callback();
      command_callback();

      if (mp_queue_get_room()) continue;
    }

    status = mp_exec_move();
    printf("EXEC: %s\n", status_to_pgmstr(status));

    switch (status) {
    case STAT_NOOP: break;       // No command executed
    case STAT_EAGAIN: continue;  // No command executed, try again

    case STAT_OK:                // Move executed
      //if (!st.move_queued) ALARM(STAT_EXPECTED_MOVE); // No move was queued
      //st.move_queued = false;
      //st.move_ready = true;
      continue;

    default:
      printf("ERROR: %s\n", status_to_pgmstr(status));
    }

    if (!reading) break;
  }

  printf("STATE: %s\n", mp_get_state_pgmstr(mp_get_state()));

  return 0;
}
