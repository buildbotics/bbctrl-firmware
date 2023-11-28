/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

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
