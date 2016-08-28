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

/* This module actually contains some parts that belong ion the
 * machine, and other parts that belong at the motion planner
 * level, but the whole thing is treated as if it were part of the
 * motion planner.
 */

#include "arc.h"

#include "buffer.h"
#include "line.h"
#include "config.h"
#include "util.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>


// See planner.h for MM_PER_ARC_SEGMENT and other arc setting #defines

typedef struct arArcSingleton {     // persistent planner and runtime variables
  uint8_t run_state;                // runtime state machine sequence

  float position[AXES];             // accumulating runtime position
  float offset[3];                  // IJK offsets

  float length;                     // length of line or helix in mm
  float theta;                      // total angle specified by arc
  float theta_end;
  float radius;                     // Raw R value, or computed via offsets
  float angular_travel;             // travel along the arc
  float linear_travel;              // travel along linear axis of arc
  float planar_travel;
  uint8_t full_circle;              // set true if full circle arcs specified
  uint32_t rotations;               // Full rotations for full circles (P value)

  uint8_t plane_axis_0;             // arc plane axis 0 - e.g. X for G17
  uint8_t plane_axis_1;             // arc plane axis 1 - e.g. Y for G17
  uint8_t linear_axis;              // linear axis (normal to plane)

  float arc_time;                   // total running time for arc (derived)
  float arc_segments;               // number of segments in arc or blend
  int32_t arc_segment_count;        // count of running segments
  float arc_segment_theta;          // angular motion per segment
  float arc_segment_linear_travel;  // linear motion per segment
  float center_0;           // center of circle at plane axis 0 (e.g. X for G17)
  float center_1;           // center of circle at plane axis 1 (e.g. Y for G17)

  MoveState_t ms;
} arc_t;

arc_t arc = {};


/* Returns a naive estimate of arc execution time to inform segment
 * calculation.  The arc time is computed not to exceed the time taken
 * in the slowest dimension in the arc plane or in linear
 * travel. Maximum feed rates are compared in each dimension, but the
 * comparison assumes that the arc will have at least one segment
 * where the unit vector is 1 in that dimension. This is not true for
 * any arbitrary arc, with the result that the time returned may be
 * less than optimal.
 */
static void _estimate_arc_time() {
  // Determine move time at requested feed rate
  if (mach.gm.feed_rate_mode == INVERSE_TIME_MODE) {
    // inverse feed rate has been normalized to minutes
    arc.arc_time = mach.gm.feed_rate;
    // reset feed rate so next block requires an explicit feed rate setting
    mach.gm.feed_rate = 0;
    mach.gm.feed_rate_mode = UNITS_PER_MINUTE_MODE;

  } else arc.arc_time = arc.length / mach.gm.feed_rate;

  // Downgrade the time if there is a rate-limiting axis
  arc.arc_time =
    max(arc.arc_time, arc.planar_travel/mach.a[arc.plane_axis_0].feedrate_max);
  arc.arc_time =
    max(arc.arc_time, arc.planar_travel/mach.a[arc.plane_axis_1].feedrate_max);

  if (0 < fabs(arc.linear_travel))
    arc.arc_time =
      max(arc.arc_time,
          fabs(arc.linear_travel/mach.a[arc.linear_axis].feedrate_max));
}


/* Compute arc center (offset) from radius.
 *
 * Needs to calculate the center of the circle that has the designated radius
 * and passes through both the current position and the target position
 *
 * This method calculates the following set of equations where:
 *
 *    [x,y] is the vector from current to target position,
 *    d == magnitude of that vector,
 *    h == hypotenuse of the triangle formed by the radius of the circle,
 *         the distance to the center of the travel vector.
 *
 * A vector perpendicular to the travel vector [-y,x] is scaled to the length
 * of h [-y/d*h, x/d*h] and added to the center of the travel vector [x/2,y/2]
 * to form the new point [i,j] at [x/2-y/d*h, y/2+x/d*h] which will be the
 * center of the arc.
 *
 *        d^2 == x^2 + y^2
 *        h^2 == r^2 - (d/2)^2
 *        i == x/2 - y/d*h
 *        j == y/2 + x/d*h
 *                                        O <- [i,j]
 *                                     -  |
 *                           r      -     |
 *                               -        |
 *                            -           | h
 *                         -              |
 *           [0,0] ->  C -----------------+--------------- T  <- [x,y]
 *                     | <------ d/2 ---->|
 *
 *        C - Current position
 *        T - Target position
 *        O - center of circle that pass through both C and T
 *        d - distance from C to T
 *        r - designated radius
 *        h - distance from center of CT to O
 *
 * Expanding the equations:
 *
 *        d -> sqrt(x^2 + y^2)
 *        h -> sqrt(4 * r^2 - x^2 - y^2)/2
 *        i -> (x - (y * sqrt(4 * r^2 - x^2 - y^2)) / sqrt(x^2 + y^2)) / 2
 *        j -> (y + (x * sqrt(4 * r^2 - x^2 - y^2)) / sqrt(x^2 + y^2)) / 2
 *
 * Which can be written:
 *
 *        i -> (x - (y * sqrt(4 * r^2 - x^2 - y^2))/sqrt(x^2 + y^2))/2
 *        j -> (y + (x * sqrt(4 * r^2 - x^2 - y^2))/sqrt(x^2 + y^2))/2
 *
 * Which we for size and speed reasons optimize to:
 *
 *        h_x2_div_d = sqrt(4 * r^2 - x^2 - y^2)/sqrt(x^2 + y^2)
 *        i = (x - (y * h_x2_div_d))/2
 *        j = (y + (x * h_x2_div_d))/2
 *
 * Computing clockwise vs counter-clockwise motion
 *
 * The counter clockwise circle lies to the left of the target direction.
 * When offset is positive the left hand circle will be generated -
 * when it is negative the right hand circle is generated.
 *
 *                                   T  <-- Target position
 *                                   ^
 *      Clockwise circles with       |     Clockwise circles with
 *      this center will have        |     this center will have
 *      > 180 deg of angular travel  |     < 180 deg of angular travel,
 *                        \          |      which is a good thing!
 *                         \         |         /
 *  center of arc when  ->  x <----- | -----> x <- center of arc when
 *  h_x2_div_d is positive           |             h_x2_div_d is negative
 *                                   |
 *                                   C  <-- Current position
 *
 *
 *    Assumes arc singleton has been pre-loaded with target and position.
 *    Parts of this routine were originally sourced from the grbl project.
 */
static stat_t _compute_arc_offsets_from_radius() {
  // Calculate the change in position along each selected axis
  float x = mach.ms.target[arc.plane_axis_0] - mach.position[arc.plane_axis_0];
  float y = mach.ms.target[arc.plane_axis_1] - mach.position[arc.plane_axis_1];

  // *** From Forrest Green - Other Machine Co, 3/27/14
  // If the distance between endpoints is greater than the arc diameter, disc
  // will be negative indicating that the arc is offset into the complex plane
  // beyond the reach of any real CNC. However, numerical errors can flip the
  // sign of disc as it approaches zero (which happens as the arc angle
  // approaches 180 degrees). To avoid mishandling these arcs we use the
  // closest real solution (which will be 0 when disc <= 0). This risks
  // obscuring g-code errors where the radius is actually too small (they will
  // be treated as half circles), but ensures that all valid arcs end up
  // reasonably close to their intended paths regardless of any numerical
  // issues.
  float disc = 4 * square(arc.radius) - (square(x) + square(y));

  float h_x2_div_d = (disc > 0) ? -sqrt(disc) / hypotf(x, y) : 0;

  // Invert the sign of h_x2_div_d if circle is counter clockwise (see header
  // notes)
  if (mach.gm.motion_mode == MOTION_MODE_CCW_ARC) h_x2_div_d = -h_x2_div_d;

  // Negative R is g-code-alese for "I want a circle with more than 180 degrees
  // of travel" (go figure!), even though it is advised against ever generating
  // such circles in a single line of g-code. By inverting the sign of
  // h_x2_div_d the center of the circles is placed on the opposite side of
  // the line of travel and thus we get the unadvisably long arcs as prescribed.
  if (arc.radius < 0) h_x2_div_d = -h_x2_div_d;

  // Complete the operation by calculating the actual center of the arc
  arc.offset[arc.plane_axis_0] = (x - y * h_x2_div_d) / 2;
  arc.offset[arc.plane_axis_1] = (y + x * h_x2_div_d) / 2;
  arc.offset[arc.linear_axis] = 0;

  return STAT_OK;
}


/* Compute arc from I and J (arc center point)
 *
 * The theta calculation sets up an clockwise or counterclockwise arc
 * from the current position to the target position around the center
 * designated by the offset vector.  All theta-values measured in
 * radians of deviance from the positive y-axis.
 *
 *                | <- theta == 0
 *              * * *
 *            *       *
 *          *           *
 *          *     O ----T   <- theta_end (e.g. 90 degrees: theta_end == PI/2)
 *          *   /
 *            C   <- theta_start (e.g. -145 degrees: theta_start == -PI*(3/4))
 *
 *  Parts of this routine were originally sourced from the grbl project.
 */
static stat_t _compute_arc() {
  // Compute radius. A non-zero radius value indicates a radius arc
  if (fp_NOT_ZERO(arc.radius))
    _compute_arc_offsets_from_radius(); // indicates a radius arc
  else // compute start radius
    arc.radius =
      hypotf(-arc.offset[arc.plane_axis_0], -arc.offset[arc.plane_axis_1]);

  // Test arc specification for correctness according to:
  // http://linuxcnc.org/docs/html/gcode/gcode.html#sec:G2-G3-Arc
  // "It is an error if: when the arc is projected on the selected
  //  plane, the distance from the current point to the center differs
  //  from the distance from the end point to the center by more than
  //  (.05 inch/.5 mm) OR ((.0005 inch/.005mm) AND .1% of radius)."

  // Compute end radius from the center of circle (offsets) to target endpoint
  float end_0 = arc.ms.target[arc.plane_axis_0] -
    arc.position[arc.plane_axis_0] - arc.offset[arc.plane_axis_0];
  float end_1 = arc.ms.target[arc.plane_axis_1] -
    arc.position[arc.plane_axis_1] - arc.offset[arc.plane_axis_1];
  // end radius - start radius
  float err = fabs(hypotf(end_0, end_1) - arc.radius);

  if (err > ARC_RADIUS_ERROR_MAX ||
      (err < ARC_RADIUS_ERROR_MIN && err > arc.radius * ARC_RADIUS_TOLERANCE))
    return STAT_ARC_SPECIFICATION_ERROR;

  // Calculate the theta (angle) of the current point (position)
  // arc.theta is angular starting point for the arc (also needed later for
  // calculating center point)
  arc.theta = atan2(-arc.offset[arc.plane_axis_0],
                    -arc.offset[arc.plane_axis_1]);

  // g18_correction is used to invert G18 XZ plane arcs for proper CW
  // orientation
  float g18_correction = (mach.gm.select_plane == PLANE_XZ) ? -1 : 1;

  if (arc.full_circle) {
    // angular travel always starts as zero for full circles
    arc.angular_travel = 0;
    // handle the valid case of a full circle arc w/P=0
    if (fp_ZERO(arc.rotations)) arc.rotations = 1.0;

  } else {
    arc.theta_end = atan2(end_0, end_1);

    // Compute the angular travel
    if (fp_EQ(arc.theta_end, arc.theta))
      // very large radii arcs can have zero angular travel (thanks PartKam)
      arc.angular_travel = 0;

    else {
      // make the difference positive so we have clockwise travel
      if (arc.theta_end < arc.theta)
        arc.theta_end += 2 * M_PI * g18_correction;

      // compute positive angular travel
      arc.angular_travel = arc.theta_end - arc.theta;

      // reverse travel direction if it's CCW arc
      if (mach.gm.motion_mode == MOTION_MODE_CCW_ARC)
        arc.angular_travel -= 2 * M_PI * g18_correction;
    }
  }

  // Add in travel for rotations
  if (mach.gm.motion_mode == MOTION_MODE_CW_ARC)
    arc.angular_travel += (2*M_PI * arc.rotations * g18_correction);
  else arc.angular_travel -= (2*M_PI * arc.rotations * g18_correction);

  // Calculate travel in the depth axis of the helix and compute the time it
  // should take to perform the move arc.length is the total mm of travel of
  // the helix (or just a planar arc)
  arc.linear_travel =
    arc.ms.target[arc.linear_axis] - arc.position[arc.linear_axis];
  arc.planar_travel = arc.angular_travel * arc.radius;
  // hypot is insensitive to +/- signs
  arc.length = hypotf(arc.planar_travel, arc.linear_travel);
  // get an estimate of execution time to inform arc_segment calculation
  _estimate_arc_time();

  // Find the minimum number of arc_segments that meets these constraints...
  float arc_segments_for_chordal_accuracy =
    arc.length / sqrt(4 * CHORDAL_TOLERANCE *
                      (2 * arc.radius - CHORDAL_TOLERANCE));
  float arc_segments_for_minimum_distance = arc.length / ARC_SEGMENT_LENGTH;
  float arc_segments_for_minimum_time =
    arc.arc_time * MICROSECONDS_PER_MINUTE / MIN_ARC_SEGMENT_USEC;

  arc.arc_segments =
    floor(min3(arc_segments_for_chordal_accuracy,
               arc_segments_for_minimum_distance,
               arc_segments_for_minimum_time));

  arc.arc_segments = max(arc.arc_segments, 1); // at least 1 arc_segment
  // gcode state struct gets arc_segment_time, not arc time
  arc.ms.move_time = arc.arc_time / arc.arc_segments;
  arc.arc_segment_count = (int32_t)arc.arc_segments;
  arc.arc_segment_theta = arc.angular_travel / arc.arc_segments;
  arc.arc_segment_linear_travel = arc.linear_travel / arc.arc_segments;
  arc.center_0 = arc.position[arc.plane_axis_0] - sin(arc.theta) * arc.radius;
  arc.center_1 = arc.position[arc.plane_axis_1] - cos(arc.theta) * arc.radius;
  // initialize the linear target
  arc.ms.target[arc.linear_axis] = arc.position[arc.linear_axis];

  return STAT_OK;
}


/* machine entry point for arc
 *
 * Generates an arc by queuing line segments to the move buffer. The arc is
 * approximated by generating a large number of tiny, linear arc_segments.
 */
stat_t mach_arc_feed(float target[], float flags[], // arc endpoints
                   float i, float j, float k, // raw arc offsets
                   float radius,  // non-zero radius implies radius mode
                   uint8_t motion_mode) { // defined motion mode
  // Set axis plane and trap arc specification errors

  // trap missing feed rate
  if (mach.gm.feed_rate_mode != INVERSE_TIME_MODE && fp_ZERO(mach.gm.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  // set radius mode flag and do simple test(s)
  bool radius_f = fp_NOT_ZERO(mach.gf.arc_radius);  // set true if radius arc
  // radius value must be + and > minimum radius
  if (radius_f && mach.gn.arc_radius < MIN_ARC_RADIUS)
    return STAT_ARC_RADIUS_OUT_OF_TOLERANCE;

  // setup some flags
  bool target_x = fp_NOT_ZERO(flags[AXIS_X]);       // is X axis specified
  bool target_y = fp_NOT_ZERO(flags[AXIS_Y]);
  bool target_z = fp_NOT_ZERO(flags[AXIS_Z]);

  bool offset_i = fp_NOT_ZERO(mach.gf.arc_offset[0]); // is offset I specified
  bool offset_j = fp_NOT_ZERO(mach.gf.arc_offset[1]); // J
  bool offset_k = fp_NOT_ZERO(mach.gf.arc_offset[2]); // K

  // Set the arc plane for the current G17/G18/G19 setting and test arc
  // specification Plane axis 0 and 1 are the arc plane, the linear axis is
  // normal to the arc plane.
  // G17 - the vast majority of arcs are in the G17 (XY) plane
  if (mach.gm.select_plane == PLANE_XY) {
    arc.plane_axis_0 = AXIS_X;
    arc.plane_axis_1 = AXIS_Y;
    arc.linear_axis  = AXIS_Z;

    if (radius_f) {
      // must have at least one endpoint specified
      if (!(target_x || target_y))
        return STAT_ARC_AXIS_MISSING_FOR_SELECTED_PLANE;

    } else if (offset_k)
      // center format arc tests, it's OK to be missing either or both i and j,
      // but error if k is present
      return STAT_ARC_SPECIFICATION_ERROR;

  } else if (mach.gm.select_plane == PLANE_XZ) { // G18
    arc.plane_axis_0 = AXIS_X;
    arc.plane_axis_1 = AXIS_Z;
    arc.linear_axis  = AXIS_Y;

    if (radius_f) {
      if (!(target_x || target_z))
        return STAT_ARC_AXIS_MISSING_FOR_SELECTED_PLANE;
    } else if (offset_j) return STAT_ARC_SPECIFICATION_ERROR;

  } else if (mach.gm.select_plane == PLANE_YZ) { // G19
    arc.plane_axis_0 = AXIS_Y;
    arc.plane_axis_1 = AXIS_Z;
    arc.linear_axis  = AXIS_X;

    if (radius_f) {
      if (!target_y && !target_z)
        return STAT_ARC_AXIS_MISSING_FOR_SELECTED_PLANE;
    } else if (offset_i) return STAT_ARC_SPECIFICATION_ERROR;
  }

  // set values in Gcode model state & copy it (line was already captured)
  mach_set_model_target(target, flags);

  // in radius mode it's an error for start == end
  if (radius_f && fp_EQ(mach.position[AXIS_X], mach.ms.target[AXIS_X]) &&
      fp_EQ(mach.position[AXIS_Y], mach.ms.target[AXIS_Y]) &&
      fp_EQ(mach.position[AXIS_Z], mach.ms.target[AXIS_Z]))
    return STAT_ARC_ENDPOINT_IS_STARTING_POINT;

  // now get down to the rest of the work setting up the arc for execution
  mach.gm.motion_mode = motion_mode;
  mach_set_work_offsets(&mach.gm);                // resolved offsets to gm
  mach.ms.line = mach.gm.line;                    // copy line number
  memcpy(&arc.ms, &mach.ms, sizeof(MoveState_t)); // context to arc singleton

  copy_vector(arc.position, mach.position);       // arc pos from gcode model

  arc.radius = TO_MILLIMETERS(radius);            // set arc radius or zero

  arc.offset[0] = TO_MILLIMETERS(i);              // offsets canonical form (mm)
  arc.offset[1] = TO_MILLIMETERS(j);
  arc.offset[2] = TO_MILLIMETERS(k);

  arc.rotations = floor(fabs(mach.gn.parameter)); // P must be positive integer

  // determine if this is a full circle arc. Evaluates true if no target is set
  arc.full_circle =
    fp_ZERO(flags[arc.plane_axis_0]) & fp_ZERO(flags[arc.plane_axis_1]);

  // compute arc runtime values
  RITORNO(_compute_arc());

  // trap zero length arcs that _compute_arc can throw
  if (fp_ZERO(arc.length)) return STAT_MINIMUM_LENGTH_MOVE;

  arc.run_state = MOVE_RUN;                // enable arc run from the callback
  mach_arc_callback();                     // Queue initial arc moves
  mach_finalize_move();

  return STAT_OK;
}


/* Generate an arc
 *
 *  Called from the controller main loop. Each time it's called it queues
 *  as many arc segments (lines) as it can before it blocks, then returns.
 *
 *  Parts of this routine were originally sourced from the grbl project.
 */
void mach_arc_callback() {
  while (arc.run_state != MOVE_OFF && mp_get_planner_buffer_room()) {
    arc.theta += arc.arc_segment_theta;
    arc.ms.target[arc.plane_axis_0] =
      arc.center_0 + sin(arc.theta) * arc.radius;
    arc.ms.target[arc.plane_axis_1] =
      arc.center_1 + cos(arc.theta) * arc.radius;
    arc.ms.target[arc.linear_axis] += arc.arc_segment_linear_travel;

    mp_aline(&arc.ms);                            // run the line
    copy_vector(arc.position, arc.ms.target);     // update arc current pos

    if (!--arc.arc_segment_count) arc.run_state = MOVE_OFF;
  }
}


bool mach_arc_active() {return arc.run_state != MOVE_OFF;}


/// Stop arc movement without maintaining position
/// OK to call if no arc is running
void mach_abort_arc() {
  arc.run_state = MOVE_OFF;
}
