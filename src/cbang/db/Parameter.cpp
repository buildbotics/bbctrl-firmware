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

#include "Parameter.h"

#include "Blob.h"
#include "Database.h"

#include <cbang/Exception.h>

#include <sqlite3.h>

using namespace std;
using namespace cb;
using namespace cb::DB;


const char *Parameter::getName() const {
  const char *name = sqlite3_bind_parameter_name(stmt, i);
  return name ? name : "<unknown>";
}


void Parameter::bind(const Blob &x) const {
  int ret =
    sqlite3_bind_blob(stmt, i, x.getData(), x.getLength(), SQLITE_TRANSIENT);
  if (ret) error("Failed to bind Blob", ret);
}


void Parameter::bind(double x) const {
  int ret = sqlite3_bind_double(stmt, i, x);
  if (ret) error("Failed to bind Double", ret);
}


void Parameter::bind(int64_t x) const {
  int ret = sqlite3_bind_int64(stmt, i, x);
  if (ret) error("Failed to bind Integer", ret);
}


void Parameter::bind(const string &x) const {
  int ret = sqlite3_bind_text(stmt, i, x.c_str(), -1, SQLITE_TRANSIENT);
  if (ret) error("Failed to bind String", ret);
}


void Parameter::bind() const {
  int ret = sqlite3_bind_null(stmt, i);
  if (ret) error("Failed to bind NULL", ret);
}


void Parameter::error(const string &msg, int err) const {
  THROWS(msg << ": in parameter '" << getName() << "' (" << i << "): "
         << Database::errorMsg(err));
}
