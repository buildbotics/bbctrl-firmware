/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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
#include "machine.h"


typedef enum {   // Used for detecting gcode errors. See NIST section 3.4
  MODAL_GROUP_G0,     // {G10,G28,G28.1,G92}       non-modal axis commands
  MODAL_GROUP_G1,     // {G0,G1,G2,G3,G80}         motion
  MODAL_GROUP_G2,     // {G17,G18,G19}             plane selection
  MODAL_GROUP_G3,     // {G90,G91}                 distance mode
  MODAL_GROUP_G5,     // {G93,G94}                 feed rate mode
  MODAL_GROUP_G6,     // {G20,G21}                 units
  MODAL_GROUP_G7,     // {G40,G41,G42}             cutter radius compensation
  MODAL_GROUP_G8,     // {G43,G49}                 tool length offset
  MODAL_GROUP_G9,     // {G98,G99}                 return mode in canned cycles
  MODAL_GROUP_G12,    // {G54,G55,G56,G57,G58,G59} coordinate system selection
  MODAL_GROUP_G13,    // {G61,G61.1,G64}           path control mode
  MODAL_GROUP_M4,     // {M0,M1,M2,M30,M60}        stopping
  MODAL_GROUP_M6,     // {M6}                      tool change
  MODAL_GROUP_M7,     // {M3,M4,M5}                spindle turning
  MODAL_GROUP_M8,     // {M7,M8,M9}                coolant
  MODAL_GROUP_M9,     // {M48,M49}                 speed/feed override switches
} modal_group_t;

#define MODAL_GROUP_COUNT (MODAL_GROUP_M9 + 1)


typedef struct {
  gcode_state_t gn;                    // gcode input values
  gcode_state_t gf;                    // gcode input flags
} parser_t;


extern parser_t parser;


stat_t gc_gcode_parser(char *block);
