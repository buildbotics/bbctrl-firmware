/*
 * kinematics.c - inverse kinematics routines
 * This file is part of the TinyG project
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
 *
 * This file ("the software") is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License, version 2 as published by the Free Software
 * Foundation. You should have received a copy of the GNU General
 * Public License, version 2 along with the software.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software
 * library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with  other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU General
 * Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT
 * WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "kinematics.h"

#include "canonical_machine.h"
#include "stepper.h"

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
  float joint[AXES];

  // you can insert inverse kinematics transformations here
  //  _inverse_kinematics(travel, joint);

  //...or just do a memcpy for Cartesian machines
  memcpy(joint, travel, sizeof(float) * AXES);

  // Map motors to axes and convert length units to steps
  // Most of the conversion math has already been done during config in
  // steps_per_unit() which takes axis travel, step angle and microsteps into
  // account.
  for (uint8_t axis = 0; axis < AXES; axis++) {
    if (cm.a[axis].axis_mode == AXIS_INHIBITED) joint[axis] = 0;

    for (int i = 0; i < MOTORS; i++)
      if (st_cfg.mot[i].motor_map == axis)
        steps[i] = joint[axis] * st_cfg.mot[i].steps_per_unit;
  }
}

