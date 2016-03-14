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

/* Encoders
 *
 * Calling this file "encoders" is kind of a lie, at least for
 * now. There are no encoders.  Instead the steppers count steps to
 * provide a "truth" reference for position. In the future when we
 * have real encoders we'll stop counting steps and actually measure
 * the position. Which should be a lot easier than how this module
 * currently works.
 *
 * Measuring position
 *
 * The challenge is that you can't just measure the position at any
 * arbitrary point because the system is so heavily queued (pipelined)
 * by the planner queue and the stepper sequencing.
 *
 * You only know where the machine should be at known "targets", which
 * are at the end of each move section (end of head, body, and
 * tail). You need to take encoder readings at these points. This
 * synchronization is taken care of by the Target, Position,
 * Position_delayed sequence in exec. Referring to ASCII art in
 * stepper.h and reproduced here:
 *
 * LOAD/STEP ~5000uS [L1][Seg1][L2][Seg2][L3][Seg3][L4][Seg4][Lb1][Segb1]
 * PREP 100 uS       [P1]      [P2]      [P3]      [P4]      [Pb1]      [Pb2]
 * EXEC 400 uS       [EXEC1]   [EXEC2]   [EXEC3]   [EXEC4]   [EXECb1]   [EXECb2]
 * PLAN <4ms  [PLANmoveA][PLANmoveB][PLANmoveC][PLANmoveD][PLANmoveE] etc.
 *
 * You can collect the target for moveA as early as the end of
 * [PLANmoveA]. The system will not reach that target position until
 * the end of [Seg4]. Data from Seg4 can only be processed
 * during the EXECb2 or Pb2 interval as it's the first time that is
 * not time-critical and you actually have enough cycles to calculate
 * the position and error terms. We use Pb2.
 *
 * Additionally, by this time the target in Gcode model knows about
 * has advanced quite a bit, so the moveA target needs to be saved
 * somewhere. Targets are propagated downward to the planner runtime
 * (the EXEC), but the exec will have moved on to moveB by the time we
 * need it. So moveA's target needs to be saved somewhere.
 *
 * Error correction
 *
 * The purpose of this module is to calculate an error term between
 * the programmed position (target) and the actual measured position
 * (position). The error term is used during move execution (exec) to
 * adjust the move to compensate for accumulated positional
 * errors. It's also the basis of closed-loop (servoed) systems.
 *
 * Positional error occurs due to floating point numerical
 * inaccuracies.  This code uses 32 bit floating point (GCC 32 bit,
 * which is NOT IEEE 32 bit). Errors creep in during planning, move
 * execution, and stepper output phases. Care has been taken to
 * minimize introducing errors throughout the process, but they still
 * occur.  In most cases errors are not noticeable as they fall below
 * the step resolution for most jobs. For jobs that run > 1 hour the
 * errors can accumulate and send results off by as much as a
 * millimeter if not corrected.
 *
 * Note: Going to doubles (from floats) would reduce the errors but
 * not eliminate them altogether. But this moot on AVRGCC which only
 * does single precision floats.
 *
 * Applying the error term for error correction
 *
 * So if you want to use the error from moveA to correct moveB it has
 * to be done in a region that is not already running (i.e. the head,
 * body, or tail) as moveB is already 2 segments into run.  Since most
 * moves in very short line Gcode files are body only, for practical
 * purposes the correction will be applied to moveC. (It's possible to
 * recompute the body of moveB, but it may not be worth the trouble).
 */

#pragma once


#include "config.h"

#include <stdint.h>


/// One real or virtual encoder per controlled motor
typedef struct enEncoder {
  int8_t  step_sign;       // set to +1 or -1
  int16_t steps_run;       // steps counted during stepper interrupt
  int32_t encoder_steps;   // counted encoder position in steps
} enEncoder_t;


extern enEncoder_t en[MOTORS]; // Used by stepper.c


void encoder_init();
void en_set_encoder_steps(uint8_t motor, float steps);
float en_read_encoder(uint8_t motor);
