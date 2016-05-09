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

#ifndef CBANG_QUATERNION_H
#define CBANG_QUATERNION_H

#include "Vector.h"
#include "AxisAngle.h"

#include <cbang/Math.h>


namespace cb {
  template <typename T>
  class Quaternion : public Vector<4, T> {
  public:
    typedef Vector<4, T> Super_T;
    using Super_T::data;
    using Super_T::x;
    using Super_T::y;
    using Super_T::z;

    Quaternion() {}
    Quaternion(const Super_T &v) : Super_T(v) {}
    Quaternion(T x, T y, T z, T w) : Super_T(x, y, z, w) {}
    Quaternion(T w, Vector<3, T> vec) : Super_T(vec.x(), vec.y(), vec.z(), w) {}

    Quaternion(const AxisAngle<T> &aa) {
      double hAngle = aa.angle() / 2;
      Vector3D v = aa.getVector().normalize();
      T s = sin(hAngle);
      x() = v.x() * s;
      y() = v.y() * s;
      z() = v.z() * s;
      w() = cos(hAngle);
    }

    const T &w() const {return data[3];}
    T &w() {return data[3];}

    Vector<3, T> getVector() const {return Vector<3, T>(x(), y(), z());}

    AxisAngle<T> toAxisAngle() const {
      if (!w()) return AxisAngle<T>();
      T s = sqrt(1 - w() * w());
      return AxisAngle<T>(2 * acos(w()), x() / s, y() / s, z() / s);
    }

    Vector<3, T> rotate(const Vector<3, T> &p) const {
      return multiply(Quaternion<T>(0, p)).multiply(inverse()).getVector();
    }

    Quaternion<T> conjugate() const {
      return Quaternion<T>(-x(), -y(), -z(), w());
    }

    Quaternion<T> inverse() const {
      return Quaternion<T>(Super_T::normalize()).conjugate() /
        Super_T::length();
    }

    Quaternion<T> multiply(const Quaternion &q) const {
      return
        Quaternion<T>(w() * q.x() + x() * q.w() + y() * q.z() - z() * q.y(),
                      w() * q.y() + y() * q.w() + z() * q.x() - x() * q.z(),
                      w() * q.z() + z() * q.w() + x() * q.y() - y() * q.x(),
                      w() * q.w() - x() * q.x() - y() * q.y() - z() * q.z());
    }
  };


  typedef Quaternion<float> QuaternionF;
  typedef Quaternion<double> QuaternionD;
}

#endif // CBANG_QUATERNION_H
