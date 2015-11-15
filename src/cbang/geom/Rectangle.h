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

#ifndef CBANG_RECTANGLE_H
#define CBANG_RECTANGLE_H

#include "Vector.h"
#include "Segment.h"

#include <algorithm>
#include <limits>

#if defined(_WIN32) && defined(max)
#undef max
#endif
#if defined(_WIN32) && defined(min)
#undef min
#endif


namespace cb {
  /// Really an axis aligned box
  template <const unsigned DIM, typename T>
  class Rectangle {
  public:
    Vector<DIM, T> rmin;
    Vector<DIM, T> rmax;

    Rectangle() : rmin(std::numeric_limits<T>::max()),
                  rmax(-std::numeric_limits<T>::max()) {}

    Rectangle(const Rectangle<DIM, T> &o) : rmin(o.rmin), rmax(o.rmax) {}
    Rectangle(const Vector<DIM, T> &p1, const Vector<DIM, T> &p2) :
      rmin(p1), rmax(p2) {
      for (unsigned i = 0; i < DIM; i++)
        if (rmax[i] < rmin[i]) std::swap(rmax[i], rmin[i]);
    }

    const Vector<DIM, T> &getMin() const {return rmin;}
    const Vector<DIM, T> &getMax() const {return rmax;}

    T getWidth() const {return rmax[0] - rmin[0];}
    T getLength() const {
      if (DIM < 2)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return rmax[1] - rmin[1];
    }
    T getHeight() const {
      if (DIM < 3)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return rmax[2] - rmin[2];
    }

    T getVolume() const {
      T volume = 1;
      for (unsigned i = 0; i < DIM; i++) volume *= getDimension(i);
      return volume;
    }


    T getDimension(unsigned i) const {return rmax[i] - rmin[i];}

    Vector<DIM, T> getDimensions() const {
      Vector<DIM, T> d;
      for (unsigned i = 0; i < DIM; i++) d[i] = getDimension(i);
      return d;
    }

    Vector<DIM, T> getCenter() const {return (rmin + rmax) / 2.0;}

    Vector<DIM, T> closestPoint(const Vector<DIM, T> &p) const {
      Vector<DIM, T> mid = getCenter();
      Vector<DIM, T> half = mid - rmin;
      Vector<DIM, T> closest;

      for (unsigned i = 0; i < DIM; i++)
        closest[i] =
          std::min(std::max(p[i], mid[i] - half[i]), mid[i] + half[i]);

      return closest;
    }

    Vector<DIM, T> closestPointOnSurface(const Vector<DIM, T> &p) const {
      if (!contains(p)) return closestPoint(p);

      // Find closest point on surface, given that p is inside

      // First find the closest face
      float faceDist[DIM * 2];
      for (unsigned i = 0; i < DIM; i++) {
        faceDist[i * 2] = p[i] - rmin[i];
        faceDist[i * 2 + 1] = rmax[i] - p[i];
      }

      unsigned face = 0;
      for (unsigned i = 1; i < DIM * 2; i++)
        if (faceDist[i] < faceDist[face]) face = i;

      // The closest point is on the closest face
      Vector<DIM, T> closest = p;
      if (face & 1) closest[face / 2] = rmax[face / 2];
      else closest[face / 2] = rmin[face / 2];

      return closest;
    }


    bool contains(const Vector<DIM, T> &p) const {
      for (unsigned i = 0; i < DIM; i++)
        if ((p[i] < rmin[i]) || (rmax[i] < p[i])) return false;
      return true;
    }

    bool contains(const Rectangle<DIM, T> &r) const {
      return contains(r.rmin) && contains(r.rmax);
    }

    bool contains(const Segment<DIM, T> &s) const {
      return contains(s.p1) && contains(s.p2);
    }


    Rectangle<DIM, T> intersection(const Rectangle<DIM, T> &o) const {
      Rectangle<DIM, T> r;

      for (unsigned i = 0; i < DIM; i++) {
        r.rmax[i] = std::min(rmax[i], o.rmax[i]);
        r.rmin[i] = std::max(rmin[i], o.rmin[i]);
      }

      return r;
    }


    bool intersects(const Rectangle<DIM, T> &o) const {
      for (unsigned i = 0; i < DIM; i++)
        if ((o.rmax[i] < rmin[i]) || (rmax[i] < o.rmin[i])) return false;
      return true;
    }

    bool intersects(const Segment<DIM, T> &s, Vector<DIM, T> &p1,
                    Vector<DIM, T> &p2) const {
      // TODO Currently only implemented for the 2D case
      if (DIM != 2)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);

      Vector<DIM, T> nil(std::numeric_limits<T>::max(),
                         std::numeric_limits<T>::max());
      p1 = p2 = nil;

      Segment<DIM, T> segs[4] = {
        Segment<DIM, T>(Vector<DIM, T>(rmin.x(), rmin.y()),
                        Vector<DIM, T>(rmax.x(), rmin.y())),
        Segment<DIM, T>(Vector<DIM, T>(rmax.x(), rmin.y()),
                        Vector<DIM, T>(rmax.x(), rmax.y())),
        Segment<DIM, T>(Vector<DIM, T>(rmax.x(), rmax.y()),
                        Vector<DIM, T>(rmin.x(), rmax.y())),
        Segment<DIM, T>(Vector<DIM, T>(rmin.x(), rmax.y()),
                        Vector<DIM, T>(rmin.x(), rmin.y())),
      };

      Vector<DIM, T> x;
      for (int i = 0; i < 4; i++)
        if (segs[i].intersection(s, x))
          if (x != p1 && x != p2) {
            if (p1 == nil) p1 = x;
            else if (p2 == nil) p2 = x;
            else break;
          }

      return p1 != nil;
    }


    void add(const Vector<DIM, T> &p) {
      for (unsigned i = 0; i < DIM; i++) {
        if (rmax[i] < p[i]) rmax[i] = p[i];
        if (p[i] < rmin[i]) rmin[i] = p[i];
      }
    }

    void add(const Rectangle<DIM, T> &r) {
      for (unsigned i = 0; i < DIM; i++) {
        if (r.rmin[i] < rmin[i]) rmin[i] = r.rmin[i];
        if (rmax[i] < r.rmax[i]) rmax[i] = r.rmax[i];
      }
    }


    template <unsigned LEN>
    Rectangle<LEN, T> slice(unsigned start = 0) const {
      return Rectangle<LEN, T>(rmin.template slice<LEN>(start),
                               rmax.template slice<LEN>(start));
    }


    Rectangle<DIM, T> grow(const Vector<DIM, T> &amount) const {
      return Rectangle<DIM, T>(rmin - amount, rmax + amount);
    }


    Rectangle<DIM, T> shrink(const Vector<DIM, T> &amount) const {
      return Rectangle<DIM, T>(rmin + amount, rmax - amount);
    }

    const Vector<DIM, T> &operator[](unsigned i) const {
      if (1 < i) CBANG_THROWS("Invalid Rectangle index" << i);
      return i ? rmax : rmin;
    }

    Vector<DIM, T> &operator[](unsigned i) {
      if (1 < i) CBANG_THROWS("Invalid Rectangle index " << i);
      return i ? rmax : rmin;
    }

    // Compare
    bool operator==(const Rectangle<DIM, T> &r) const {
      return rmin == r.rmin && rmax == r.rmax;
    }

    bool operator!=(const Rectangle<DIM, T> &r) const {
      return rmin != r.rmin || rmax != r.rmax;
    }

    // Arithmetic
    Rectangle<DIM, T> operator+(const Rectangle<DIM, T> &r) const {
      return Rectangle<DIM, T>(getMin() + r.getMin(), getMax() + r.getMax());
    }

    Rectangle<DIM, T> operator-(const Rectangle<DIM, T> &r) const {
      return Rectangle<DIM, T>(getMin() - r.getMin(), getMax() - r.getMax());
    }

    Rectangle<DIM, T> operator*(const Rectangle<DIM, T> &r) const {
      return Rectangle<DIM, T>(getMin() * r.getMin(), getMax() * r.getMax());
    }

    Rectangle<DIM, T> operator/(const Rectangle<DIM, T> &r) const {
      return Rectangle<DIM, T>(getMin() / r.getMin(), getMax() / r.getMax());
    }


    Rectangle<DIM, T> operator+(const Vector<DIM, T> &v) const {
      return Rectangle<DIM, T>(getMin() + v, getMax() + v);
    }

    Rectangle<DIM, T> operator-(const Vector<DIM, T> &v) const {
      return Rectangle<DIM, T>(getMin() - v, getMax() - v);
    }

    Rectangle<DIM, T> operator*(const Vector<DIM, T> &v) const {
      return Rectangle<DIM, T>(getMin() * v, getMax() * v);
    }

    Rectangle<DIM, T> operator/(const Vector<DIM, T> &v) const {
      return Rectangle<DIM, T>(getMin() / v, getMax() / v);
    }


    Rectangle<DIM, T> operator+(T x) const {
      return Rectangle<DIM, T>(getMin() + x, getMax() + x);
    }

    Rectangle<DIM, T> operator-(T x) const {
      return Rectangle<DIM, T>(getMin() - x, getMax() - x);
    }

    Rectangle<DIM, T> operator*(T x) const {
      return Rectangle<DIM, T>(getMin() * x, getMax() * x);
    }

    Rectangle<DIM, T> operator/(T x) const {
      return Rectangle<DIM, T>(getMin() / x, getMax() / x);
    }

    // Math
    bool isReal() const {return rmin.isReal() || rmax.isReal();}

    bool isEmpty() const {
      for (unsigned i = 0; i < DIM; i++)
        if (rmin[i] == rmax[i]) return true;
      return false;
    }

    // Cast
    template <typename U>
    operator Rectangle<DIM, U>() const {
      return Rectangle<DIM, U>(rmin, rmax);
    }

    std::ostream &print(std::ostream &stream) const
    {return stream << '(' << rmin << ", " << rmax << ')';}


    void write(JSON::Sink &sink) const {
      sink.beginDict();
      sink.beginInsert("min");
      rmin.write(sink);
      sink.beginInsert("max");
      rmax.write(sink);
      sink.endDict();
    }
  };


  template<const unsigned DIM, typename T> static inline
  std::ostream &operator<<(std::ostream &stream, const Rectangle<DIM, T> &r) {
    return r.print(stream);
  }


  typedef Rectangle<2, int> Rectangle2I;
  typedef Rectangle<2, unsigned> Rectangle2U;
  typedef Rectangle<2, double> Rectangle2D;
  typedef Rectangle<2, float> Rectangle2F;

  typedef Rectangle<3, int> Rectangle3I;
  typedef Rectangle<3, unsigned> Rectangle3U;
  typedef Rectangle<3, double> Rectangle3D;
  typedef Rectangle<3, float> Rectangle3F;
}

#endif // CBANG_RECTANGLE_H

