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

#include "JSONSink.h"
#include "Context.h"
#include "Array.h"
#include "Object.h"

using namespace cb::duk;
using namespace std;


void JSONSink::writeNull() {
  NullSink::writeNull();
  ctx.pushNull();
  put();
}


void JSONSink::writeBoolean(bool value) {
  NullSink::writeBoolean(value);
  ctx.pushBoolean(value);
  put();
}


void JSONSink::write(double value) {
  NullSink::write(value);
  ctx.push(value);
  put();
}


void JSONSink::write(const string &value) {
  NullSink::write(value);
  ctx.push(value);
  put();
}


void JSONSink::beginList(bool simple) {
  NullSink::beginList();
  ctx.pushArray();
}


void JSONSink::beginAppend() {
  NullSink::beginAppend();
  ctx.push(ctx.length());
}


void JSONSink::endList() {
  NullSink::endList();
  put();
}


void JSONSink::beginDict(bool simple) {
  NullSink::beginDict();
  ctx.pushObject();
}


void JSONSink::beginInsert(const string &key) {
  NullSink::beginInsert(key);
  ctx.push(key);
}


void JSONSink::endDict() {
  NullSink::endDict();
  put();
}


void JSONSink::put() {if (getDepth()) ctx.put(-3);}
