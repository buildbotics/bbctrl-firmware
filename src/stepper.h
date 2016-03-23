/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2012 - 2015 Rob Giseburt
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


/* # Coordinated motion
 *
 * Coordinated motion (line drawing) is performed using a classic
 * Bresenham DDA.  A number of additional steps are taken to optimize
 * interpolation and pulse train timing accuracy to minimize pulse
 * jitter and make for very smooth motion and surface finish.
 *
 * - stepper.c is not used as a 'ramp' for acceleration
 *   management. Accel is computed upstream in the motion planner as
 *   3rd order (controlled jerk) equations. These generate accel/decel
 *   segments that are passed to stepper.c for step output.
 *
 * - stepper.c accepts and processes fractional motor steps as floating
 *   point numbers from the planner. Steps do not need to be whole
 *   numbers, and are not expected to be.  Rounding is performed to
 *   avoid a truncation bias.
 *
 * # Move generation overview and timing illustration
 *
 * This ASCII art illustrates a 4 segment move to show stepper sequencing
 * timing.
 *
 * LOAD/STEP (~5000uSec)             [L1][segment1][L2][segment2][L3]
 * PREP (100 uSec)               [P1]          [P2]          [P3]
 * EXEC (400 uSec)        [EXEC1]       [EXEC2]       [EXEC3]
 * PLAN (<4ms) [planmoveA][plan move B][plan move C][plan move D] etc.
 *
 * The move begins with the planner PLANning move A
 * [planmoveA]. When this is done the computations for the first
 * segment of move A's S curve are performed by the planner runtime,
 * EXEC1. The runtime computes the number of segments and the
 * segment-by-segment accelerations and decelerations for the
 * move. Each call to EXEC generates the values for the next segment
 * to be run. Once the move is running EXEC is executed as a
 * callback from the step loader.
 *
 * When the runtime calculations are done EXEC calls the segment
 * PREParation function [P1].  PREP turns the EXEC results into
 * values needed for the loader and does some encoder work.  The
 * combined exec and prep take about 400 uSec.
 *
 * PREP takes care of heavy numerics and other cycle-intesive
 * operations so the step loader L1 can run as fast as possible. The
 * time budget for LOAD is about 10 uSec. In the diagram, when P1 is
 * done segment 1 is loaded into the stepper runtime [L1]
 *
 * Once the segment is loaded it will pulse out steps for the
 * duration of the segment.  Segment timing can vary, but segments
 * take around 5 Msec to pulse out.
 *
 * Now the move is pulsing out segment 1 (at HI interrupt
 * level). Once the L1 loader is finished it invokes the exec
 * function for the next segment (at LO interrupt level).  [EXEC2]
 * and [P2] compute and prepare the segment 2 for the loader so it
 * can be loaded as soon as segment 1 is complete [L2]. When move A
 * is done EXEC pulls the next move (moveB) from the planner queue,
 * The process repeats until there are no more segments or moves.
 *
 * While all this is happening subsequent moves (B, C, and D) are
 * being planned in background.  As long as a move takes less than
 * the segment times (5ms x N) the timing budget is satisfied,
 *
 * # Line planning and execution (in more detail)
 *
 * Move planning, execution and pulse generation takes place at 3
 * levels:
 *
 * Move planning occurs in the main-loop. The canonical machine calls
 * the planner to generate lines, arcs, dwells, synchronous
 * stop/starts, and any other cvommand that needs to be syncronized
 * wsith motion. The planner module generates blocks (bf's) that hold
 * parameters for lines and the other move types. The blocks are
 * backplanned to join lines and to take dwells and stops into
 * account. ("plan" stage).
 *
 * Arc movement is planned above the line planner. The arc planner
 * generates short lines that are passed to the line planner.
 *
 * Once lines are planned the must be broken up into "segments" of
 * about 5 milliseconds to be run. These segments are how S curves are
 * generated. This is the job of the move runtime (aka. exec or mr).
 *
 * Move execution and load prep takes place at the LOW interrupt
 * level. Move execution generates the next acceleration, cruise, or
 * deceleration segment for planned lines, or just transfers
 * parameters needed for dwells and stops. This layer also prepares
 * segments for loading by pre-calculating the values needed by
 * stepper.c and converting the segment into parameters that can be directly
 * loaded into the steppers ("exec" and "prep" stages).
 *
 * # Multi stage pull queue
 *
 * What happens when the pulse generator is done with the current pulse train
 * (segment) is a multi-stage "pull" queue that looks like this:
 *
 * As long as the steppers are running the sequence of events is:
 *
 *   - The stepper interrupt (HI) runs the step clock to setup the motor
 *     timers for the current move. This runs for the length of the pulse
 *     train currently executing.  The "segment", usually 5ms worth of pulses
 *
 *   - When the current segment is finished the stepper interrupt LOADs the
 *     next segment from the prep buffer, reloads the timers, and starts the
 *     next segment. At the of the load the stepper interrupt routine requests
 *     an "exec" of the next move in order to prepare for the next load
 *     operation. It does this by calling the exec using a software interrupt
 *     (actually a timer, since that's all we've got).
 *
 *   - As a result of the above, the EXEC handler fires at the LO interrupt
 *     level. It computes the next accel/decel or cruise (body) segment for the
 *     current move (i.e. the move in the planner's runtime buffer) by calling
 *     back to the exec routine in planner.c. If there are no more segments to
 *     run for the move the exec first gets the next buffer in the planning
 *     queue and begins execution.
 *
 *     In some cases the next "move" is not actually a move, but a dewll, stop,
 *     IO operation (e.g. M5). In this case it executes the requested operation,
 *     and may attempt to get the next buffer from the planner when its done.
 *
 *   - Once the segment has been computed the exec handler finshes up by
 *     running the PREP routine in stepper.c. This computes the timer values and
 *     gets the segment into the prep buffer - and ready for the next LOAD
 *     operation.
 *
 *   - The main loop runs in background to receive gcode blocks, parse them,
 *     and send them to the planner in order to keep the planner queue full so
 *     that when the planner's runtime buffer completes the next move (a gcode
 *     block or perhaps an arc segment) is ready to run.
 *
 * If the steppers are not running the above is similar, except that the exec is
 * invoked from the main loop by the software interrupt, and the stepper load is
 * invoked from the exec by another software interrupt.
 *
 * # Control flow
 *
 * Control flow can be a bit confusing. This is a typical sequence for planning
 * executing, and running an acceleration planned line:
 *
 *  1  planner.mp_aline() is called, which populates a planning buffer (bf)
 *     and back-plans any pre-existing buffers.
 *
 *  2  When a new buffer is added _mp_queue_write_buffer() tries to invoke
 *     execution of the move by calling stepper.st_request_exec_move().
 *
 *  3a If the steppers are running this request is ignored.
 *  3b If the steppers are not running this will set a timer to cause an
 *     EXEC "software interrupt" that will ultimately call st_exec_move().
 *
 *  4  At this point a call to _exec_move() is made, either by the
 *     software interrupt from 3b, or once the steppers finish running
 *     the current segment and have loaded the next segment. In either
 *     case the call is initated via the EXEC software interrupt which
 *     causes _exec_move() to run at the MEDium interupt level.
 *
 *  5  _exec_move() calls back to planner.mp_exec_move() which generates
 *     the next segment using the mr singleton.
 *
 *  6  When this operation is complete mp_exec_move() calls the appropriate
 *     PREP routine in stepper.c to derive the stepper parameters that will
 *     be needed to run the move - in this example st_prep_line().
 *
 *  7  st_prep_line() generates the timer values and stages these into
 *     the prep structure (sp) - ready for loading into the stepper runtime
 *     struct
 *
 *  8  stepper.st_prep_line() returns back to planner.mp_exec_move(), which
 *     frees the planning buffer (bf) back to the planner buffer pool if the
 *     move is complete. This is done by calling
 *     _mp_request_finalize_run_buffer()
 *
 *  9  At this point the MED interrupt is complete, but the planning buffer has
 *     not actually been returned to the pool yet. The buffer will be returned
 *     by the main-loop prior to testing for an available write buffer in order
 *     to receive the next Gcode block. This handoff prevents possible data
 *     conflicts between the interrupt and main loop.
 *
 * 10  The final step in the sequence is _load_move() requesting the next
 *     segment to be executed and prepared by calling st_request_exec()
 *     - control goes back to step 4.
 *
 * Note: For this to work you have to be really careful about what structures
 * are modified at what level, and use volatiles where necessary.
 */

#include "config.h"
#include "status.h"
#include "canonical_machine.h"
#include "plan/planner.h"

#include <stdbool.h>


/* Stepper control structures
 *
 * There are 5 main structures involved in stepper operations;
 *
 * data structure:                   found in:    runs primarily at:
 *   mpBuffer planning buffers (bf)    planner.c    main loop
 *   mrRuntimeSingleton (mr)           planner.c    MED ISR
 *   stConfig (st_cfg)                 stepper.c    write=bkgd, read=ISRs
 *   stPrepSingleton (st_pre)          stepper.c    MED ISR
 *   stRunSingleton (st_run)           stepper.c    HI ISR
 *
 * Care has been taken to isolate actions on these structures to the execution
 * level in which they run and to use the minimum number of volatiles in these
 * structures.  This allows the compiler to optimize the stepper inner-loops
 * better.
 */

void stepper_init();
uint8_t st_runtime_isbusy();
void st_request_exec_move();
void st_request_load_move();
stat_t st_prep_line(float travel_steps[], float following_error[],
                    float segment_time);
void st_prep_null();
void st_prep_command(void *bf); // void * since mpBuf_t is not visible here
void st_prep_dwell(float seconds);
