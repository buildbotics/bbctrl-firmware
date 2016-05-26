/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include "coolant.h"
#include "config.h"

#include <avr/io.h>


void coolant_init() {
  MIST_PORT.OUTSET = MIST_PIN_bm; // High
  MIST_PORT.DIRSET = MIST_PIN_bm; // Output
  FLOOD_PORT.OUTSET = FLOOD_PIN_bm; // High
  FLOOD_PORT.DIRSET = FLOOD_PIN_bm; // Output
}


void coolant_set_mist(bool x) {
  if (x) MIST_PORT.OUTCLR = MIST_PIN_bm;
  else MIST_PORT.OUTSET = MIST_PIN_bm;
}


void coolant_set_flood(bool x) {
  if (x) FLOOD_PORT.OUTCLR = FLOOD_PIN_bm;
  else FLOOD_PORT.OUTSET = FLOOD_PIN_bm;
}
