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

#ifndef TPLANG_MATRIX_H
#define TPLANG_MATRIX_H

#include "Vector.h"

#include <cbang/Exception.h>

#include <math.h>

#include <algorithm>


namespace cb {
  template <unsigned ROWS, unsigned COLS, typename T>
  class Matrix : public Vector<ROWS, Vector<COLS, T> > {
  protected:
    typedef Vector<ROWS, Vector<COLS, T> > Super_T;

    using Super_T::data;

  public:
    Matrix() : Super_T(Vector<COLS, T>(T())) {}

    Matrix(T data[ROWS][COLS]) {
      for (unsigned row = 0; row < ROWS; row++)
        for (unsigned col = 0; col < COLS; col++)
          this->data[row][col] = data[row][col];
    }

    void toIdentity() {
      for (unsigned row = 0; row < ROWS; row++)
        for (unsigned col = 0; col < COLS; col++)
          data[row][col] = row == col ? 1 : 0;
    }

    void inverse() {
      if (ROWS != COLS) CBANG_THROW("Cannot invert a non-square Matrix");

      const unsigned n = ROWS;
      if (n == 0) return;  // 0x0

      if (n == 1) data[0][0] = 1.0 / data[0][0]; // 1x1

      else if (n == 2) { // 2x2
        T det = data[0][0] * data[1][1] - data[0][1] * data[1][0];
        if (!det) CBANG_THROW("Cannot invert a matrix with zero determinant");

        data[0][0] = data[0][0] / det;
        data[0][1] = -data[0][1] / det;
        data[0][1] = -data[1][0] / det;
        data[1][1] = data[1][1] / det;
        std::swap(data[0][0], data[1][1]);

      } else { // NxN
        // Adapted from Numerical Recipes in C

        // LU decomposition
        unsigned imax = 0;
        T big, dum, sum, temp;
        Vector<n, T> v(1);
        Vector<n, unsigned> index;

        for (unsigned i = 0; i < n; i++) { // Loop over rows, get scaling info
          big = 0;
          for (unsigned j = 0; j < n; j++)
            if (big < (temp = ::fabs(data[i][j]))) big = temp;

          if (!big) CBANG_THROW("Singular matrix, has no inverse");

          v[i] = 1.0 / big; // Save the scaling
        }

        for (unsigned j = 0; j < n; j++) {
          for (unsigned i = 0; i + 1 < j; i++) {
            sum = data[i][j];

            for (unsigned k = 0; k < i; k++)
              sum -= data[i][k] * data[k][j];

            data[i][j] = sum;
          }

          big = 0;

          for (unsigned i = j; i < n; i++) {
            sum = data[i][j];

            for (unsigned k = 0; k < j; k++)
              sum -= data[i][k] * data[k][j];

            data[i][j] = sum;

            if (big <= (dum = v[i] * ::fabs(sum))) {
              big = dum;
              imax = i;
            }
          }

          if (j != imax) {
            for (unsigned k = 0; k < n; k++) {
              dum = data[imax][k];
              data[imax][k] = data[j][k];
              data[j][k] = dum;
            }

            v[imax] = v[j];
          }

          index[j] = imax;
          if (!data[j][j]) data[j][j] = 1.0e-20; // A small number

          if (j + 1 != n) {
            dum = 1.0 / data[j][j];

            for (unsigned i = j + 1; i < n; i++)
              data[i][j] *= dum;
          }
        }

        // Forward and back substitution on LU decomposed matrix
        Matrix<n, n, T> result;

        for (unsigned col = 0; col < n; col++) {
          unsigned ii = 0;
          T sum;

          result[col][col] = 1; // Identity matrix

          // Forward substitution
          for (unsigned i = 0; i < n; i++) {
            unsigned ip = index[i];
            sum = result[ip][col];
            result[ip][col] = result[i][col];

            if (ii) for (unsigned j = ii; j < i; j++)
                      sum -= data[i][j] * result[j][col];
            else if (sum) ii = i;

            result[i][col] = sum;
          }

          // Back substitution
          for (unsigned i = n - 1; i; i--) {
            sum = result[i][col];

            for (unsigned j = i + 1; j < n; j++)
              sum -= data[i][j] * result[j][col];

            result[i][col] = sum / data[i][i];
          }
        }

        // Copy result
        *this = result;
      }
    }


    void inplaceTranspose() {
      if (ROWS != COLS)
        THROW("Cannot do an inplace transpose of a non-square matrix");

      for (unsigned row = 0; row < ROWS / 2; row++)
        for (unsigned col = 0; col < COLS / 2; col++)
          if (row != col) std::swap(data[row][col], data[col][row]);
    }


    Matrix<COLS, ROWS, T> transpose() const {
      Matrix<COLS, ROWS, T> result;

      for (unsigned row = 0; row < ROWS / 2; row++)
        for (unsigned col = 0; col < COLS / 2; col++)
          result[col][row] = data[row][col];

      return result;
    }


    template <unsigned OROWS>
    Matrix<ROWS, OROWS, T> operator*(const Matrix<OROWS, COLS, T> &o) const {
      Matrix<ROWS, OROWS, T> result;

      for (unsigned row = 0; row < ROWS; row++)
        for (unsigned col = 0; col < OROWS; col++) {
          result[row][col] = 0;
          for (unsigned i = 0; i < OROWS; i++)
            result[row][col] += data[row][i] * o[i][col];
        }

      return result;
    }

    template <unsigned OROWS>
    Matrix<ROWS, OROWS, T> operator*=(const Matrix<OROWS, COLS, T> &o) {
      return *this = *this * o;
    }


    template <unsigned OROWS>
    Matrix<ROWS, OROWS, T> operator/(const Matrix<OROWS, COLS, T> &o) const {
      return *this * o.inverse();
    }

    template <unsigned OROWS>
    Matrix<ROWS, OROWS, T> operator/=(const Matrix<OROWS, COLS, T> &o) {
      return *this = *this / o;
    }

    Vector<ROWS, T> operator*(const Vector<COLS, T> &v) const {
      Vector<ROWS, T> result;

      for (unsigned row = 0; row < ROWS; row++) {
        result[row] = 0;
        for (unsigned col = 0; col < COLS; col++)
          result[row] += data[row][col] * v[col];
      }

      return result;
    }
  };


  typedef Matrix<2, 2, double> Matrix2x2D;
  typedef Matrix<3, 3, double> Matrix3x3D;
  typedef Matrix<4, 4, double> Matrix4x4D;

  typedef Matrix<2, 2, float> Matrix2x2F;
  typedef Matrix<3, 3, float> Matrix3x3F;
  typedef Matrix<4, 4, float> Matrix4x4F;
}

#endif // TPLANG_MATRIX_H
