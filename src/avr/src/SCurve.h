/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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

#include <math.h>


class SCurve {
  float maxV;
  float maxA;
  float maxJ;

  float v;
  float a;
  float j;

public:
  SCurve(float maxV = 0, float maxA = 0, float maxJ = 0);

  float getMaxVelocity() const {return maxV;}
  void setMaxVelocity(float v) {maxV = v;}
  float getMaxAcceleration() const {return maxA;}
  void setMaxAcceleration(float a) {maxA = a;}
  float getMaxJerk() const {return maxJ;}
  void setMaxJerk(float j) {maxJ = j;}

  float getVelocity() const {return v;}
  float getAcceleration() const {return a;}
  float getJerk() const {return j;}

  unsigned getPhase() const;
  float getStoppingDist() const;
  float next(float t, float targetV);

  static float stoppingDist(float v, float a, float maxA, float maxJ);
  static float nextAccel(float t, float targetV, float v, float a, float maxA,
                         float maxJ);
  static float distance(float t, float v, float a, float j);
  static float velocity(float t, float a, float j);
  static float acceleration(float t, float j);
};
