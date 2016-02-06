/******************************************************************************\

                   This file is part of the TinyG firmware.

                     Copyright (c) 2016, Buildbotics LLC
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                            joseph@buildbotics.com

\******************************************************************************/

#ifndef COMMAND_H
#define COMMAND_H

#include "status.h"

#define MAX_ARGS 16

typedef uint8_t (*command_cb_t)(int argc, char *argv[]);

typedef struct {
  const char *name;
  command_cb_t cb;
  uint8_t minArgs;
  uint8_t maxArgs;
  const char *help;
} command_t;


stat_t command_dispatch();
int command_find(const char *name);
int command_exec(int argc, char *argv[]);
int command_eval(char *cmd);

#endif // COMMAND_H
