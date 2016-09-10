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
  OUTSET_PIN(SWITCH_1_PIN); // High
  DIRSET_PIN(SWITCH_1_PIN); // Output
  OUTSET_PIN(SWITCH_2_PIN); // High
  DIRSET_PIN(SWITCH_2_PIN); // Output
}


void coolant_set_mist(bool x) {
  if (x) OUTCLR_PIN(SWITCH_1_PIN);
  else OUTSET_PIN(SWITCH_1_PIN);
}


void coolant_set_flood(bool x) {
  if (x) OUTCLR_PIN(SWITCH_2_PIN);
  else OUTSET_PIN(SWITCH_2_PIN);
}


bool coolant_get_mist() {return OUT_PIN(SWITCH_1_PIN);}
bool coolant_get_flood() {return OUT_PIN(SWITCH_2_PIN);}
