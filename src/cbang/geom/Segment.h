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

#ifndef CBANG_SEGMENT_H
#define CBANG_SEGMENT_H

#include "Vector.h"


namespace cb {
  template <const unsigned DIM, typename T>
  class Segment : public Vector<2, Vector<DIM, T> > {
  public:
    typedef Vector<2, Vector<DIM, T> > Super_T;
    using Super_T::data;

    Segment() : Super_T(Vector<DIM, T>()) {}
    Segment(const Super_T &v) : Super_T(v) {}
    Segment(const Vector<DIM, T> &p1, const Vector<DIM, T> &p2) :
      Vector<2, Vector<DIM, T> >(p1, p2) {}

    const Vector<DIM, T> &getStart() const {return data[0];}
    Vector<DIM, T> &getStart() {return data[0];}
    const Vector<DIM, T> &getEnd() const {return data[1];}
    Vector<DIM, T> &getEnd() {return data[1];}

    T length() const {return data[0].distance(data[1]);}
    T lengthSquared() const {return data[0].distanceSquared(data[1]);}


    bool intersection(const Segment<DIM, T> &s, Vector<DIM, T> &p) const {
      // TODO Currently only implemented for the 2D case
      if (DIM != 2)
        CBANG_THROWS("Invalid operation for Segment of dimension " << DIM);

      T d = (s.data[1][1] - s.data[0][1]) * (data[1][0] - data[0][0]) -
        (s.data[1][0] - s.data[0][0]) * (data[1][1] - data[0][1]);

      if (!d) return false; // Parallel

      d = 1.0 / d;

      T ua = ((s.data[1][0] - s.data[0][0]) * (data[0][1] - s.data[0][1]) -
              (s.data[1][1] - s.data[0][1]) * (data[0][0] - s.data[0][0])) * d;
      if (ua <= 0 || 1 <= ua) return false;

      T ub = ((data[1][0] - data[0][0]) * (data[0][1] - s.data[0][1]) -
              (data[1][1] - data[0][1]) * (data[0][0] - s.data[0][0])) * d;
      if (ub <= 0 || 1 <= ub) return false;

      p[0] = data[0][0] + ua * (data[1][0] - data[0][0]);
      p[1] = data[0][1] + ua * (data[1][1] - data[0][1]);

      return true;
    }


    bool intersects(const Segment<DIM, T> &s) const {
      Vector<DIM, T> p;
      return intersection(s, p);
    }


    T distance(const Vector<DIM, T> &p) const {
      return p.distance(closest(p));
    }

    T distance(const Vector<DIM, T> &p, Vector<DIM, T> &c) const {
      return p.distance(c = closest(p));
    }

    Vector<DIM, T> closest(const Vector<DIM, T> &p) const {
      T len2 = lengthSquared();
      if (!len2) return data[0];

      T t = (p - data[0]).dot(data[1] - data[0]) / len2;

      if (t <= 0) return data[0];
      if (1 <= t) return data[1];

      return data[0] + (data[1] - data[0]) * t;
    }

    Vector<DIM, T> closest(const Vector<DIM, T> &p, T &dist) const {
      Vector<DIM, T> c = closest(p);
      dist = p.distance(c);
      return c;
    }


    // Cast
    template <typename U>
    operator Segment<DIM, U>() const {return Segment<DIM, U>(data[0], data[1]);}
  };


  typedef Segment<2, int> Segment2I;
  typedef Segment<2, unsigned> Segment2U;
  typedef Segment<2, double> Segment2D;
  typedef Segment<2, float> Segment2F;

  typedef Segment<3, int> Segment3I;
  typedef Segment<3, unsigned> Segment3U;
  typedef Segment<3, double> Segment3D;
  typedef Segment<3, float> Segment3F;
}

#endif // CBANG_SEGMENT_H

