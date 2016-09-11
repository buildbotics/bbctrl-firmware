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

/* This module actually contains some parts that belong in the machine, and
 * other parts that belong at the motion planner level, but the whole thing is
 * treated as if it were part of the motion planner.
 */

#include "arc.h"

#include "axes.h"
#include "buffer.h"
#include "line.h"
#include "gcode_parser.h"
#include "config.h"
#include "util.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>


typedef struct {
  bool running;
  int32_t line;                     // gcode block line number

  float target[AXES];               // XYZABC where the move should go
  float position[AXES];             // end point of the current segment

  float theta;                      // total angle specified by arc
  float radius;                     // Raw R value, or computed via offsets

  uint8_t plane_axis_0;             // arc plane axis 0 - e.g. X for G17
  uint8_t plane_axis_1;             // arc plane axis 1 - e.g. Y for G17
  uint8_t linear_axis;              // linear axis (normal to plane)

  uint32_t segments;                // count of running segments
  float segment_theta;              // angular motion per segment
  float segment_linear_travel;      // linear motion per segment
  float center_0;                   // center at axis 0 (e.g. X for G17)
  float center_1;                   // center at axis 1 (e.g. Y for G17)
} arc_t;

arc_t arc = {0};


/*** Returns a naive estimate of arc execution time to inform segment
 * calculation.  The arc time is computed not to exceed the time taken
 * in the slowest dimension in the arc plane or in linear
 * travel. Maximum feed rates are compared in each dimension, but the
 * comparison assumes that the arc will have at least one segment
 * where the unit vector is 1 in that dimension. This is not true for
 * any arbitrary arc, with the result that the time returned may be
 * less than optimal.
 */
static float _estimate_arc_time(float length, float linear_travel,
                                float planar_travel) {
  // Determine move time at requested feed rate
  // Inverse feed rate is normalized to minutes
  float time = mach_get_feed_mode() == INVERSE_TIME_MODE ?
    mach_get_feed_rate() : length / mach_get_feed_rate();

  // Downgrade the time if there is a rate-limiting axis
  return max4(time, planar_travel / axes[arc.plane_axis_0].feedrate_max,
              planar_travel / axes[arc.plane_axis_1].feedrate_max,
              fabs(linear_travel) / axes[arc.linear_axis].feedrate_max);
}


/*** Compute arc center (offset) from radius.
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
static stat_t _compute_arc_offsets_from_radius(float offset[]) {
  // Calculate the change in position along each selected axis
  float x = mach.gm.target[arc.plane_axis_0] - mach.position[arc.plane_axis_0];
  float y = mach.gm.target[arc.plane_axis_1] - mach.position[arc.plane_axis_1];

  // *** From Forrest Green - Other Machine Co, 3/27/14
  // If the distance between endpoints is greater than the arc diameter, disc
  // will be negative indicating that the arc is offset into the complex plane
  // beyond the reach of any real CNC.  However, numerical errors can flip the
  // sign of disc as it approaches zero (which happens as the arc angle
  // approaches 180 degrees).  To avoid mishandling these arcs we use the
  // closest real solution (which will be 0 when disc <= 0).  This risks
  // obscuring g-code errors where the radius is actually too small (they will
  // be treated as half circles), but ensures that all valid arcs end up
  // reasonably close to their intended paths regardless of any numerical
  // issues.
  float disc = 4 * square(arc.radius) - (square(x) + square(y));

  float h_x2_div_d = disc > 0 ? -sqrt(disc) / hypotf(x, y) : 0;

  // Invert sign of h_x2_div_d if circle is counter clockwise (see header notes)
  if (mach_get_motion_mode() == MOTION_MODE_CCW_ARC) h_x2_div_d = -h_x2_div_d;

  // Negative R is g-code-alese for "I want a circle with more than 180 degrees
  // of travel" (go figure!), even though it is advised against ever generating
  // such circles in a single line of g-code. By inverting the sign of
  // h_x2_div_d the center of the circles is placed on the opposite side of
  // the line of travel and thus we get the unadvisably long arcs as prescribed.
  if (arc.radius < 0) h_x2_div_d = -h_x2_div_d;

  // Complete the operation by calculating the actual center of the arc
  offset[arc.plane_axis_0] = (x - y * h_x2_div_d) / 2;
  offset[arc.plane_axis_1] = (y + x * h_x2_div_d) / 2;
  offset[arc.linear_axis] = 0;

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
static stat_t _compute_arc(bool radius_f, const float position[],
                           float offset[], float rotations, bool full_circle) {
  // Compute radius.  A non-zero radius value indicates a radius arc
  if (radius_f) _compute_arc_offsets_from_radius(offset);
  else // compute start radius
    arc.radius = hypotf(-offset[arc.plane_axis_0], -offset[arc.plane_axis_1]);

  // Test arc specification for correctness according to:
  // http://linuxcnc.org/docs/html/gcode/gcode.html#sec:G2-G3-Arc
  // "It is an error if: when the arc is projected on the selected
  //  plane, the distance from the current point to the center differs
  //  from the distance from the end point to the center by more than
  //  (.05 inch/.5 mm) OR ((.0005 inch/.005mm) AND .1% of radius)."

  // Compute end radius from the center of circle (offsets) to target endpoint
  float end_0 = arc.target[arc.plane_axis_0] -
    position[arc.plane_axis_0] - offset[arc.plane_axis_0];

  float end_1 = arc.target[arc.plane_axis_1] -
    position[arc.plane_axis_1] - offset[arc.plane_axis_1];

  // end radius - start radius
  float err = fabs(hypotf(end_0, end_1) - arc.radius);

  if (err > ARC_RADIUS_ERROR_MAX ||
      (err < ARC_RADIUS_ERROR_MIN && err > arc.radius * ARC_RADIUS_TOLERANCE))
    return STAT_ARC_SPECIFICATION_ERROR;

  // Calculate the theta (angle) of the current point (position)
  // arc.theta is angular starting point for the arc (also needed later for
  // calculating center point)
  arc.theta = atan2(-offset[arc.plane_axis_0], -offset[arc.plane_axis_1]);

  // g18_correction is used to invert G18 XZ plane arcs for proper CW
  // orientation
  float g18_correction = mach_get_plane() == PLANE_XZ ? -1 : 1;
  float angular_travel = 0;

  if (full_circle) {
    // angular travel always starts as zero for full circles
    angular_travel = 0;

    // handle the valid case of a full circle arc w/ P=0
    if (fp_ZERO(rotations)) rotations = 1.0;

  } else {
    float theta_end = atan2(end_0, end_1);

    // Compute the angular travel
    if (fp_EQ(theta_end, arc.theta))
      // very large radii arcs can have zero angular travel (thanks PartKam)
      angular_travel = 0;

    else {
      // make the difference positive so we have clockwise travel
      if (theta_end < arc.theta) theta_end += 2 * M_PI * g18_correction;

      // compute positive angular travel
      angular_travel = theta_end - arc.theta;

      // reverse travel direction if it's CCW arc
      if (mach_get_motion_mode() == MOTION_MODE_CCW_ARC)
        angular_travel -= 2 * M_PI * g18_correction;
    }
  }

  // Add in travel for rotations
  if (mach_get_motion_mode() == MOTION_MODE_CW_ARC)
    angular_travel += 2 * M_PI * rotations * g18_correction;
  else angular_travel -= 2 * M_PI * rotations * g18_correction;

  // Calculate travel in the depth axis of the helix and compute the time it
  // should take to perform the move length is the total mm of travel of
  // the helix (or just a planar arc)
  float linear_travel = arc.target[arc.linear_axis] - position[arc.linear_axis];
  float planar_travel = angular_travel * arc.radius;
  // hypot is insensitive to +/- signs
  float length = hypotf(planar_travel, linear_travel);

  // trap zero length arcs that _compute_arc can throw
  if (fp_ZERO(length)) return STAT_MINIMUM_LENGTH_MOVE;

  // get an estimate of execution time to inform segment calculation
  float arc_time = _estimate_arc_time(length, linear_travel, planar_travel);

  // Find the minimum number of segments that meets these constraints...
  float segments_for_chordal_accuracy =
    length / sqrt(4 * CHORDAL_TOLERANCE * (2 * arc.radius - CHORDAL_TOLERANCE));
  float segments_for_minimum_distance = length / ARC_SEGMENT_LENGTH;
  float segments_for_minimum_time =
    arc_time * MICROSECONDS_PER_MINUTE / MIN_ARC_SEGMENT_USEC;

  float segments = floor(min3(segments_for_chordal_accuracy,
                              segments_for_minimum_distance,
                              segments_for_minimum_time));
  if (segments < 1) segments = 1; // at least 1 segment

  arc.segments = (uint32_t)segments;
  arc.segment_theta = angular_travel / segments;
  arc.segment_linear_travel = linear_travel / segments;
  arc.center_0 = position[arc.plane_axis_0] - sin(arc.theta) * arc.radius;
  arc.center_1 = position[arc.plane_axis_1] - cos(arc.theta) * arc.radius;

  // initialize the linear position
  arc.position[arc.linear_axis] = position[arc.linear_axis];

  return STAT_OK;
}


/*** Machine entry point for arc
 *
 * Generates an arc by queuing line segments to the move buffer. The arc is
 * approximated by generating a large number of tiny, linear segments.
 */
stat_t mach_arc_feed(float target[], float target_f[],   // arc endpoints
                     float offsets[], float offsets_f[], // arc offsets
                     float radius,  bool radius_f,       // radius
                     float P, bool P_f,                  // parameter
                     bool modal_g1_f,
                     uint8_t motion_mode) { // defined motion mode

  // Trap some precursor cases.  Since motion mode (MODAL_GROUP_G1) persists
  // from the previous move it's possible for non-modal commands such as F or P
  // to arrive here when no motion has actually been specified. It's also
  // possible to run an arc as simple as "I25" if CW or CCW motion mode was
  // already set by a previous block.  Here are 2 cases to handle if CW or CCW
  // motion mode was set by a previous block:
  //
  // Case 1: F, P or other non modal is specified but no movement is specified
  //         (no offsets or radius). This is OK: return STAT_OK
  //
  // Case 2: Movement is specified w/o a new G2 or G3 word in the (new) block.
  //         This is OK: continue the move
  //
  if (!modal_g1_f && !offsets_f[0] && !offsets_f[1] && !offsets_f[2] &&
      !radius_f) return STAT_OK;

  // trap missing feed rate
  if (fp_ZERO(mach_get_feed_rate()) ||
      (mach_get_feed_mode() == INVERSE_TIME_MODE && !parser.gf.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  // radius must be positive and > minimum
  if (radius_f && radius < MIN_ARC_RADIUS)
    return STAT_ARC_RADIUS_OUT_OF_TOLERANCE;

  // Set the arc plane for the current G17/G18/G19 setting and test arc
  // specification Plane axis 0 and 1 are the arc plane, the linear axis is
  // normal to the arc plane.
  switch (mach_get_plane()) {
  case PLANE_XY: // G17
    arc.plane_axis_0 = AXIS_X;
    arc.plane_axis_1 = AXIS_Y;
    arc.linear_axis  = AXIS_Z;
    break;

  case PLANE_XZ: // G18
    arc.plane_axis_0 = AXIS_X;
    arc.plane_axis_1 = AXIS_Z;
    arc.linear_axis  = AXIS_Y;
    break;

  case PLANE_YZ: // G19
    arc.plane_axis_0 = AXIS_Y;
    arc.plane_axis_1 = AXIS_Z;
    arc.linear_axis  = AXIS_X;
    break;
  }

  // Test if endpoints are specified in the selected plane
  bool full_circle = false;
  if (!target_f[arc.plane_axis_0] && !target_f[arc.plane_axis_1]) {
    if (radius_f) // in radius mode arcs missing both endpoints is an error
      return STAT_ARC_AXIS_MISSING_FOR_SELECTED_PLANE;
    else full_circle = true; // in center format arc specifies full circle
  }

  // Test radius arcs for radius tolerance
  if (radius_f) {
    arc.radius = TO_MILLIMETERS(radius);    // set to internal format (mm)
    if (fabs(arc.radius) < MIN_ARC_RADIUS)  // radius value must be > minimum
      return STAT_ARC_RADIUS_OUT_OF_TOLERANCE;

    // Test that center format absolute distance mode arcs have both offsets
  } else if (mach_get_arc_distance_mode() == ABSOLUTE_MODE &&
             !(offsets_f[arc.plane_axis_0] && offsets_f[arc.plane_axis_1]))
    return STAT_ARC_OFFSETS_MISSING_FOR_PLANE;

  // Set arc rotations
  float rotations = 0;

  if (P_f) {
    // If P is present it must be a positive integer
    if (P < 0 || 0 < floor(P) - P) return STAT_P_WORD_IS_NOT_A_POSITIVE_INTEGER;

    rotations = P;

  } else if (full_circle) rotations = 1; // default to 1 for full circles

  // set values in Gcode model state & copy it (line was already captured)
  mach_set_model_target(target, target_f);

  // in radius mode it's an error for start == end
  if (radius_f && fp_EQ(mach.position[AXIS_X], mach.gm.target[AXIS_X]) &&
      fp_EQ(mach.position[AXIS_Y], mach.gm.target[AXIS_Y]) &&
      fp_EQ(mach.position[AXIS_Z], mach.gm.target[AXIS_Z]))
    return STAT_ARC_ENDPOINT_IS_STARTING_POINT;

  // now get down to the rest of the work setting up the arc for execution
  mach_set_motion_mode(motion_mode);
  mach_update_work_offsets();                      // Update resolved offsets
  arc.line = mach_get_line();                      // copy line number
  copy_vector(arc.target, mach.gm.target);         // copy move target
  arc.radius = TO_MILLIMETERS(radius);             // set arc radius or zero

  float offset[3];
  for (int i = 0; i < 3; i++) offset[i] = TO_MILLIMETERS(offsets[i]);

  if (mach_get_arc_distance_mode() == ABSOLUTE_MODE) {
    if (offsets_f[0]) offset[0] -= mach.position[0];
    if (offsets_f[1]) offset[1] -= mach.position[1];
    if (offsets_f[2]) offset[2] -= mach.position[2];
  }

  // compute arc runtime values
  RITORNO(_compute_arc(radius_f, mach.position, offset, rotations,
                       full_circle));

  // Note, arc soft limits are not tested here

  arc.running = true;                         // Enable arc run in callback
  mach_arc_callback();                        // Queue initial arc moves
  copy_vector(mach.position, mach.gm.target); // update model position

  return STAT_OK;
}


/*** Generate an arc
 *
 *  Called from the controller main loop.  Each time it's called it queues
 *  as many arc segments (lines) as it can before it blocks, then returns.
 */
void mach_arc_callback() {
  while (arc.running && mp_queue_get_room()) {
    if (arc.segments == 1) { // Final segment
      arc.position[arc.plane_axis_0] = arc.target[arc.plane_axis_0];
      arc.position[arc.plane_axis_1] = arc.target[arc.plane_axis_1];
      arc.position[arc.linear_axis] = arc.target[arc.linear_axis];

    } else {
      arc.theta += arc.segment_theta;

      arc.position[arc.plane_axis_0] =
        arc.center_0 + sin(arc.theta) * arc.radius;
      arc.position[arc.plane_axis_1] =
        arc.center_1 + cos(arc.theta) * arc.radius;
      arc.position[arc.linear_axis] += arc.segment_linear_travel;
    }

    mp_aline(arc.position, arc.line); // run the line

    if (!--arc.segments) arc.running = false;
  }
}


bool mach_arc_active() {return arc.running;}


/// Stop arc movement without maintaining position
/// OK to call if no arc is running
void mach_abort_arc() {arc.running = false;}
