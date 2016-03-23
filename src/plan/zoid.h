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

#pragma once

#include "buffer.h"

/// Max iterations for convergence in the HT asymmetric case.
#define TRAPEZOID_ITERATION_MAX             10

/// Error percentage for iteration convergence. As percent - 0.01 = 1%
#define TRAPEZOID_ITERATION_ERROR_PERCENT   0.1

/// Tolerance for "exact fit" for H and T cases
/// allowable mm of error in planning phase
#define TRAPEZOID_LENGTH_FIT_TOLERANCE      0.0001

/// Adaptive velocity tolerance term
#define TRAPEZOID_VELOCITY_TOLERANCE        (max(2, bf->entry_velocity / 100))


void mp_calculate_trapezoid(mpBuf_t *bf);
float mp_get_target_length(const float Vi, const float Vf, const mpBuf_t *bf);
float mp_get_target_velocity(const float Vi, const float L, const mpBuf_t *bf);
