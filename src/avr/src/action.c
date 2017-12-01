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

#include "action.h"

#include "queue.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>


static stat_t _queue(const char **block) {
  queue_push(**block++);
  return STAT_OK;
}


static stat_t _queue_int(const char **block) {
  char *end = 0;

  int32_t value = strtol(*block + 1, &end, 0);
  if (end == *block + 1) return STAT_INVALID_ARGUMENTS;

  queue_push_int(**block, value);
  *block = end;

  return STAT_OK;
}


static stat_t _queue_bool(const char **block) {
  if (4 < strlen(*block) && tolower((*block)[1]) == 't' &&
      tolower((*block)[2]) == 'r' && tolower((*block)[3]) == 'u' &&
      tolower((*block)[4]) == 'e' && !isalpha((*block)[5])) {
    queue_push_bool(**block, true);
    *block += 5;
    return STAT_OK;
  }

  char *end = 0;
  int32_t value = strtol(*block + 1, &end, 0);

  queue_push_bool(**block, value);
  *block = end;

  return STAT_OK;
}


static stat_t _queue_float(const char **block) {
  char *end = 0;

  float value = strtod(*block + 1, &end);
  if (end == *block + 1) return STAT_INVALID_ARGUMENTS;

  queue_push_float(**block, value);
  *block = end;

  return STAT_OK;
}


static stat_t _queue_pair(const char **block) {
  char *end = 0;

  int32_t left = strtol(*block + 1, &end, 0);
  if (end == *block + 1) return STAT_INVALID_ARGUMENTS;
  *block = end;

  int32_t right = strtol(*block, &end, 0);
  if (*block == end) return STAT_INVALID_ARGUMENTS;

  queue_push_pair(**block, left, right);
  *block = end;

  return STAT_OK;
}


stat_t action_parse(const char *block) {
  stat_t status = STAT_OK;

  while (*block && status == STAT_OK) {
    if (isspace(*block)) {block++; continue;}

    status = STAT_INVALID_COMMAND;

    switch (*block) {
    case ACTION_SCURVE:   status = _queue_int(&block);   break;
    case ACTION_DATA:     status = _queue_float(&block); break;
    case ACTION_VELOCITY: status = _queue_float(&block); break;
    case ACTION_X:        status = _queue_float(&block); break;
    case ACTION_Y:        status = _queue_float(&block); break;
    case ACTION_Z:        status = _queue_float(&block); break;
    case ACTION_A:        status = _queue_float(&block); break;
    case ACTION_B:        status = _queue_float(&block); break;
    case ACTION_C:        status = _queue_float(&block); break;
    case ACTION_SEEK:     status = _queue_pair(&block);  break;
    case ACTION_OUTPUT:   status = _queue_pair(&block);  break;
    case ACTION_DWELL:    status = _queue_float(&block); break;
    case ACTION_PAUSE:    status = _queue_bool(&block);  break;
    case ACTION_TOOL:     status = _queue_int(&block);   break;
    case ACTION_SPEED:    status = _queue_float(&block); break;
    case ACTION_JOG:      status = _queue(&block);       break;
    case ACTION_LINE_NUM: status = _queue_int(&block);   break;
    case ACTION_SET_HOME: status = _queue_pair(&block);  break;
    }
  }

  return status;
}
