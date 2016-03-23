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

#include "kinematics.h"

#include "canonical_machine.h"
#include "motor.h"

#include <string.h>

/* Wrapper routine for inverse kinematics
 *
 * Calls kinematics function(s).  Performs axis mapping & conversion
 * of length units to steps (and deals with inhibited axes)
 *
 * The reason steps are returned as floats (as opposed to, say,
 * uint32_t) is to accommodate fractional DDA steps. The DDA deals
 * with fractional step values as fixed-point binary in order to get
 * the smoothest possible operation. Steps are passed to the move prep
 * routine as floats and converted to fixed-point binary during queue
 * loading. See stepper.c for details.
 */
void ik_kinematics(const float travel[], float steps[]) {
  // you can insert inverse kinematics transformations here
  //   float joint[AXES];
  //   _inverse_kinematics(travel, joint);
  // ...or just use travel directly for Cartesian machines

  // Map motors to axes and convert length units to steps
  // Most of the conversion math has already been done during config in
  // steps_per_unit() which takes axis travel, step angle and microsteps into
  // account.
  for (int motor = 0; motor < MOTORS; motor++) {
    int axis = motor_get_axis(motor);
    if (cm.a[axis].axis_mode == AXIS_INHIBITED) steps[motor] = 0;
    else steps[motor] = travel[axis] * motor_get_steps_per_unit(motor);
  }
}
