/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#ifndef CBANG_AXIS_ANGLE_H
#define CBANG_AXIS_ANGLE_H

#include "Vector.h"

#include <cbang/Math.h>

namespace cb {
  template <typename T> class Quaternion;

  template <typename T>
  class AxisAngle : public Vector<4, T> {
  public:
    typedef Vector<4, T> Super_T;
    using Super_T::data;

    AxisAngle() {}
    AxisAngle(T angle, const Vector<3, T> &vec) :
      Vector<4, T>(vec.x(), vec.y(), vec.z(), angle) {}
    AxisAngle(T angle, T x, T y, T z) : Vector<4, T>(x, y, z, angle) {}

    const T &angle() const {return data[3];}
    T &angle() {return data[3];}

    Vector<3, T> getVector() const {
      return Vector<3, T>(Super_T::x(), Super_T::y(), Super_T::z());
    }

    Vector<3, T> rotate(const Vector<3, T> &v) const {
      Vector<3, T> k(Super_T::x(), Super_T::y(), Super_T::z());
      T c = cos(angle());
      T s = sin(angle());

      // Rodrigues' rotation forumla
      return v * c + (k.cross(v) * s) + k * k.dot(v) * (1 - c);
    }

    Quaternion<T> toQuaternion() const;

    void toGLRotation(T rotation[4]) const {
      rotation[0] = (angle() * 180 / M_PI); // To degrees
      rotation[1] = Super_T::x();
      rotation[2] = Super_T::y();
      rotation[3] = Super_T::z();
    }
  };


  typedef AxisAngle<float> AxisAngleF;
  typedef AxisAngle<double> AxisAngleD;
}


#include "Quaternion.h"

namespace cb {
  template <typename T> inline
  Quaternion<T> AxisAngle<T>::toQuaternion() const
  {return Quaternion<T>(*this);}
}

#endif // CBANG_AXIS_ANGLE_H
