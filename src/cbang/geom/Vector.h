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

#ifndef CB_VECTOR_H
#define CB_VECTOR_H

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/Math.h>

#include <cbang/json/List.h>
#include <cbang/json/Sink.h>

#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>

#if defined(_WIN32) && defined(max)
#undef max
#endif
#if defined(_WIN32) && defined(min)
#undef min
#endif


namespace cb {
  template <const unsigned DIM, typename T>
  class Vector {
  public:
    T data[DIM];

    Vector(T v = 0) {for (unsigned i = 0; i < DIM; i++) data[i] = v;}

    Vector(const T data[DIM])
    {for (unsigned i = 0; i < DIM; i++) this->data[i] = data[i];}

    Vector(const Vector<DIM, T> &v)
    {for (unsigned i = 0; i < DIM; i++) data[i] = v.data[i];}

    Vector(const Vector<DIM - 1, T> &v, T x) {
      if (DIM < 1)
        CBANG_THROWS("Invalid constructor for Vector of dimension " << DIM);
      for (unsigned i = 0; i < DIM - 1; i++) data[i] = v.data[i];
      data[DIM - 1] = x;
    }

    Vector(const Vector<DIM - 2, T> &v, T x, T y) {
      if (DIM < 2)
        CBANG_THROWS("Invalid constructor for Vector of dimension " << DIM);
      for (unsigned i = 0; i < DIM - 2; i++) data[i] = v.data[i];
      data[DIM - 2] = x;
      data[DIM - 1] = y;
    }

    Vector(const Vector<DIM - 3, T> &v, T x, T y, T z) {
      if (DIM < 3)
        CBANG_THROWS("Invalid constructor for Vector of dimension " << DIM);
      for (unsigned i = 0; i < DIM - 3; i++) data[i] = v.data[i];
      data[DIM - 3] = x;
      data[DIM - 2] = y;
      data[DIM - 1] = z;
    }

    explicit Vector(const std::string &s) {
      std::vector<std::string> tokens;
      String::tokenize(s, tokens, "(,; \t\n\r)");
      if (tokens.size() != DIM)
        CBANG_THROWS("Invalid Vector<" << DIM << "> string '" << s << "'");

      for (unsigned i = 0; i < DIM; i++)
        data[i] = (T)String::parseDouble(tokens[i]);
    }

    Vector(T x, T y) {
      if (DIM != 2)
        CBANG_THROWS("Invalid constructor for Vector of dimension " << DIM);
      data[0] = x;
      data[1] = y;
    }

    Vector(T x, T y, T z) {
      if (DIM != 3)
        CBANG_THROWS("Invalid constructor for Vector of dimension " << DIM);
      data[0] = x;
      data[1] = y;
      data[2] = z;
    }

    Vector(T x, T y, T z, T a) {
      if (DIM != 4)
        CBANG_THROWS("Invalid constructor for Vector of dimension " << DIM);
      data[0] = x;
      data[1] = y;
      data[2] = z;
      data[3] = a;
    }


    unsigned getSize() const {return DIM;}


    T &operator[](unsigned i) {return data[i];}
    const T &operator[](unsigned i) const {return data[i];}

    T &x() {return data[0];}
    T x() const {return data[0];}
    T &y() {
      if (DIM < 2)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return data[1];
    }
    T y() const {
      if (DIM < 2)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return data[1];
    }
    T &z() {
      if (DIM < 3)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return data[2];
    }
    T z() const {
      if (DIM < 3)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return data[2];
    }


    template <unsigned LEN>
    Vector<LEN, T> slice(unsigned start = 0) const {
      Vector<LEN, T> result;

      for (unsigned i = 0; i < DIM - start && i < LEN; i++)
        result[i] = data[start + i];

      return result;
    }

    void clear() {for (unsigned i = 0; i < DIM; i++) data[i] = 0;}

    void reverse() {
      for (unsigned i = 0; i < DIM / 2; i++)
        std::swap(data[i], data[DIM - i - 1]);
    }


    T lengthSquared() const {
      T result = 0;
      for (unsigned i = 0; i < DIM; i++) result += data[i] * data[i];
      return result;
    }

    T length() const {return sqrt(lengthSquared());}


    Vector<DIM, T> normalize() const {
      return *this / length();
    }


    T distanceSquared(const Vector<DIM, T> &v) const {
      T d = 0;
      for (unsigned i = 0; i < DIM; i++)
        d += (data[i] - v.data[i]) * (data[i] - v.data[i]);
      return d;
    }

    T distance(const Vector<DIM, T> &v) const {
      return sqrt(distanceSquared(v));
    }


    Vector<DIM, T> crossProduct(const Vector<DIM, T> &v) const {
      if (DIM != 3)
        CBANG_THROWS("Invalid operation for Vector of dimension " << DIM);
      return Vector<DIM, T>(data[1] * v.data[2] - data[2] * v.data[1],
                            data[2] * v.data[0] - data[0] * v.data[2],
                            data[0] * v.data[1] - data[1] * v.data[0]);
    }

    Vector<DIM, T> cross(const Vector<DIM, T> &v) const {
      return crossProduct(v);
    }

    T dotProduct(const Vector<DIM, T> &v) const {
      T result = 0;
      for (unsigned i = 0; i < DIM; i++) result += data[i] * v.data[i];
      return result;
    }

    T dot(const Vector<DIM, T> &v) const {return dotProduct(v);}


    T angleBetween(const Vector<DIM, T> &v) const {
      if (DIM == 2) {
        T angle = atan2(v.y(), v.x()) - atan2(y(), x());
        if (angle < 0) angle += 2 * M_PI;

        return fmod(2 * M_PI - angle, 2 * M_PI);
      }

      return acos(normalize().dotProduct(v.normalize()));
    }


    Vector<DIM, T> intersect(const Vector<DIM, T> &v, T distance) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++)
        result[i] = data[i] + distance * (v.data[i] - data[i]);
      return result;
    }


    unsigned findLargest() const {
      unsigned index = 0;
      T value = data[0];
      for (unsigned i = 1; i < DIM; i++)
        if (value < data[i]) {
          value = data[i];
          index = i;
        }

      return index;
    }

    unsigned findSmallest() const {
      unsigned index = 0;
      T value = data[0];
      for (unsigned i = 1; i < DIM; i++)
        if (data[i] < value) {
          value = data[i];
          index = i;
        }

      return index;
    }


    // Cast
    template <typename U>
    operator Vector<DIM, U>() const {
      Vector<DIM, U> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i];
      return result;
    }

    // Uniary
    Vector<DIM, T> operator-() const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = -data[i];
      return result;
    }

    bool operator!() const {
      for (unsigned i = 0; i < DIM; i++) if (data[i]) return false;
      return true;
    }


    // Arithmetic
    Vector<DIM, T> operator+(const Vector<DIM, T> &v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] + v.data[i];
      return result;
    }

    Vector<DIM, T> operator-(const Vector<DIM, T> &v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] - v.data[i];
      return result;
    }

    Vector<DIM, T> operator*(const Vector<DIM, T> &v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] * v.data[i];
      return result;
    }

    Vector<DIM, T> operator/(const Vector<DIM, T> &v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] / v.data[i];
      return result;
    }


    Vector<DIM, T> operator+(T v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] + v;
      return result;
    }

    Vector<DIM, T> operator-(T v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] - v;
      return result;
    }

    Vector<DIM, T> operator*(T v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] * v;
      return result;
    }

    Vector<DIM, T> operator/(T v) const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = data[i] / v;
      return result;
    }


    // Assignment
    Vector<DIM, T> &operator=(const Vector<DIM, T> &v) {
      for (unsigned i = 0; i < DIM; i++) data[i] = v.data[i];
      return *this;
    }

    Vector<DIM, T> &operator+=(const Vector<DIM, T> &v) {
      for (unsigned i = 0; i < DIM; i++) data[i] += v.data[i];
      return *this;
    }

    Vector<DIM, T> &operator-=(const Vector<DIM, T> &v) {
      for (unsigned i = 0; i < DIM; i++) data[i] -= v.data[i];
      return *this;
    }

    Vector<DIM, T> &operator*=(const Vector<DIM, T> &v) {
      for (unsigned i = 0; i < DIM; i++) data[i] *= v.data[i];
      return *this;
    }

    Vector<DIM, T> &operator/=(const Vector<DIM, T> &v) {
      for (unsigned i = 0; i < DIM; i++) data[i] /= v.data[i];
      return *this;
    }


    Vector<DIM, T> &operator+=(T v) {
      for (unsigned i = 0; i < DIM; i++) data[i] += v;
      return *this;
    }

    Vector<DIM, T> &operator-=(T v) {
      for (unsigned i = 0; i < DIM; i++) data[i] -= v;
      return *this;
    }

    Vector<DIM, T> &operator*=(T v) {
      for (unsigned i = 0; i < DIM; i++) data[i] *= v;
      return *this;
    }

    Vector<DIM, T> &operator/=(T v) {
      for (unsigned i = 0; i < DIM; i++) data[i] /= v;
      return *this;
    }


    // Comparison
    bool operator==(const Vector<DIM, T> &v) const {
      for (unsigned i = 0; i < DIM; i++) if (data[i] != v.data[i]) return false;
      return true;
    }

    bool operator!=(const Vector<DIM, T> &v) const {
      return !(*this == v);
    }

    bool operator<(const Vector<DIM, T> &v) const {
      for (unsigned i = 0; i < DIM; i++)
        if (data[i] < v.data[i]) return true;
        else if (v.data[i] < data[i]) return false;

      return false;
    }

    bool operator<=(const Vector<DIM, T> &v) const {
      for (unsigned i = 0; i < DIM; i++)
        if (data[i] < v.data[i]) return true;
        else if (v.data[i] < data[i]) return false;

      return true;
    }

    bool operator>(const Vector<DIM, T> &v) const {
      return v <= *this;
    }

    bool operator>=(const Vector<DIM, T> &v) const {
      return v < *this;
    }


    // Math
    template <T (*F)(T)>
    Vector<DIM, T> apply() const {
      Vector<DIM, T> result;
      for (unsigned i = 0; i < DIM; i++) result[i] = F(data[i]);
      return result;
    }

    Vector<DIM, T> abs() const {return apply<std::abs>();}
    Vector<DIM, T> ceil() const {return apply<std::ceil>();}
    Vector<DIM, T> floor() const {return apply<std::floor>();}

    T min() const {
      T value = std::numeric_limits<T>::max();
      for (unsigned i = 0; i < DIM; i++) if (data[i] < value) value = data[i];
      return value;
    }

    T max() const {
      T value = std::numeric_limits<T>::min();
      for (unsigned i = 0; i < DIM; i++) if (value < data[i]) value = data[i];
      return value;
    }

    bool isReal() const {
      for (unsigned i = 0; i < DIM; i++)
        if (cb::Math::isnan(data[i]) || cb::Math::isinf(data[i]))
          return false;
      return true;
    }

    // JSON
    cb::SmartPointer<cb::JSON::Value> getJSON() const {
      SmartPointer<JSON::List> list = new JSON::List;
      for (unsigned i = 0; i < DIM; i++) list->append(data[i]);
      return list;
    }

    void loadJSON(const cb::JSON::Value &value) {read(value);}

    void read(const cb::JSON::Value &value) {
      const JSON::List &list = value.getList();

      if (list.size() != DIM)
        CBANG_THROWS("Vector<" << DIM << "> expected list of length " << DIM);
      for (unsigned i = 0; i < DIM; i++) data[i] = list[i]->getNumber();
    }

    void write(JSON::Sink &sink) const {
      sink.beginList();
      for (unsigned i = 0; i < DIM; i++) sink.append(data[i]);
      sink.endList();
    }


    std::ostream &print(std::ostream &stream) const {
      for (unsigned i = 0; i < DIM; i++)
        stream << (i ? ',' : '(') << data[i];
      return stream << ')';
    }


    std::string toString() const {
      std::ostringstream s;
      print(s);
      return s.str();
    }
  };


  template<const unsigned DIM, typename T> static inline
  std::ostream &operator<<(std::ostream &stream, const Vector<DIM, T> &v) {
    return v.print(stream);
  }


  typedef Vector<2, int> Vector2I;
  typedef Vector<2, unsigned> Vector2U;
  typedef Vector<2, double> Vector2D;
  typedef Vector<2, float> Vector2F;

  typedef Vector<3, int> Vector3I;
  typedef Vector<3, unsigned> Vector3U;
  typedef Vector<3, double> Vector3D;
  typedef Vector<3, float> Vector3F;

  typedef Vector<4, int> Vector4I;
  typedef Vector<4, unsigned> Vector4U;
  typedef Vector<4, double> Vector4D;
  typedef Vector<4, float> Vector4F;
}

#endif // CB_VECTOR_H

