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

#include "Value.h"

#include <cbang/String.h>

#include <Jsrt/ChakraCore.h>

using namespace cb::chakra;
using namespace std;


namespace {
  JsPropertyIdRef getPropID(const string &key) {
    JsPropertyIdRef id;
    CHAKRA_CHECK(JsCreatePropertyIdUtf8(CPP_TO_C_STR(key), key.length(), &id));
    return id;
  }
}


Value::Value(const string &value) {
  CHAKRA_CHECK(JsCreateStringUtf8
               ((uint8_t *)CPP_TO_C_STR(value), value.length(), &ref));
}


string Value::toString() const {
  JsValueRef strRef;
  CHAKRA_CHECK(JsConvertValueToString(ref, &strRef));

  size_t size;
  CHAKRA_CHECK(JsCopyStringUtf8(strRef, 0, 0, &size));

  SmartPointer<char>::Array buffer = new char[size];
  CHAKRA_CHECK(JsCopyStringUtf8(strRef, (uint8_t *)buffer.get(), size, 0));

  return string(buffer.get(), size);
}


int Value::toInteger() const {
  int x;
  CHAKRA_CHECK(JsNumberToInt(ref, &x));
  return x;
}


bool Value::has(const string &key) const {
  bool x;
  CHAKRA_CHECK(JsHasProperty(ref, getPropID(key), &x));
  return x;
}


Value Value::get(const string &key) const {
  JsValueRef propRef;
  CHAKRA_CHECK(JsGetProperty(ref, getPropID(key), &propRef));
  return Value(propRef);
}


bool Value::hasException() {
  bool x = false;
  CHAKRA_CHECK(JsHasException(&x));
  return x;
}


Value Value::getException() {
  JsValueRef ex;
  CHAKRA_CHECK(JsGetAndClearException(&ex));
  return Value(ex);
}
