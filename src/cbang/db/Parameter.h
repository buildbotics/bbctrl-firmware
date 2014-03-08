/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
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

#ifndef CBANG_DB_PARAMETER_H
#define CBANG_DB_PARAMETER_H

#include <cbang/StdTypes.h>

struct sqlite3_stmt;

namespace cb {
  namespace DB {
    class Blob;

    class Parameter {
      sqlite3_stmt *stmt;
      unsigned i;

    public:
      Parameter(sqlite3_stmt *stmt, unsigned i) : stmt(stmt), i(i) {}

      unsigned getIndex() const {return i;}
      const char *getName() const;

      void bind(const Blob &x) const;
      void bind(double x) const;
      void bind(float x) const {bind((double)x);}
      void bind(int64_t x) const;
      void bind(uint64_t x) const {bind((int64_t)x);}
      void bind(int32_t x) const {bind((int64_t)x);}
      void bind(uint32_t x) const {bind((int64_t)x);}
      void bind(const std::string &x) const;
      void bind() const;

      const Blob &operator=(const Blob &x) {bind(x); return x;}
      double operator=(double x) {bind(x); return x;}
      int64_t operator=(int64_t x) {bind(x); return x;}
      const std::string &operator=(const std::string &x) {bind(x); return x;}

    protected:
      void error(const std::string &msg, int err) const;
    };
  }
}

#endif // CBANG_DB_PARAMETER_H

