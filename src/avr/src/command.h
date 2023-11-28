/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#pragma once

#include "config.h"
#include "status.h"

#include <stdbool.h>
#include <stdint.h>


// Commands
typedef enum {
#define CMD(CODE, NAME, ...) COMMAND_##NAME = CODE,
#include "command.def"
#undef CMD
} command_t;


void command_init();
bool command_is_active();
unsigned command_get_count();
void command_print_json();
void command_flush_queue();
void command_push(char code, void *data);
bool command_callback();
void command_set_axis_position(int axis, const float p);
void command_set_position(const float position[AXES]);
void command_get_position(float position[AXES]);
void command_reset_position();
char command_peek();
uint8_t *command_next();
bool command_exec();
