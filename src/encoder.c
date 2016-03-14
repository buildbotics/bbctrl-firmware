/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2013 - 2015 Alden S. Hart, Jr.
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

#include "encoder.h"

#include <string.h>
#include <math.h>


enEncoder_t en[MOTORS];


void encoder_init() {
  memset(en, 0, sizeof(en)); // clear all values, pointers and status
}


/* Set encoder values to a current step count
 *
 * Sets encoder_steps. Takes floating point steps as input,
 * writes integer steps. So it's not an exact representation of machine
 * position except if the machine is at zero.
 */
void en_set_encoder_steps(uint8_t motor, float steps) {
  en[motor].encoder_steps = (int32_t)round(steps);
}


/* The stepper ISR counts steps into steps_run. These values are
 * accumulated to encoder_steps during LOAD (HI interrupt
 * level). The encoder_steps is therefore always stable. But be
 * advised: the position lags target and position values
 * elsewhere in the system becuase the sample is taken when the
 * steps for that segment are complete.
 */
float en_read_encoder(uint8_t motor) {
  return en[motor].encoder_steps;
}
