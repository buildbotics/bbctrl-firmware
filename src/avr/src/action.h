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

#pragma once

#include "status.h"


typedef enum {
  ACTION_SCURVE   = 's',
  ACTION_DATA     = 'd',
  ACTION_VELOCITY = 'v',

  ACTION_X        = 'X',
  ACTION_Y        = 'Y',
  ACTION_Z        = 'Z',
  ACTION_A        = 'A',
  ACTION_B        = 'B',
  ACTION_C        = 'C',

  ACTION_SEEK     = 'K',
  ACTION_OUTPUT   = 'O',

  ACTION_DWELL    = 'D',
  ACTION_PAUSE    = 'P',
  ACTION_TOOL     = 'T',
  ACTION_SPEED    = 'S',
  ACTION_JOG      = 'J',
  ACTION_LINE_NUM = 'N',

  ACTION_SET_HOME = 'H',
} action_t;


stat_t action_parse(const char *block);
