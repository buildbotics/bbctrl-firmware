/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "gcode_state.h"
#include "status.h"


#define ARC_SEGMENT_LENGTH      0.1 // mm
#define MIN_ARC_RADIUS          0.1

#define MIN_ARC_SEGMENT_USEC    10000.0 // minimum arc segment time
#define MIN_ARC_SEGMENT_TIME    (MIN_ARC_SEGMENT_USEC / MICROSECONDS_PER_MINUTE)


stat_t mach_arc_feed(float target[], bool flags[], float offsets[],
                     bool offset_f[], float radius, bool radius_f,
                     float P, bool P_f, bool modal_g1_f,
                     motion_mode_t motion_mode);
void mach_arc_callback();
bool mach_arc_active();
void mach_abort_arc();
