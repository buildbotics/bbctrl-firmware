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

#include "Arguments.h"

#include "Array.h"
#include "Object.h"
#include "Context.h"

#include <cbang/json/Builder.h>
#include <cbang/log/Logger.h>

#include <duktape.h>

using namespace cb::duk;
using namespace cb;
using namespace std;


Arguments::Arguments(Context &ctx, const Signature &sig) : ctx(ctx), sig(sig) {
  JSON::Builder builder(SmartPointer<JSON::Value>::Phony(this));

  // Load single Object as arguments
  if (ctx.top() == 1 && ctx.isObject())
    ctx.toObject().insertValues(builder);

  else { // Add positional arguments
    int index = 0;

    for (unsigned i = 0; i < ctx.top(); i++) {
      // Begin insert
      if (i < sig.size()) builder.beginInsert(sig.keyAt(i));
      else {
        while (has(String(index))) index++;
        builder.beginInsert(String(index++));
      }

      // Write value
      if (ctx.isUndefined(i) && i < sig.size()) builder.write(*sig.get(i));
      else ctx.write(i, builder);
    }
  }

  // Check for missing values and set defaults
  for (unsigned i = 0; i < sig.size(); i++) {
    const string &key = sig.keyAt(i);

    if (!has(key)) {
      JSON::ValuePtr value = sig.get(key);
      if (!value->isUndefined()) insert(key, value);
    }
  }
}
