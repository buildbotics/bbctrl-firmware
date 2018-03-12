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

#include "axis.h"
#include "command.h"
#include "exec.h"
#include "state.h"
#include "vars.h"
#include "report.h"

#include <stdio.h>
#include <stdlib.h>


int main() {
  axis_map_motors();
  exec_init();                    // motion exec
  vars_init();                    // configuration variables
  command_init();

  stat_t status = STAT_OK;

  while (true) {
    bool reading = !feof(stdin);

    report_callback();

    if (reading) {
      state_callback();
      if (command_callback()) continue;
    }

    status = exec_next();
    printf("EXEC: %s\n", status_to_pgmstr(status));

    switch (status) {
    case STAT_NOP: break;       // No command executed
    case STAT_AGAIN: continue;  // No command executed, try again
    case STAT_OK: continue;     // Move executed

    default:
      printf("ERROR: %s\n", status_to_pgmstr(status));
    }

    if (!reading) break;
  }

  printf("STATE: %s\n", state_get_pgmstr(state_get()));

  return 0;
}
