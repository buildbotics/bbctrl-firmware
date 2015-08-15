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

#include "Transaction.h"

#include "Database.h"

#include <cbang/util/DefaultCatch.h>
#include <cbang/time/Timer.h>

using namespace cb::DB;


Transaction::~Transaction() {
  if (!db) return;

  // Retry for at most timeout seconds
  double start = Timer::now();

  while (true) {
    try {
      db->rollback();
      return;
    } CBANG_CATCH_WARNING;

    if (timeout < Timer::now() - start) break;
    Timer::sleep(0.25);
  }

  LOG_ERROR("Failed to roll back DB transaction, continuing anyway");
}


void Transaction::commit() {
  if (!db) THROW("Database was released before commit");
  db->commit();
  release();
}
